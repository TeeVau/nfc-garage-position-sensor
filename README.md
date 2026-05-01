# NFC Garage Position Sensor

![License: GPLv3](https://img.shields.io/badge/license-GPLv3-blue.svg)
![Platform: ESP32-C6](https://img.shields.io/badge/platform-ESP32--C6-1f2937)
![Protocol: Zigbee](https://img.shields.io/badge/protocol-Zigbee-f4b400)
![Sensor: PN532 NFC](https://img.shields.io/badge/sensor-PN532-0891b2)
![Status: Field Tested](https://img.shields.io/badge/status-field--tested-15803d)

ESP32-C6 firmware that reads fixed PN532 NFC tags and reports a garage door's opening percentage to Zigbee2MQTT over Zigbee.

Current release: `0.4.0`

![NFC garage position sensor hero](docs/assets/github-social-preview.png)

## Quick Start

1. Build the firmware with `.\tools\firmware\build.ps1`.
2. Flash the device with `.\tools\firmware\flash.ps1`, or use `.\tools\firmware\flash-clean.ps1` for a fresh Zigbee join.
3. Pair it in Zigbee2MQTT and use [`zigbee2mqtt/external_converters/nfc-garage-position-sensor.js`](./zigbee2mqtt/external_converters/nfc-garage-position-sensor.js) if metadata or reporting is incomplete.
4. Validate the `0 %` and `100 %` tags, then verify LED behavior and payloads with [docs/status-led.md](./docs/status-led.md), [docs/tag-layout.md](./docs/tag-layout.md), and [docs/z2m-setup.md](./docs/z2m-setup.md).

## Overview

- MCU: `ESP32-C6`
- NFC reader: `PN532` via SPI
- Zigbee mode: `ZigbeeWindowCovering` end device
- Local diagnostics: serial logs, onboard `WS2812` status LED
- Optional diagnostics: BLE UART notifications when enabled
- Position model: fixed UID order mapped to `0..100 %`
- Update model: USB flash only, no Wi-Fi / HTTP OTA path

The current runtime logic uses `pendingIndex` plus `pendingCount` to confirm a newly seen NFC tag before it becomes the next real position. That keeps the sketch responsive while reducing false transitions around neighboring tags.

## Features

- Reads passive NFC tags through the PN532 and maps them to door positions
- Reports `position` and `state` over Zigbee with `0 = closed` and `100 = open`
- Publishes both Basic `appVersion` and `swBuildId` so Zigbee2MQTT can resolve Firmware-ID correctly
- Actively reports position updates so live state delivery does not depend on Zigbee2MQTT having completed converter `configure()`
- Uses the ESP32-C6 onboard WS2812 LED for visible runtime status
- Supports local Zigbee diagnostics and factory reset through the onboard button
- Includes helper scripts for build, flash, clean flash, and serial monitoring
- Generates versioned local release binaries under `bin/`

## Hardware

### Core Components

| Component | Purpose |
|-----------|---------|
| ESP32-C6 Dev Module | Main controller and Zigbee radio |
| PN532 | NFC reader |
| Onboard WS2812 LED | Local status indication |
| Onboard button | Diagnostics / factory reset |

### Pin Assignments

| Signal | Pin |
|--------|-----|
| `PIN_SPI_SCK` | `20` |
| `PIN_SPI_MISO` | `19` |
| `PIN_SPI_MOSI` | `18` |
| `PIN_PN532_SS` | `14` |
| `BUTTON_PIN` | `9` |

## Firmware Behavior

### Position Semantics

This project keeps the logical door position in the intuitive form:

- `0 %` = closed
- `100 %` = open

Internally, the Zigbee Window Covering cluster uses inverted lift semantics, so the sketch translates the logical opening percentage before sending it into `ZigbeeWindowCovering`. Zigbee2MQTT should therefore normally use `invert_cover = false`.

### Tag Layout

The UID table is ordered from closed to open. With the current configuration `INDEX_INCREASES_WHEN_OPENING = true`, the first configured tag represents `0 %` and the last tag represents `100 %`.

See [docs/tag-layout.md](./docs/tag-layout.md) for the full table and mapping details.

### Local LED Status

The onboard WS2812 LED gives a quick local indication even when no serial monitor is attached.

| State | LED behavior |
|-------|--------------|
| Boot | Short white flash |
| Startup / early join | Slow blue blink |
| Zigbee not ready after stack start | Fast blue blink |
| Ready, no confirmed tag yet | Very dim green |
| Confirmed closed (`0 %`) | Green |
| Confirmed non-closed (`> 0 %`) | Orange |
| Fatal startup error | Red blink |

The startup indicator is intentionally held visible for a few seconds so a fast Zigbee rejoin is still human-noticeable.

See [docs/status-led.md](./docs/status-led.md) for the detailed LED behavior and validation flow.

## Zigbee And Zigbee2MQTT

The device currently exposes:

- endpoint `10`: Window Covering
- Basic `modelId`
- Basic `manufacturerName`
- Basic `powerSource`
- Basic `appVersion`
- Basic `swBuildId`

Important project detail:

- `setVersion()` alone is not enough for Zigbee2MQTT Firmware-ID
- the firmware therefore adds Basic `swBuildId` explicitly before `Zigbee.begin()`
- Zigbee2MQTT can complete `interview` successfully while still skipping the converter `configure()` step
- the firmware therefore actively reports the cover position directly to coordinator endpoint `1`, so state delivery no longer depends on the binding/reporting setup being present

If Zigbee2MQTT needs help classifying the device, use the repository-provided external converter:

- [zigbee2mqtt/external_converters/nfc-garage-position-sensor.js](./zigbee2mqtt/external_converters/nfc-garage-position-sensor.js)

For the full pairing, metadata, and expected payload behavior, see [docs/z2m-setup.md](./docs/z2m-setup.md).

### FHEM Integration

When FHEM consumes the Zigbee2MQTT MQTT stream, use the main device topic and the availability subtopic separately:

- `zigbee2mqtt/ga_Torsensor`: main JSON payload such as `{"last_seen":"...","linkquality":196,"position":0,"state":"CLOSE"}`
- `zigbee2mqtt/ga_Torsensor/availability`: availability JSON such as `{"state":"online"}`

The repository includes a tested `MQTT2_DEVICE` example and attrTemplate snippet that:

- uses `attr ... devicetopic zigbee2mqtt/ga_Torsensor`
- keeps the standard Zigbee2MQTT reading name `linkquality`
- maps the availability subtopic to a dedicated FHEM reading `availability`
- renders both a garage-door icon and the formatted state text through `devStateIcon`

See [docs/z2m-setup.md](./docs/z2m-setup.md) for the full FHEM configuration and [examples/fhem-mqtt2-device-template.txt](./examples/fhem-mqtt2-device-template.txt) for a copy/paste-ready example.

## Build, Flash, And Monitor

The Arduino sketch now lives under `src/nfc-garage-position-sensor`, while the helper scripts for build, flash, and monitoring live under `tools/firmware`.

### Common Commands

```powershell
.\tools\firmware\build.ps1
.\tools\firmware\flash.ps1
.\tools\firmware\monitor.ps1
```

Clean flash for a fresh Zigbee join:

```powershell
.\tools\firmware\flash-clean.ps1
```

If PowerShell blocks local scripts:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\firmware\build.ps1
powershell -ExecutionPolicy Bypass -File .\tools\firmware\flash.ps1
powershell -ExecutionPolicy Bypass -File .\tools\firmware\monitor.ps1
```

BLE debug build for garage-side testing:

```powershell
.\tools\firmware\build.ps1 -EnableBleDebug
.\tools\firmware\flash.ps1 -EnableBleDebug
```

Each build also copies a versioned local release binary into `bin/`, for example:

```text
bin/nfc-garage-position-sensor-0.4.0.bin
bin/nfc-garage-position-sensor-0.4.0-ble-debug.bin
```

Short serial capture with optional log file:

```powershell
.\tools\firmware\serial-capture.ps1 -Port COM3 -DurationSeconds 30 -OutputFile .\tmp\serial.log
```

Short MQTT capture for Zigbee2MQTT bridge topics plus one device topic:

```powershell
.\tools\mqtt-capture.ps1 -BrokerHost 192.168.178.2 -DeviceId 0x58e6c5fffedd775c -DurationSeconds 30 -OutputFile .\tmp\mqtt.log
```

Short MQTT capture for one explicit topic:

```powershell
.\tools\mqtt-capture.ps1 -BrokerHost 192.168.178.2 -Topic zigbee2mqtt/ga_Torsensor -DurationSeconds 5 -PassThru
```

This uses a separate build variant so the normal and BLE-debug artifacts do not overwrite each other.

The monitor script can take a bit of time to attach and start showing data after a reset or reconnect. Give it a short grace period before treating the connection as failed.

## Arduino IDE Settings

Tested with:

- Board: `ESP32C6 Dev Module`
- Zigbee Mode: `Zigbee ED`
- Partition Scheme: `zigbee`
- CDC On Boot: `cdc`

On this machine, Arduino IDE compile times can take several minutes.

## Software Dependencies

### Required Libraries / Core Features

| Library / Core Component | Purpose |
|--------------------------|---------|
| `SPI` | PN532 communication |
| `Adafruit_PN532` | NFC reader access |
| `Zigbee` / `ZigbeeWindowCovering` | Zigbee endpoint and reporting |
| ESP32 Arduino RGB LED API | Onboard WS2812 control |

### Optional

| Library | Purpose |
|---------|---------|
| `NimBLEDevice` | BLE UART debug output when `BLE_DEBUG_ENABLED` is enabled |

## Repository Layout

### Top-Level Structure

- `src/nfc-garage-position-sensor`: firmware sketch and local source files
- `tools/firmware`: build, flash, monitor, and serial-capture helpers
- `docs`: user-facing setup notes and validation-oriented documentation
- `zigbee2mqtt`: repository-owned Zigbee2MQTT converter files
- `Documents`: functional specification and longer-form project documentation

### Key Files

- [src/nfc-garage-position-sensor/nfc-garage-position-sensor.ino](./src/nfc-garage-position-sensor/nfc-garage-position-sensor.ino): setup, loop, and shared globals
- [src/nfc-garage-position-sensor/config.h](./src/nfc-garage-position-sensor/config.h): project constants and shared configuration
- [src/nfc-garage-position-sensor/nfc_logic.ino](./src/nfc-garage-position-sensor/nfc_logic.ino): PN532 handling and position state machine
- [src/nfc-garage-position-sensor/tag_map.ino](./src/nfc-garage-position-sensor/tag_map.ino): UID table and percent conversion
- [src/nfc-garage-position-sensor/zigbee.ino](./src/nfc-garage-position-sensor/zigbee.ino): Zigbee setup, status, and publishing
- [src/nfc-garage-position-sensor/status_led.ino](./src/nfc-garage-position-sensor/status_led.ino): onboard WS2812 status indication
- [src/nfc-garage-position-sensor/ble_debug.ino](./src/nfc-garage-position-sensor/ble_debug.ino): BLE UART debug transport
- [src/nfc-garage-position-sensor/button.ino](./src/nfc-garage-position-sensor/button.ino): short-press diagnostics and long-press reset
- [tools/firmware/build.ps1](./tools/firmware/build.ps1): compile helper
- [tools/firmware/flash.ps1](./tools/firmware/flash.ps1): upload while keeping pairing state
- [tools/firmware/flash-clean.ps1](./tools/firmware/flash-clean.ps1): clean-flash upload for re-pair workflows
- [tools/firmware/monitor.ps1](./tools/firmware/monitor.ps1): serial monitor with reconnect handling
- [tools/firmware/serial-capture.ps1](./tools/firmware/serial-capture.ps1): short serial capture helper with optional timestamps and file logging
- [tools/mqtt-capture.ps1](./tools/mqtt-capture.ps1): generic MQTT capture helper for Zigbee2MQTT bridge topics, explicit MQTT topics, and one device topic
- [tools/z2m-join.ps1](./tools/z2m-join.ps1): Zigbee2MQTT join helper
- [docs/tag-layout.md](./docs/tag-layout.md): UID order and percent mapping
- [docs/status-led.md](./docs/status-led.md): LED state documentation
- [docs/z2m-setup.md](./docs/z2m-setup.md): Zigbee2MQTT setup and expectations
- [examples/fhem-mqtt2-device-template.txt](./examples/fhem-mqtt2-device-template.txt): tested FHEM `MQTT2_DEVICE` example and local attrTemplate snippet
- [Documents/nfc-garage-position-sensor-fsd.md](./Documents/nfc-garage-position-sensor-fsd.md): functional specification document

## Troubleshooting

### Zigbee2MQTT Shows Inverted Position

Check that:

- the current firmware is flashed
- `invert_cover` in Zigbee2MQTT is `false`

Expected:

- `0 %` tag -> `position: 0`, `state: CLOSE`
- `100 %` tag -> `position: 100`, `state: OPEN`

### Firmware-ID Is Missing

Re-pair the device after flashing significant Zigbee metadata changes. This project relies on Basic `swBuildId` in addition to `appVersion`.

### The Monitor Starts Slowly

That can be normal after reset or USB reconnect. Wait for the monitor to finish attaching before retrying.

### The LED State Is Unexpected

Compare the visible LED pattern against [docs/status-led.md](./docs/status-led.md) and, if needed, verify it against serial output from `tools/firmware/monitor.ps1`.

### The Device Rejoins But Zigbee2MQTT Does Not Interview It Cleanly

Use the garage-oriented checklist in [docs/garage-debug-checklist.md](./docs/garage-debug-checklist.md). It walks through BLE debug build, clean Zigbee2MQTT interview setup, and parent-router sanity checks.

If Zigbee2MQTT shows the device as supported but endpoint `10` still has empty `bindings` and `configured_reportings`, the interview finished but converter `configure()` did not run. A manual Zigbee2MQTT `reconfigure` will restore the metadata, but current firmware builds already send position updates directly to the coordinator so runtime status is still delivered.

## Further Documentation

- [docs/status-led.md](./docs/status-led.md)
- [docs/tag-layout.md](./docs/tag-layout.md)
- [docs/z2m-setup.md](./docs/z2m-setup.md)
- [docs/garage-debug-checklist.md](./docs/garage-debug-checklist.md)
- [Documents/nfc-garage-position-sensor-fsd.md](./Documents/nfc-garage-position-sensor-fsd.md)

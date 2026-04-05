# NFC Garage Position Sensor

ESP32-C6 sketch for a garage door position sensor based on PN532 NFC tags.

The firmware reads NFC tag UIDs, maps them to a logical door position, derives movement direction, and reports the current state over Zigbee as a window covering device. BLE debug output stays available for local diagnostics.

## Status

- MCU: `ESP32-C6`
- NFC reader: `PN532` via SPI
- Zigbee: `ZigbeeWindowCovering`
- Debug: serial + BLE notifications
- Position model: fixed UID order mapped to `0..100 %`

The current tag transition logic confirms a newly seen tag with `pendingIndex` and `pendingCount` before accepting it as the next real position.

## Repository Layout

- [nfc-garage-position-sensor.ino](./nfc-garage-position-sensor.ino): current firmware
- [config.h](./config.h): project constants and shared configuration
- [tag_map.ino](./tag_map.ino): NFC tag table and UID helper functions
- [nfc_logic.ino](./nfc_logic.ino): PN532 handling and position state machine
- [zigbee.ino](./zigbee.ino): Zigbee setup, status, and publishing
- [ble_debug.ino](./ble_debug.ino): BLE UART debug output
- [button.ino](./button.ino): local button handling
- [build.ps1](./build.ps1): compile helper
- [flash.ps1](./flash.ps1): normal upload, keeps Zigbee pairing
- [flash-clean.ps1](./flash-clean.ps1): full erase upload for a fresh Zigbee join
- [monitor.ps1](./monitor.ps1): serial monitor with reconnect handling
- [docs/tag-layout.md](./docs/tag-layout.md): UID order and percent mapping
- [docs/garage-door-position-tracker-codex-handoff.json](./docs/garage-door-position-tracker-codex-handoff.json): earlier project handoff notes
- [examples/Zigbee_Window_Covering](./examples/Zigbee_Window_Covering): reference example kept for comparison

## Build And Flash

```powershell
.\build.ps1
.\flash.ps1
.\monitor.ps1
```

Clean flash for a fresh Zigbee join:

```powershell
.\flash-clean.ps1
```

If PowerShell blocks scripts locally:

```powershell
powershell -ExecutionPolicy Bypass -File .\monitor.ps1
```

## Arduino IDE Settings

Tested with:

- Board: `ESP32C6 Dev Module`
- Zigbee Mode: `Zigbee ED`
- Partition Scheme: `zigbee`
- CDC On Boot: `cdc`

On this machine, Arduino IDE compile times can be around 10 minutes.

## Zigbee Behavior

- The device joins as Zigbee end device.
- Position is published as lift percentage.
- Direction is derived from confirmed index changes.
- If no confirmed movement happens for `STOP_DETECT_MS`, the state changes to `stopped`.

`flash.ps1` keeps existing Zigbee pairing. Use `flash-clean.ps1` only when you explicitly want a clean re-pair.

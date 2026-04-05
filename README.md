# NFC Garage Position Sensor

ESP32-C6 sketch for a garage door position sensor based on PN532 NFC tags.

The firmware reads NFC tag UIDs, maps them to a logical door position, and reports the current state over Zigbee as a window covering device. Serial debug output stays available for local diagnostics, and BLE debug can be re-enabled when needed.

## Status

- MCU: `ESP32-C6`
- NFC reader: `PN532` via SPI
- Zigbee: `ZigbeeWindowCovering`
- Debug: serial, optional BLE notifications
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
- [TODO.md](./TODO.md): deferred topics and next engineering tasks
- [docs/tag-layout.md](./docs/tag-layout.md): UID order and percent mapping
- [docs/z2m-setup.md](./docs/z2m-setup.md): Zigbee2MQTT setup
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
- Position is published as lift percentage with cover semantics: `0 = closed`, `100 = open`.

`INDEX_INCREASES_WHEN_OPENING` in [config.h](./config.h) defines how the UID table maps to Zigbee cover semantics.

We keep the firmware aligned with the SDK and Zigbee2MQTT semantics:

- `0 %` opening = `CLOSE`
- `100 %` opening = `OPEN`
- the tag labeled `0 %` represents a closed door
- the tag labeled `100 %` represents a fully open door

With the current setting `true`, index `0` maps to `0 % / CLOSE` and the last index maps to `100 % / OPEN`, matching a UID table that runs from closed to open.

Before sending the value into `ZigbeeWindowCovering`, the firmware converts the opening percentage to the Zigbee Window Covering lift percentage used by the cluster in practice. This keeps our project semantics stable even though the cluster-facing value is inverted.

For Zigbee2MQTT this means `invert_cover` should normally stay `false`. If Zigbee2MQTT still shows the opposite direction, check that device option there as well.

`flash.ps1` keeps existing Zigbee pairing. Use `flash-clean.ps1` only when you explicitly want a clean re-pair.

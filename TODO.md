# Project TODO

## Deferred Topics

### OTA Updates Over Temporary Wi-Fi AP

Status: postponed for later implementation.

Decision:
- Do not implement Zigbee OTA for now.
- Preferred future approach is OTA over a temporary Wi-Fi access point with a web updater.

Why this approach:
- Lower implementation complexity than Zigbee OTA for this single-device / small-device setup.
- Better fit for the current ESP32-C6 Arduino firmware.
- Can be activated only on demand, so normal Zigbee operation stays unchanged.

Expected future behavior:
- Device can enter a dedicated update mode.
- In update mode, Zigbee is paused and the device starts a temporary Wi-Fi AP.
- Firmware update is uploaded through a local web page.
- After a successful update, the device reboots back into normal Zigbee mode.

Open implementation points:
- Define how update mode is entered, for example long button press at boot or a separate long-press threshold.
- Verify or introduce an OTA-capable partition layout (`ota_0`, `ota_1`, `otadata`) compatible with the project.
- Protect the temporary AP and updater page with a password.
- Add a timeout so update mode exits automatically if unused.
- Document the update flow in the README once implementation starts.

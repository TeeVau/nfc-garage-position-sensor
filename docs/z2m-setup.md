# Zigbee2MQTT Setup For The Garage Door Sensor

This document describes how to set up the garage door sensor in Zigbee2MQTT for the branch `codex/zigbee-direction`.

Goal:

- `position`: opening percentage with `0 = closed`, `100 = open`
- `state`: cover state as `CLOSE` / `OPEN`
- `direction`: `unknown | opening | closing | stopped`

The firmware on this branch exposes:

- endpoint `10`: window covering
- endpoint `11`: direction as multistate input

The direction field in Zigbee2MQTT requires the external converter in [zigbee2mqtt-external-converter.js](./zigbee2mqtt-external-converter.js).
The current converter reads:

- `position` and `state` from endpoint `10` / `closuresWindowCovering`
- `direction` and `direction_code` from endpoint `11` / `genMultistateInput`

## Prerequisites

- Firmware from branch `codex/zigbee-direction` is flashed to the ESP32-C6.
- The device can join your Zigbee network.
- You have access to the Zigbee2MQTT data directory.

Official Zigbee2MQTT references used for this setup:

- [External converters](https://www.zigbee2mqtt.io/advanced/more/external_converters.html)
- [More configuration options](https://www.zigbee2mqtt.io/guide/configuration/more-config-options.html)
- [Allowing devices to join](https://www.zigbee2mqtt.io/guide/usage/pairing_devices.html)

## 1. Install The External Converter

Copy [zigbee2mqtt-external-converter.js](./zigbee2mqtt-external-converter.js) into the Zigbee2MQTT `external_converters` folder.

Typical path:

```text
<zigbee2mqtt-data-dir>/external_converters/nfc-garage-position-sensor.js
```

Example Docker/Home Assistant style layout:

```text
.../zigbee2mqtt/data/external_converters/nfc-garage-position-sensor.js
```

Important:

- The file must be placed next to `configuration.yaml`, inside the `external_converters` subfolder.
- The converter file name can be chosen freely, but using `nfc-garage-position-sensor.js` keeps things obvious.

Alternative:

- You can also add the converter via Zigbee2MQTT frontend under `Settings -> Dev console -> External converters`.

## 2. Restart Zigbee2MQTT

Restart Zigbee2MQTT after placing the converter file.

After restart, check the Zigbee2MQTT log. The converter should load without syntax errors.

If there is a converter error, fix that first before pairing or re-pairing the device.

## 3. Pair Or Re-Pair The Device

Recommended path:

1. Remove the already known garage sensor from Zigbee2MQTT.
2. Permit joining in Zigbee2MQTT.
3. Put the ESP32-C6 device into pairing mode again.
4. Let Zigbee2MQTT interview the device from scratch.

Why this is recommended:

- The firmware changed on this branch.
- A fresh interview is the most reliable way to make sure Zigbee2MQTT sees both endpoint `10` and endpoint `11`.

If you do not want to remove the device first:

- you can try `Reconfigure` / `Interview` from the Zigbee2MQTT frontend
- but a full remove + re-pair is the safer option for endpoint changes

This is an engineering recommendation based on the new endpoint shape; Zigbee2MQTT documentation does not guarantee that every endpoint shape change is picked up by a partial refresh.

## 4. Expected Device Behavior In Zigbee2MQTT

After successful interview, the device should expose the normal cover data and, with the external converter, also the direction fields.

Expected payload fields:

- `position`
- `state`
- `direction`
- `direction_code`

Expected semantics:

- `position: 0` means fully closed
- `position: 100` means fully open
- `state: "CLOSE"` means closed
- `state: "OPEN"` means open
- `direction: "opening"` means the door is currently moving open
- `direction: "closing"` means the door is currently moving closed
- `direction: "stopped"` means movement has stopped
- `direction: "unknown"` is the initial fallback before a meaningful movement state is known

Direction code mapping:

- `0 = unknown`
- `1 = opening`
- `2 = closing`
- `3 = stopped`

## 5. What You Should See During Tests

### Scan The 0% Tag

Expected meaning:

- door is closed
- opening percentage is `0`

Expected Zigbee2MQTT result:

```json
{
  "position": 0,
  "state": "CLOSE"
}
```

If the sensor is not moving anymore, direction should become:

```json
{
  "direction": "stopped",
  "direction_code": 3
}
```

### Scan The 100% Tag

Expected meaning:

- door is fully open
- opening percentage is `100`

Expected Zigbee2MQTT result:

```json
{
  "position": 100,
  "state": "OPEN"
}
```

### Move From 0% Toward 100%

Expected meaning:

- door is opening

Expected Zigbee2MQTT result while moving:

```json
{
  "direction": "opening",
  "direction_code": 1
}
```

### Move From 100% Toward 0%

Expected meaning:

- door is closing

Expected Zigbee2MQTT result while moving:

```json
{
  "direction": "closing",
  "direction_code": 2
}
```

## 6. Console vs Zigbee2MQTT

The firmware logs two related but different things:

- `TAG ...` lines: internal NFC-derived state
- `ZB ...` lines: values sent into Zigbee

Example:

```text
TAG 0 0 closing nfc
ZB open=0 lift=100 nfc
ZB dir=closing code=2 nfc
```

Interpretation:

- `open=0` is the project-level opening percentage
- `lift=100` is the Zigbee Window Covering cluster-facing value
- `dir=closing` is the direction value sent by the firmware over Zigbee

The inverted `lift` value is expected internally for the window covering cluster implementation used here.

## 7. Troubleshooting

### Position Looks Inverted

Check:

- the firmware branch really is `codex/zigbee-direction`
- the flashed firmware is current
- Zigbee2MQTT device option `invert_cover` is `false`

Expected:

- `0%` tag -> `position: 0`, `state: CLOSE`
- `100%` tag -> `position: 100`, `state: OPEN`

### Direction Does Not Appear

Check:

- the external converter is loaded successfully
- the device was re-paired or at least freshly interviewed after flashing this branch
- endpoint `11` exists in the device interview details

If `position` works but `direction` does not appear, the most likely causes are:

- converter not loaded
- endpoint `11` is not being reported to Zigbee2MQTT

### Raw Debug Logging

To debug the real Zigbee traffic instead of only the summarized frontend log, temporarily enable debug logging in `configuration.yaml`:

```yaml
advanced:
  log_level: debug
```

If you also want debug logs visible in the Zigbee2MQTT frontend and MQTT stream, enable:

```yaml
advanced:
  log_level: debug
  log_debug_to_mqtt_frontend: true
```

After restart, move the door and look for raw messages involving:

- cluster `closuresWindowCovering`
- cluster `genMultistateInput`
- endpoint `10`
- endpoint `11`

If you see `closuresWindowCovering` but never `genMultistateInput`, then the problem is not the converter naming but the missing raw direction report from endpoint `11`.

### Only `direction_code` Works But Not `direction`

That usually points to a converter issue in Zigbee2MQTT.

Check:

- that [zigbee2mqtt-external-converter.js](./zigbee2mqtt-external-converter.js) was copied unchanged
- Zigbee2MQTT startup logs for converter parse errors

## 8. FHEM Expectations

If FHEM consumes the Zigbee2MQTT MQTT payload, you should eventually see fields equivalent to:

- `ga_Torsensor_position = 0..100`
- `ga_Torsensor_state = OPEN | CLOSE`
- `ga_Torsensor_direction = unknown | opening | closing | stopped`

The exact FHEM reading names depend on your MQTT mapping.

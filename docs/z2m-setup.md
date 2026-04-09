# Zigbee2MQTT Setup For The Garage Door Sensor

This document describes the current simplified Zigbee2MQTT setup for the garage door sensor.

Goal:

- `position`: opening percentage with `0 = closed`, `100 = open`
- `state`: cover state as `CLOSE` / `OPEN`

The firmware currently exposes:

- endpoint `10`: window covering

Metadata relevant for Zigbee2MQTT:

- Basic `modelId`
- Basic `manufacturerName`
- Basic `powerSource`
- Basic `appVersion`
- Basic `swBuildId`

## External Converter In This Repository

If Zigbee2MQTT still classifies the device as unsupported or exposes incomplete metadata, use the repository-provided external converter:

- `zigbee2mqtt/external_converters/nfc-garage-position-sensor.js`

What this converter does:

- fingerprints the device by Basic `modelId = nfc-garage-position-sensor`
- fingerprints the device by Basic `manufacturerName = TeeVau`
- exposes read-only `position` and `state`
- binds reporting for `closuresWindowCovering` on endpoint `10`

Recommended installation:

1. Copy `zigbee2mqtt/external_converters/nfc-garage-position-sensor.js` into Zigbee2MQTT's `external_converters` directory next to its `configuration.yaml`.
2. Restart Zigbee2MQTT.
3. Remove the previously interviewed device if needed.
4. Pair the sensor again so Zigbee2MQTT interviews it with the external converter active.

Important note:

- The converter is intentionally read-only. The device is used as a position sensor even though it reports through the Window Covering cluster.

## Firmware-ID And Version Metadata

Zigbee2MQTT distinguishes between multiple version-related Basic-cluster attributes.

Important project detail:

- Espressif's documented Arduino Zigbee API exposes `setVersion()`, which writes Basic `appVersion`.
- In practice Zigbee2MQTT still reads Basic `swBuildId` separately for the UI field `Firmware-ID`.
- Our tests showed that `appVersion` alone is not enough: Zigbee2MQTT successfully read `appVersion`, but still reported `swBuildId` as `UNSUPPORTED_ATTRIBUTE`, which left `Firmware-ID` unresolved.

Because of that, the firmware intentionally contains a small local extension around `ZigbeeWindowCovering` that writes Basic attribute `0x4000` (`swBuildId`) before `Zigbee.begin()`.

This is deliberate and should not be removed as a "cleanup" unless Zigbee2MQTT is verified again against the real device.

Current mapping used by this project:

- `SOFTWARE_APPLICATION_VERSION` -> Basic `appVersion`
- `SOFTWARE_VERSION` -> serial startup banner and Basic `swBuildId`

## Pair Or Re-Pair The Device

Recommended path:

1. Remove the already known garage sensor from Zigbee2MQTT.
2. Permit joining in Zigbee2MQTT.
3. Put the ESP32-C6 device into pairing mode again.
4. Let Zigbee2MQTT interview the device from scratch.

Why this is recommended:

- The firmware changed.
- A fresh interview is the most reliable way to make sure Zigbee2MQTT sees the current endpoint shape.

## Expected Device Behavior In Zigbee2MQTT

Expected payload fields:

- `position`
- `state`

Expected semantics:

- `position: 0` means fully closed
- `position: 100` means fully open
- `state: "CLOSE"` means closed
- `state: "OPEN"` means open

For integrations and dashboards, use the unsuffixed fields:

- `position`
- `state`

If Zigbee2MQTT also shows endpoint-suffixed variants such as `position_10`, `position_default`, `state_10`, or `state_default`, treat those as secondary endpoint-specific or cached values. The intended stable consumer-facing fields for this project are the plain `position` and `state` keys.

## What You Should See During Tests

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

## Console vs Zigbee2MQTT

The firmware logs two related but different things:

- `TAG ...` lines: internal NFC-derived state
- `ZB ...` lines: values sent into Zigbee

Example:

```text
TAG 0 0 nfc
ZB open=0 lift=100 nfc
```

Interpretation:

- `open=0` is the project-level opening percentage
- `lift=100` is the Zigbee Window Covering cluster-facing value

The inverted `lift` value is expected internally for the window covering cluster implementation used here.

## Troubleshooting

### Position Looks Inverted

Check:

- the flashed firmware is current
- Zigbee2MQTT device option `invert_cover` is `false`

Expected:

- `0%` tag -> `position: 0`, `state: CLOSE`
- `100%` tag -> `position: 100`, `state: OPEN`

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
- endpoint `10`

## FHEM Expectations

If FHEM consumes the Zigbee2MQTT MQTT payload, you should eventually see fields equivalent to:

- `ga_Torsensor_position = 0..100`
- `ga_Torsensor_state = OPEN | CLOSE`

The exact FHEM reading names depend on your MQTT mapping.

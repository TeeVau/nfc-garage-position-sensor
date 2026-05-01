# Quick Verification

Use this short check after wiring, flashing, and pairing the device.

## Basic Checks

1. Power-cycle the board and confirm the boot and startup LED patterns.
2. Confirm the device joins Zigbee2MQTT successfully.
3. Present the `0 %` tag and verify `position = 0`.
4. Present the `100 %` tag and verify `position = 100`.
5. Move through the intermediate tags and confirm stable intermediate positions.

## Expected Behavior

- `invert_cover` should normally remain `false` in Zigbee2MQTT
- the onboard LED should reflect boot, join, ready, and position states
- live position updates should arrive without requiring manual reconfiguration

## If Something Looks Wrong

- Re-check the tag order in [tag-layout.md](./tag-layout.md)
- Re-check the physical installation in [assembly.md](./assembly.md)
- Re-check Zigbee expectations in [z2m-setup.md](./z2m-setup.md)

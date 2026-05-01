# Assembly Guide

This guide focuses on the physical installation so the reader sees the tags reliably across the full garage door travel path.

## Installation Goals

- The PN532 should read only the intended nearby tag at each position.
- Tag order should match the configured closed-to-open firmware mapping.
- The reader and wiring should stay mechanically stable over repeated movement and vibration.

## Recommended Build Order

1. Bench-test the ESP32-C6 and PN532 on the desk.
2. Confirm the firmware reads the `0 %` and `100 %` tags before mounting anything permanently.
3. Mark the garage door travel path from fully closed to fully open.
4. Place the 10 tags in ascending order along that path.
5. Mount the PN532 so it passes the tags with consistent spacing during movement.
6. Re-test the full travel path and adjust spacing where neighboring reads are unstable.

## Reader Placement

- Keep the PN532 parallel to the tag surface where possible.
- Avoid metal directly behind the tags unless you have already validated the exact tag and mounting combination.
- Keep the read distance consistent along the whole path.
- Secure the reader so it cannot twist, sag, or vibrate into a different angle over time.

## Tag Placement

- Use the released 10-tag order from [tag-layout.md](./tag-layout.md).
- Start with the `0 %` tag at the fully closed end.
- End with the `100 %` tag at the fully open end.
- Keep spacing large enough that the reader can settle cleanly on one tag at a time.
- If two neighboring tags produce unstable readings, increase spacing before changing firmware constants.

## Mounting Suggestions

- Use removable tape during the first calibration pass.
- Switch to stronger adhesive or brackets only after the layout is proven.
- Add strain relief to the PN532 wiring near both the board and the reader.
- Protect the board from moisture, dust, and accidental cable pulls.

## Verification After Installation

1. Power-cycle the device and confirm the LED startup sequence.
2. Move the door to the fully closed position and verify `position = 0`.
3. Move the door to the fully open position and verify `position = 100`.
4. Pass through the intermediate tags and confirm stable intermediate values.
5. Repeat several open/close cycles to check for drift or intermittent reads.

## If Readings Are Unstable

- Re-check the physical gap between reader and tag.
- Increase spacing between neighboring tags.
- Improve reader rigidity before changing firmware confirmation logic.
- Review [status-led.md](./status-led.md) and [z2m-setup.md](./z2m-setup.md) for runtime behavior clues.

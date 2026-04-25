# Onboard Status LED

This project uses the ESP32-C6 onboard WS2812 LED as a local runtime indicator.

The implementation lives in `src/nfc-garage-position-sensor/status_led.ino` and uses the ESP32 Arduino core's `RGB_BUILTIN` / `rgbLedWrite()` path instead of an extra NeoPixel library.

## State Mapping

| Situation | LED behavior | Notes |
|-----------|--------------|-------|
| Boot | Short white flash | Confirms that firmware started at all |
| Startup / early join | Slow blue blink | Guaranteed visible for `STATUS_LED_STARTUP_VISIBLE_MS` so fast rejoins are still noticeable |
| Zigbee stack started but not yet ready | Fast blue blink | Indicates that the device is still waiting to become ready |
| Ready, but no confirmed NFC position yet | Very dim green | Keeps normal idle behavior unobtrusive |
| Confirmed closed position (`0 %`) | Green | Stable closed indication |
| Confirmed non-closed position (`> 0 %`) | Orange | First implementation intentionally treats every non-closed state as "not closed" |
| Fatal startup error | Red blink | Currently triggered by PN532 init failure or Zigbee begin timeout |

## Why The Startup Window Exists

In practice, the ESP32-C6 can rejoin Zigbee quickly enough that a purely runtime-driven blue state would be too short to notice.

To keep the visual feedback useful in the real world, the firmware forces a visible startup window before Zigbee initialization continues.

Current defaults from `src/nfc-garage-position-sensor/config.h`:

- `STATUS_LED_BOOT_FLASH_MS = 80`
- `STATUS_LED_STARTUP_VISIBLE_MS = 3000`
- `STATUS_LED_START_BLINK_MS = 700`
- `STATUS_LED_PAIRING_BLINK_MS = 180`
- `STATUS_LED_ERROR_BLINK_MS = 250`

## Practical Test Flow

1. Power-cycle or reset the device.
2. Confirm the short white boot flash.
3. Confirm the visible blue startup blink.
4. Present the `0 %` NFC tag and confirm solid green.
5. Present any non-zero tag and confirm orange.

For a deeper diagnostic run, attach the serial monitor in parallel and compare the LED behavior against `ZB ...` and `TAG ...` log lines.

## Current Scope And Future Ideas

The current implementation covers the recommended first scope from issue `#3`:

- boot
- startup / pairing visibility
- normal operation
- closed
- open / not closed
- error

Possible future refinements:

- dedicated intermediate-state pattern
- short event flash when a new tag is confirmed
- stronger distinction between "fully open" and "partially open"

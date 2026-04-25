#if defined(RGB_BUILTIN)
#include "esp32-hal-rgb-led.h"
#endif

static constexpr uint32_t statusLedColor(uint8_t red, uint8_t green, uint8_t blue) {
  return (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(green) << 8) | blue;
}

static constexpr uint32_t STATUS_LED_OFF = statusLedColor(0, 0, 0);
static constexpr uint32_t STATUS_LED_BOOT = statusLedColor(48, 48, 48);
static constexpr uint32_t STATUS_LED_READY = statusLedColor(0, 6, 0);
static constexpr uint32_t STATUS_LED_CLOSED = statusLedColor(0, 24, 0);
static constexpr uint32_t STATUS_LED_OPEN = statusLedColor(28, 10, 0);
static constexpr uint32_t STATUS_LED_JOIN = statusLedColor(0, 0, 20);
static constexpr uint32_t STATUS_LED_PAIRING = statusLedColor(0, 0, 40);
static constexpr uint32_t STATUS_LED_ERROR = statusLedColor(36, 0, 0);

bool statusLedErrorActive = false;

bool statusLedBlinkOn(uint32_t now, uint32_t intervalMs) {
  if (intervalMs == 0) {
    return true;
  }

  return ((now / intervalMs) % 2U) == 0U;
}

void writeStatusLed(uint32_t color) {
#if defined(RGB_BUILTIN)
  rgbLedWrite(
    RGB_BUILTIN,
    static_cast<uint8_t>((color >> 16) & 0xFF),
    static_cast<uint8_t>((color >> 8) & 0xFF),
    static_cast<uint8_t>(color & 0xFF)
  );
#else
  (void)color;
#endif
}

void setupStatusLed() {
  writeStatusLed(STATUS_LED_BOOT);
  delay(STATUS_LED_BOOT_FLASH_MS);
  writeStatusLed(STATUS_LED_OFF);
}

void showStatusLedStartupWindow() {
  uint32_t startMs = millis();

  while (millis() - startMs < STATUS_LED_STARTUP_VISIBLE_MS) {
    uint32_t now = millis();
    writeStatusLed(statusLedBlinkOn(now, STATUS_LED_START_BLINK_MS) ? STATUS_LED_JOIN : STATUS_LED_OFF);
    delay(25);
  }

  writeStatusLed(STATUS_LED_OFF);
}

void setStatusLedError() {
  statusLedErrorActive = true;
}

void pollStatusLed() {
  uint32_t color = STATUS_LED_OFF;
  uint32_t now = millis();

  if (statusLedErrorActive) {
    color = statusLedBlinkOn(now, STATUS_LED_ERROR_BLINK_MS) ? STATUS_LED_ERROR : STATUS_LED_OFF;
  } else if (!Zigbee.started()) {
    color = statusLedBlinkOn(now, STATUS_LED_START_BLINK_MS) ? STATUS_LED_JOIN : STATUS_LED_OFF;
  } else if (!zigbeeReady) {
    color = statusLedBlinkOn(now, STATUS_LED_PAIRING_BLINK_MS) ? STATUS_LED_PAIRING : STATUS_LED_OFF;
  } else if (stableIndex < 0) {
    color = STATUS_LED_READY;
  } else {
    // For the first iteration of the feature, every confirmed non-closed
    // position is shown as orange to make "not closed" immediately visible.
    color = (indexToPercent(stableIndex) == 0) ? STATUS_LED_CLOSED : STATUS_LED_OPEN;
  }

  writeStatusLed(color);
}

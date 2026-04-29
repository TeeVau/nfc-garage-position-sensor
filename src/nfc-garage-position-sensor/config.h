#pragma once

static constexpr const char* PROJECT_NAME = "nfc-garage-position-sensor";
// One shared version string keeps serial output and Zigbee2MQTT Firmware-ID in sync.
// Zigbee2MQTT reads this value from Basic.swBuildId, so keep it as a plain
// Semantic Versioning 2.0.0 dotted string.
static constexpr const char* SOFTWARE_VERSION = "0.2.6";
static constexpr uint8_t SOFTWARE_APPLICATION_VERSION = 26;
static constexpr const char* ZB_MFR = "TeeVau";
static constexpr const char* ZB_MODEL = "nfc-garage-position-sensor";
static constexpr const char* BLE_DEVICE_NAME = "garage-sensor";

#ifndef BLE_DEBUG_ENABLED
#define BLE_DEBUG_ENABLED 0
#endif

static constexpr uint32_t ZB_STATUS_INTERVAL_MS = 30000;
// Keep all Zigbee channels enabled for fresh pairing. The ESP32-C6 stack can be
// flaky while scanning, and the channel-15-only optimization blocked re-pairing.
static constexpr uint32_t ZB_PRIMARY_CHANNEL_MASK = 0x07FFF800;
// Real-world scans needed about 3 minutes before the network was found.
static constexpr uint32_t ZB_JOIN_TIMEOUT_MS = 240000;
static constexpr uint32_t ZB_FACTORY_RESET_HOLD_MS = 10000;
static constexpr uint32_t STATUS_LED_BOOT_FLASH_MS = 80;
static constexpr uint32_t STATUS_LED_STARTUP_VISIBLE_MS = 3000;
static constexpr uint32_t STATUS_LED_START_BLINK_MS = 700;
static constexpr uint32_t STATUS_LED_PAIRING_BLINK_MS = 180;
static constexpr uint32_t STATUS_LED_ERROR_BLINK_MS = 250;

static constexpr size_t BLE_NAME_LEN = 48;

static constexpr uint8_t PIN_SPI_SCK = 20;
static constexpr uint8_t PIN_SPI_MISO = 19;
static constexpr uint8_t PIN_SPI_MOSI = 18;
static constexpr uint8_t PIN_PN532_SS = 14;

static constexpr uint8_t ZIGBEE_COVERING_ENDPOINT = 10;
static constexpr uint8_t BUTTON_PIN = 9;

static constexpr uint16_t NFC_TIMEOUT_MS = 50;
static constexpr uint32_t LOOP_DELAY_MS = 2;
static constexpr uint32_t TAG_LOST_MS = 50;
static constexpr uint8_t INDEX_CONFIRM_COUNT = 2;
static constexpr uint8_t TAG_UID_LENGTH = 7;
static constexpr uint8_t MAX_UID_LENGTH = 10;
static constexpr uint8_t TAG_COUNT = 10;
// ZigbeeWindowCovering and Zigbee2MQTT use 0 = closed and 100 = open.
// Keep the UID table ordered from closed -> open so the reported opening
// percentage matches the logical tag labels directly.
static constexpr bool INDEX_INCREASES_WHEN_OPENING = true;

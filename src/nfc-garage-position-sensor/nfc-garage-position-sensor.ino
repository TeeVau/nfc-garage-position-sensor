#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include <Arduino.h>
#include "config.h"
#include <SPI.h>
#include <cstring>
#include <Adafruit_PN532.h>
#if BLE_DEBUG_ENABLED
#include <NimBLEDevice.h>
#endif
#include "ZigbeeCore.h"
#include "ep/ZigbeeWindowCovering.h"

extern "C" {
#include "esp_zigbee_core.h"
#include "nwk/esp_zigbee_nwk.h"
#include "zdo/esp_zigbee_zdo_common.h"
}

Adafruit_PN532 nfc(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_PN532_SS);

// Espressif's documented Arduino ZigbeeEP API exposes setVersion() for the
// Basic Application Version attribute, but it does not expose a public setter
// for Basic.swBuildId. Zigbee2MQTT reads appVersion successfully, yet still
// keeps Firmware-ID unknown unless swBuildId is present. We therefore extend
// the endpoint locally so we can add swBuildId before Zigbee.begin().
class GarageZigbeeWindowCovering : public ZigbeeWindowCovering {
public:
  using ZigbeeWindowCovering::ZigbeeWindowCovering;

  bool setSoftwareBuildId(const char* buildId);
  bool reportLiftPercentage();
};

GarageZigbeeWindowCovering zbCovering(ZIGBEE_COVERING_ENDPOINT);

#if BLE_DEBUG_ENABLED
char bleDeviceName[BLE_NAME_LEN] = {0};

NimBLEServer* pServer = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;

bool bleClientConnected = false;
#endif
bool zigbeeReady = false;
bool zigbeeStackStarted = false;

uint8_t lastSeenUid[MAX_UID_LENGTH] = {0};
uint8_t lastSeenUidLength = 0;
uint32_t lastSeenAtMs = 0;

int8_t stableIndex = -1;
int8_t pendingIndex = -1;
uint8_t pendingCount = 0;

uint8_t currentOpeningPercentage = 100;
uint32_t lastZigbeeStatusMs = 0;

void setupNfc();
void pollNfc();
void setupBleUart();
void setupZigbee();
void pollZigbee();
void pollButton();
void printRuntimeState(const char* source);
void publishCurrentPosition(const char* source);
void setupStatusLed();
void pollStatusLed();
void setStatusLedError();
void showStatusLedStartupWindow();

void setup() {
  char msg[48];

  Serial.begin(115200);
  delay(300);

  snprintf(msg, sizeof(msg), "%s %s", PROJECT_NAME, SOFTWARE_VERSION);
  Serial.println();
  Serial.println(msg);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setupStatusLed();
  showStatusLedStartupWindow();

  setupNfc();
  setupZigbee();
  setupBleUart();
}

void loop() {
  pollButton();
  pollNfc();
  pollZigbee();
  pollStatusLed();
  delay(LOOP_DELAY_MS);
}

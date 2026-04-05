#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include <Arduino.h>
#include <SPI.h>
#include <cstring>
#include <Adafruit_PN532.h>
#include <NimBLEDevice.h>
#include "ZigbeeCore.h"
#include "ep/ZigbeeWindowCovering.h"
#include "config.h"

extern "C" {
#include "esp_zigbee_core.h"
#include "nwk/esp_zigbee_nwk.h"
#include "zdo/esp_zigbee_zdo_common.h"
}

Adafruit_PN532 nfc(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_PN532_SS);
ZigbeeWindowCovering zbCovering(ZIGBEE_COVERING_ENDPOINT);

char bleDeviceName[BLE_NAME_LEN] = {0};

NimBLEServer* pServer = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;

bool bleClientConnected = false;
bool zigbeeReady = false;

uint8_t lastSeenUid[MAX_UID_LENGTH] = {0};
uint8_t lastSeenUidLength = 0;
uint32_t lastSeenAtMs = 0;

int8_t stableIndex = -1;
int8_t pendingIndex = -1;
uint8_t pendingCount = 0;
uint32_t lastIndexChangeMs = 0;

uint8_t currentLiftPercentage = 100;
uint32_t lastZigbeeStatusMs = 0;
Direction direction = DIR_UNKNOWN;

void setupNfc();
void pollNfc();
void setupBleUart();
void setupZigbee();
void pollZigbee();
void pollButton();

void setup() {
  char msg[48];

  Serial.begin(115200);
  delay(300);

  snprintf(msg, sizeof(msg), "%s %s", PROJECT_NAME, SOFTWARE_VERSION);
  Serial.println();
  Serial.println(msg);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setupNfc();
  setupZigbee();
  setupBleUart();
}

void loop() {
  pollButton();
  pollNfc();
  pollZigbee();
  delay(LOOP_DELAY_MS);
}

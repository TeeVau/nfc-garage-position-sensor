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

extern "C" {
#include "esp_zigbee_core.h"
#include "nwk/esp_zigbee_nwk.h"
#include "zdo/esp_zigbee_zdo_common.h"
}

static constexpr const char* PROJECT_NAME = "nfc-garage-position-sensor";
static constexpr const char* SOFTWARE_VERSION = "v0.2.1";
static constexpr const char* ZB_MFR = "TeeVau";
static constexpr const char* ZB_MODEL = "nfc-garage-position-sensor";
static constexpr uint32_t ZB_STATUS_INTERVAL_MS = 30000;

static constexpr size_t BLE_NAME_LEN = 48;
char bleDeviceName[BLE_NAME_LEN] = {0};

// SPI
static constexpr uint8_t PIN_SPI_SCK  = 20;
static constexpr uint8_t PIN_SPI_MISO = 19;
static constexpr uint8_t PIN_SPI_MOSI = 18;
static constexpr uint8_t PIN_PN532_SS = 14;

// HW
static constexpr uint8_t ZIGBEE_COVERING_ENDPOINT = 10;
static constexpr uint8_t BUTTON_PIN = 9;

// NFC
static constexpr uint16_t NFC_TIMEOUT_MS = 50;
static constexpr uint32_t LOOP_DELAY_MS  = 2;
static constexpr uint32_t TAG_LOST_MS    = 50;
static constexpr uint32_t STOP_DETECT_MS = 2000;
static constexpr uint8_t  INDEX_CONFIRM_COUNT = 2;
static constexpr uint8_t  TAG_UID_LENGTH = 7;
static constexpr uint8_t  MAX_UID_LENGTH = 10;

Adafruit_PN532 nfc(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_PN532_SS);
ZigbeeWindowCovering zbCovering(ZIGBEE_COVERING_ENDPOINT);

static constexpr uint8_t TAGS[][TAG_UID_LENGTH] = {
  {0x04, 0x29, 0xCB, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x61, 0xC2, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0xAD, 0xBC, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x2D, 0xB7, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0xDC, 0xAE, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x21, 0xA7, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x2F, 0xA3, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x11, 0x9E, 0x3E, 0xD4, 0x2A, 0x81},
  {0x04, 0x96, 0x9A, 0x3E, 0xD4, 0x2A, 0x81}
};

static constexpr size_t TAG_COUNT = sizeof(TAGS) / sizeof(TAGS[0]);

// BLE NUS
static NimBLEUUID NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID NUS_TX_UUID     ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

NimBLEServer*         pServer           = nullptr;
NimBLEService*        pService          = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;

bool bleClientConnected = false;

uint8_t lastSeenUid[MAX_UID_LENGTH] = {0};
uint8_t lastSeenUidLength = 0;
uint32_t lastSeenAtMs = 0;

int8_t stableIndex = -1;
int8_t pendingIndex = -1;
uint8_t pendingCount = 0;
uint32_t lastIndexChangeMs = 0;
bool zigbeeReady = false;

uint8_t currentLiftPercentage = 100;
uint32_t lastZigbeeStatusMs = 0;

enum Direction : int8_t {
  DIR_UNKNOWN = 0,
  DIR_OPENING = 1,
  DIR_CLOSING = -1,
  DIR_STOPPED = 2
};

Direction direction = DIR_UNKNOWN;

void fullOpen();
void fullClose();
void goToLiftPercentage(uint8_t liftPercentage);
void stopMotor();
void handleShortButtonPress();
void updateDetectedIndex(int8_t newIndex);
void updateStoppedState(uint32_t now);
void publishCurrentPosition(const char* source);
void printZigbeeStatus();

void logLine(const char* msg) {
  Serial.println(msg);

  if (bleClientConnected && pTxCharacteristic != nullptr) {
    pTxCharacteristic->setValue(msg);
    pTxCharacteristic->notify();
  }
}

bool sameUid(const uint8_t* uidA, uint8_t lenA, const uint8_t* uidB, uint8_t lenB) {
  return lenA == lenB && memcmp(uidA, uidB, lenA) == 0;
}

void copyUid(uint8_t* dst, uint8_t* dstLen, const uint8_t* src, uint8_t srcLen) {
  uint8_t copyLen = srcLen;
  if (copyLen > MAX_UID_LENGTH) {
    copyLen = MAX_UID_LENGTH;
  }

  *dstLen = copyLen;
  memcpy(dst, src, copyLen);
}

void formatUid(const uint8_t* uid, uint8_t uidLength, char* out, size_t outSize) {
  size_t pos = 0;
  out[0] = '\0';

  for (uint8_t i = 0; i < uidLength && pos + 3 < outSize; i++) {
    int written = snprintf(out + pos, outSize - pos, (i == 0) ? "%02X" : "-%02X", uid[i]);
    if (written <= 0) {
      break;
    }
    pos += static_cast<size_t>(written);
    if (pos >= outSize) {
      break;
    }
  }
}

int8_t findIndex(const uint8_t* uid, uint8_t uidLength) {
  if (uidLength != TAG_UID_LENGTH) {
    return -1;
  }

  for (size_t i = 0; i < TAG_COUNT; i++) {
    if (memcmp(uid, TAGS[i], TAG_UID_LENGTH) == 0) {
      return static_cast<int8_t>(i);
    }
  }

  return -1;
}

const char* dirCode(Direction dir) {
  switch (dir) {
    case DIR_OPENING: return "up";
    case DIR_CLOSING: return "dn";
    case DIR_STOPPED: return "stp";
    default:          return "?";
  }
}

uint8_t indexToPercent(int8_t index) {
  if (index < 0 || TAG_COUNT <= 1) {
    return 0;
  }

  return static_cast<uint8_t>((index * 100) / (TAG_COUNT - 1));
}

void publishCurrentPosition(const char* source) {
  char msg[48];

  if (stableIndex < 0) {
    snprintf(msg, sizeof(msg), "ZB pos ? %s", source);
    logLine(msg);
    return;
  }

  currentLiftPercentage = indexToPercent(stableIndex);
  zbCovering.setLiftPercentage(currentLiftPercentage);

  snprintf(msg, sizeof(msg), "ZB pos %u %s", currentLiftPercentage, source);
  logLine(msg);
}

void logTagState(const char* source) {
  char msg[48];
  snprintf(msg, sizeof(msg), "TAG %d %u %s %s", stableIndex, indexToPercent(stableIndex), dirCode(direction), source);
  logLine(msg);
}

const char* roleCode(esp_zb_nwk_device_type_t role) {
  switch (role) {
    case ESP_ZB_DEVICE_TYPE_COORDINATOR:
      return "zc";
    case ESP_ZB_DEVICE_TYPE_ROUTER:
      return "zr";
    case ESP_ZB_DEVICE_TYPE_ED:
      return "zed";
    default:
      return "?";
  }
}

void printIeeeLine(const char* label, const uint8_t* addr) {
  char msg[64];
  int len = snprintf(msg, sizeof(msg), "%s ", label);

  for (int i = 7; i >= 0 && len > 0 && len < static_cast<int>(sizeof(msg) - 3); i--) {
    len += snprintf(msg + len, sizeof(msg) - len, (i == 7) ? "%02X" : ":%02X", addr[i]);
  }

  logLine(msg);
}

void printNeighborTable() {
  esp_zb_nwk_info_iterator_t it = ESP_ZB_NWK_INFO_ITERATOR_INIT;
  esp_zb_nwk_neighbor_info_t nbr;
  bool found = false;
  char msg[96];

  logLine("ZB nbr");

  while (esp_zb_nwk_get_next_neighbor(&it, &nbr) == ESP_OK) {
    found = true;
    snprintf(
      msg,
      sizeof(msg),
      "N 0x%04X lqi=%u rssi=%d d=%u rel=%u rx=%u",
      nbr.short_addr,
      nbr.lqi,
      nbr.rssi,
      nbr.depth,
      nbr.relationship,
      nbr.rx_on_when_idle
    );
    logLine(msg);
  }

  if (!found) {
    logLine("N none");
  }
}

void printRouteTable() {
  esp_zb_nwk_info_iterator_t it = ESP_ZB_NWK_INFO_ITERATOR_INIT;
  esp_zb_nwk_route_info_t route;
  bool found = false;
  char msg[64];

  logLine("ZB route");

  while (esp_zb_nwk_get_next_route(&it, &route) == ESP_OK) {
    found = true;
    snprintf(msg, sizeof(msg), "R dst=0x%04X via=0x%04X st=%u", route.dest_addr, route.next_hop_addr, route.flags.status);
    logLine(msg);
  }

  if (!found) {
    logLine("R none");
  }
}

void printZigbeeStatus() {
  char msg[64];
  esp_zb_ieee_addr_t ieee = {0};
  esp_zb_ieee_addr_t extPan = {0};

  esp_zb_get_long_address(ieee);
  esp_zb_get_extended_pan_id(extPan);

  snprintf(
    msg,
    sizeof(msg),
    "ZB st role=%s ch=%u pan=0x%04X short=0x%04X conn=%u",
    roleCode(esp_zb_get_network_device_role()),
    esp_zb_get_current_channel(),
    esp_zb_get_pan_id(),
    esp_zb_get_short_address(),
    Zigbee.connected() ? 1 : 0
  );
  logLine(msg);

  printIeeeLine("ZB ieee", ieee);
  printIeeeLine("ZB xpan", extPan);
  printNeighborTable();
  printRouteTable();
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleClientConnected = true;
    logLine("BLE conn");
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    bleClientConnected = false;
    Serial.println("BLE disc");
    NimBLEDevice::startAdvertising();
  }
};


void setupBleUart() {
  snprintf(bleDeviceName, sizeof(bleDeviceName), "%s-%s", PROJECT_NAME, SOFTWARE_VERSION);
  NimBLEDevice::init(bleDeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(NUS_SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    NUS_TX_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  pTxCharacteristic->createDescriptor("2902");

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->setName(bleDeviceName);
  pAdvertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  logLine("BLE adv");
}

void setupZigbee() {
  char msg[48];

  bool ok = zbCovering.setManufacturerAndModel(ZB_MFR, ZB_MODEL);
  snprintf(msg, sizeof(msg), "ZB mm %u", ok ? 1 : 0);
  logLine(msg);

  zbCovering.setCoveringType(ROLLERSHADE);
  zbCovering.setConfigStatus(true, true, false, true, true, false, false);
  zbCovering.setMode(false, true, false, false);
  zbCovering.setLimits(0, 100, 0, 0);

  Zigbee.addEndpoint(&zbCovering);
  logLine("ZB ep ok");

  Zigbee.setDebugMode(true);
  logLine("ZB dbg on");

  if (!Zigbee.begin()) {
    logLine("ERR zb begin");
    delay(1000);
    ESP.restart();
  }

  logLine("ZB join...");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void updateDetectedIndex(int8_t newIndex) {
  uint32_t now = millis();

  if (stableIndex < 0) {
    stableIndex = newIndex;
    pendingIndex = -1;
    pendingCount = 0;
    direction = DIR_UNKNOWN;
    lastIndexChangeMs = now;
    logTagState("boot");
    publishCurrentPosition("nfc0");
    return;
  }

  if (newIndex == stableIndex) {
    pendingIndex = -1;
    pendingCount = 0;
    return;
  }

  if (newIndex != pendingIndex) {
    pendingIndex = newIndex;
    pendingCount = 1;
    return;
  }

  if (pendingCount < 255) {
    pendingCount++;
  }

  if (pendingCount < INDEX_CONFIRM_COUNT) {
    return;
  }

  int8_t oldStable = stableIndex;
  stableIndex = newIndex;
  pendingIndex = -1;
  pendingCount = 0;
  lastIndexChangeMs = now;

  if (stableIndex > oldStable) {
    direction = DIR_OPENING;
  } else if (stableIndex < oldStable) {
    direction = DIR_CLOSING;
  } else {
    direction = DIR_STOPPED;
  }

  logTagState("nfc");
  publishCurrentPosition("nfc");
}

void updateStoppedState(uint32_t now) {
  if (stableIndex < 0) {
    return;
  }

  if (direction != DIR_OPENING && direction != DIR_CLOSING) {
    return;
  }

  if ((now - lastIndexChangeMs) < STOP_DETECT_MS) {
    return;
  }

  direction = DIR_STOPPED;
  logTagState("hold");
  publishCurrentPosition("hold");
}

void fullOpen() {
  direction = DIR_OPENING;
  stableIndex = static_cast<int8_t>(TAG_COUNT - 1);
  lastIndexChangeMs = millis();
  logLine("ZB cmd open");
  publishCurrentPosition("open");
}

void fullClose() {
  direction = DIR_CLOSING;
  stableIndex = 0;
  lastIndexChangeMs = millis();
  logLine("ZB cmd close");
  publishCurrentPosition("close");
}

void goToLiftPercentage(uint8_t liftPercentage) {
  char msg[32];

  currentLiftPercentage = liftPercentage;
  stableIndex = static_cast<int8_t>((liftPercentage * (TAG_COUNT - 1)) / 100);
  direction = DIR_STOPPED;
  lastIndexChangeMs = millis();

  snprintf(msg, sizeof(msg), "ZB cmd %u", liftPercentage);
  logLine(msg);
  publishCurrentPosition("goto");
}

void stopMotor() {
  direction = DIR_STOPPED;
  lastIndexChangeMs = millis();
  logLine("ZB cmd stop");
  publishCurrentPosition("stop");
}

void handleShortButtonPress() {
  logLine("BTN short");
  printZigbeeStatus();
}

void setup() {
  char msg[48];

  Serial.begin(115200);
  delay(300);

  snprintf(msg, sizeof(msg), "%s %s", PROJECT_NAME, SOFTWARE_VERSION);
  Serial.println();
  Serial.println(msg);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    logLine("ERR pn532");
    while (true) {
      delay(1000);
    }
  }

  snprintf(msg, sizeof(msg), "PN532 0x%08lX", static_cast<unsigned long>(versiondata));
  logLine(msg);

  nfc.SAMConfig();
  logLine("NFC wait");

  setupZigbee();
  setupBleUart();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(100);
    uint32_t startTime = millis();
    bool factoryResetTriggered = false;

    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        logLine("ZB reset");
        Zigbee.factoryReset();
        factoryResetTriggered = true;
        delay(30000);
        break;
      }
    }

    if (!factoryResetTriggered) {
      handleShortButtonPress();
    }
  }

  uint8_t uid[MAX_UID_LENGTH] = {0};
  uint8_t uidLength = 0;

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    NFC_TIMEOUT_MS
  );

  if (success) {
    uint32_t now = millis();
    int8_t index = findIndex(uid, uidLength);

    if (index >= 0) {
      updateDetectedIndex(index);
    } else if (!sameUid(uid, uidLength, lastSeenUid, lastSeenUidLength)) {
        char uidText[3 * MAX_UID_LENGTH];
        char msg[40];
        formatUid(uid, uidLength, uidText, sizeof(uidText));
        snprintf(msg, sizeof(msg), "TAG ? %s", uidText);
        logLine(msg);
    }

    copyUid(lastSeenUid, &lastSeenUidLength, uid, uidLength);
    lastSeenAtMs = now;
  } else if (lastSeenUidLength > 0 && (millis() - lastSeenAtMs > TAG_LOST_MS)) {
    lastSeenUidLength = 0;
  }

  updateStoppedState(millis());

  if (Zigbee.started()) {
    uint32_t now = millis();
    bool connected = Zigbee.connected();

    if (connected && !zigbeeReady) {
      zigbeeReady = true;
      Serial.println();
      logLine("ZB up");
      printZigbeeStatus();
      publishCurrentPosition("boot");
    } else if (!connected && zigbeeReady) {
      zigbeeReady = false;
      logLine("ZB down");
    }

    if (now - lastZigbeeStatusMs >= ZB_STATUS_INTERVAL_MS) {
      lastZigbeeStatusMs = now;
      printZigbeeStatus();
    } else if (!connected) {
      Serial.print(".");
    }
  }

  delay(LOOP_DELAY_MS);
}

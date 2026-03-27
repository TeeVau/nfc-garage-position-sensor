#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_PN532.h>
#include <NimBLEDevice.h>
#include "secrets.h"

static constexpr const char* PROJECT_NAME = "nfc-garage-position-sensor";
static constexpr const char* SOFTWARE_VERSION = "v0.1.0";
char bleDeviceName[64] = {0};

// =========================
// WLAN in secrets.h anpassen
// =========================
static constexpr uint32_t WIFI_LOG_INTERVAL_MS = 5000;
uint32_t lastWifiLogMs = 0;

// =========================
// SPI-Pins an dein Board anpassen
// =========================
static constexpr uint8_t PIN_SPI_SCK  = 4;   // Platzhalter
static constexpr uint8_t PIN_SPI_MISO = 5;   // Platzhalter
static constexpr uint8_t PIN_SPI_MOSI = 6;   // Platzhalter
static constexpr uint8_t PIN_PN532_SS = 7;   // Platzhalter

// =========================
// NFC-Parameter
// =========================
static constexpr uint16_t NFC_TIMEOUT_MS = 50;   // kurze Blockierzeit
static constexpr uint32_t LOOP_DELAY_MS  = 2;    // kleine Entlastung der CPU
static constexpr uint32_t TAG_LOST_MS    = 120;  // nach Tagverlust wieder "neu" erkennbar

// Richtungs-/Positionsstabilisierung
static constexpr uint8_t  INDEX_CONFIRM_COUNT = 2;
static constexpr uint32_t INDEX_CONFIRM_MS    = 80;

Adafruit_PN532 nfc(PIN_PN532_SS);

// =========================
// UID Liste (Reihenfolge = Position)
// =========================
static const char* TAGS[] = {
  "04-29-CB-3E-D4-2A-81",
  "04-61-C2-3E-D4-2A-81",
  "04-AD-BC-3E-D4-2A-81",
  "04-2D-B7-3E-D4-2A-81",
  "04-DC-AE-3E-D4-2A-81",
  "04-21-A7-3E-D4-2A-81",
  "04-2F-A3-3E-D4-2A-81",
  "04-11-9E-3E-D4-2A-81",
  "04-96-9A-3E-D4-2A-81"
};

static constexpr size_t TAG_COUNT = sizeof(TAGS) / sizeof(TAGS[0]);

// =========================
// BLE NUS UUIDs
// =========================
static NimBLEUUID NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID NUS_RX_UUID     ("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // iPhone -> ESP
static NimBLEUUID NUS_TX_UUID     ("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // ESP -> iPhone

NimBLEServer*         pServer           = nullptr;
NimBLEService*        pService          = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;

bool bleClientConnected = false;

// =========================
// Status
// =========================
String lastSeenUid   = "";
uint32_t lastSeenAtMs = 0;
bool tagPresent = false;

int stableIndex = -1;
int candidateIndex = -1;
uint8_t candidateCount = 0;
uint32_t candidateFirstSeenMs = 0;

enum Direction : int8_t {
  DIR_UNKNOWN = 0,
  DIR_OPENING = 1,
  DIR_CLOSING = -1,
  DIR_STOPPED = 2
};

Direction direction = DIR_UNKNOWN;

// =========================
// Helper
// =========================
String wifiQuality(int rssi) {
  if (rssi >= -60) return "sehr gut";
  if (rssi >= -70) return "gut";
  if (rssi >= -80) return "kritisch";
  return "schlecht";
}

String uidToString(const uint8_t* uid, uint8_t uidLength) {
  String out;
  out.reserve(uidLength * 3);

  for (uint8_t i = 0; i < uidLength; i++) {
    if (i > 0) out += "-";
    if (uid[i] < 0x10) out += "0";
    out += String(uid[i], HEX);
  }

  out.toUpperCase();
  return out;
}

int findIndex(const String& uid) {
  for (size_t i = 0; i < TAG_COUNT; i++) {
    if (uid.equals(TAGS[i])) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

const char* directionToText(Direction dir) {
  switch (dir) {
    case DIR_OPENING: return "oeffnet";
    case DIR_CLOSING: return "schliesst";
    case DIR_STOPPED: return "steht";
    default:          return "unbekannt";
  }
}

uint8_t indexToPercent(int index) {
  if (index < 0) return 0;
  return static_cast<uint8_t>((index * 100) / (TAG_COUNT - 1));
}

void logLine(const String& msg) {
  Serial.println(msg);

  if (bleClientConnected && pTxCharacteristic != nullptr) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[WIFI] Verbinde...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  logLine("[WIFI] Verbunden");
  logLine("[WIFI] IP: " + WiFi.localIP().toString());
}

// =========================
// BLE Callbacks
// =========================
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleClientConnected = true;
    Serial.println("[BLE] Client verbunden");
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    bleClientConnected = false;
    Serial.println("[BLE] Client getrennt");
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
      String rx = "[BLE RX] ";
      for (char c : value) rx += c;
      Serial.println(rx);
    }
  }
};

void setupBleUart() {
  snprintf(bleDeviceName, sizeof(bleDeviceName), "%s-%s", PROJECT_NAME, SOFTWARE_VERSION);
  NimBLEDevice::init(bleDeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // starke Sendeleistung, falls verfügbar

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(NUS_SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    NUS_TX_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  pTxCharacteristic->createDescriptor("2902");

  pRxCharacteristic = pService->createCharacteristic(
    NUS_RX_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(NUS_SERVICE_UUID);
  pAdvertising->setName(bleDeviceName);
  pAdvertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising gestartet");
}

// =========================
// Stabile Positionslogik
// =========================
void processDetectedIndex(int newIndex) {
  uint32_t now = millis();

  if (stableIndex < 0) {
    stableIndex = newIndex;
    direction = DIR_UNKNOWN;

    logLine("[TAG] STABLE Index: " + String(stableIndex) +
            " | " + String(indexToPercent(stableIndex)) + " % | " +
            directionToText(direction));
    return;
  }

  // gleicher stabiler Index -> Kandidat zurücksetzen
  if (newIndex == stableIndex) {
    candidateIndex = -1;
    candidateCount = 0;
    candidateFirstSeenMs = 0;
    return;
  }

  // Kandidat neu beginnen oder fortführen
  if (newIndex != candidateIndex) {
    candidateIndex = newIndex;
    candidateCount = 1;
    candidateFirstSeenMs = now;
    return;
  } else {
    candidateCount++;
  }

  // Kandidat erst übernehmen, wenn ausreichend bestätigt
  bool candidateConfirmed =
      (candidateCount >= INDEX_CONFIRM_COUNT) ||
      ((now - candidateFirstSeenMs) >= INDEX_CONFIRM_MS);

  if (!candidateConfirmed) {
    return;
  }

  int delta = candidateIndex - stableIndex;

  // Einzelschritt-Gegenimpulse bei aktiver Richtung ignorieren
  if (direction == DIR_OPENING && delta == -1) {
    return;
  }

  if (direction == DIR_CLOSING && delta == +1) {
    return;
  }

  int oldStable = stableIndex;
  stableIndex = candidateIndex;

  if (stableIndex > oldStable) {
    direction = DIR_OPENING;
  } else if (stableIndex < oldStable) {
    direction = DIR_CLOSING;
  } else {
    direction = DIR_STOPPED;
  }

  logLine("[TAG] STABLE Index: " + String(stableIndex) +
          " | " + String(indexToPercent(stableIndex)) + " % | " +
          directionToText(direction));

  candidateIndex = -1;
  candidateCount = 0;
  candidateFirstSeenMs = 0;
}

void logWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();

    logLine("[WIFI] RSSI: " + String(rssi) + " dBm | " + wifiQuality(rssi));
  } else {
    logLine("[WIFI] NICHT verbunden");
  }
}

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("==== " + String(PROJECT_NAME) + " ====");
  Serial.println("Version: " + String(SOFTWARE_VERSION));

  setupBleUart();
  connectWifi();

  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_PN532_SS);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    logLine("[FEHLER] Kein PN532 gefunden");
    while (true) delay(1000);
  }

  logLine("[OK] PN532 Firmware: 0x" + String(versiondata, HEX));

  nfc.SAMConfig();
  logLine("[OK] Warte auf Tags...");
}

// =========================
// Loop
// =========================
void loop() {
  uint8_t uid[7] = {0};
  uint8_t uidLength = 0;

  bool success = nfc.readPassiveTargetID(
    PN532_MIFARE_ISO14443A,
    uid,
    &uidLength,
    NFC_TIMEOUT_MS
  );

  if (success) {
    String uidStr = uidToString(uid, uidLength);
    uint32_t now = millis();

    // Nur neue UID weiterverarbeiten
    if (!tagPresent || uidStr != lastSeenUid) {
      int index = findIndex(uidStr);

      if (index >= 0) {
        processDetectedIndex(index);
      } else {
        logLine("[TAG] " + uidStr + " | unbekannt");
      }

      lastSeenUid = uidStr;
    }

    tagPresent = true;
    lastSeenAtMs = now;

  } else {
    if (tagPresent && (millis() - lastSeenAtMs > TAG_LOST_MS)) {
      tagPresent = false;
      lastSeenUid = "";
    }
  }

  delay(LOOP_DELAY_MS);
  uint32_t now = millis();

if (now - lastWifiLogMs >= WIFI_LOG_INTERVAL_MS) {
  lastWifiLogMs = now;
  logWifiStatus();
}
}

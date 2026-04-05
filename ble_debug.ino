static NimBLEUUID NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID NUS_TX_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

void logLine(const char* msg) {
  Serial.println(msg);

  if (bleClientConnected && pTxCharacteristic != nullptr) {
    pTxCharacteristic->setValue(msg);
    pTxCharacteristic->notify();
  }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    bleClientConnected = true;
    logLine("BLE conn");
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
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

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(NUS_SERVICE_UUID);
  advertising->setName(bleDeviceName);
  advertising->enableScanResponse(true);
  NimBLEDevice::startAdvertising();

  logLine("BLE adv");
}

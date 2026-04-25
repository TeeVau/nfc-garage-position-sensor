#if BLE_DEBUG_ENABLED
static NimBLEUUID NUS_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID NUS_RX_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
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
    Serial.println("BLE conn");
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    bleClientConnected = false;
    Serial.println("BLE disc");

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (advertising == nullptr || !advertising->start()) {
      Serial.println("ERR BLE adv restart");
    }
  }
};

void setupBleUart() {
  snprintf(bleDeviceName, sizeof(bleDeviceName), "%s", BLE_DEVICE_NAME);
  if (!NimBLEDevice::init(bleDeviceName)) {
    logLine("ERR BLE init");
    return;
  }

  if (!NimBLEDevice::setDeviceName(bleDeviceName)) {
    logLine("ERR BLE name");
  }

  NimBLEDevice::setPower(ESP_PWR_LVL_P20);

  pServer = NimBLEDevice::createServer();
  if (pServer == nullptr) {
    logLine("ERR BLE srv");
    return;
  }
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(NUS_SERVICE_UUID);
  if (pService == nullptr) {
    logLine("ERR BLE svc");
    return;
  }

  pRxCharacteristic = pService->createCharacteristic(
    NUS_RX_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pTxCharacteristic = pService->createCharacteristic(
    NUS_TX_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  if (pRxCharacteristic == nullptr || pTxCharacteristic == nullptr) {
    logLine("ERR BLE chr");
    return;
  }

  pRxCharacteristic->setValue("");
  pTxCharacteristic->createDescriptor("2902");

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  if (advertising == nullptr) {
    logLine("ERR BLE adv");
    return;
  }

  advertising->addServiceUUID(NUS_SERVICE_UUID);
  if (!advertising->setName(bleDeviceName)) {
    logLine("ERR BLE adv name");
  }
  advertising->enableScanResponse(true);
  if (!advertising->start()) {
    logLine("ERR BLE adv start");
    return;
  }

  logLine("BLE adv");
}
#else
void logLine(const char* msg) {
  Serial.println(msg);
}

void setupBleUart() {
  logLine("BLE off");
}
#endif

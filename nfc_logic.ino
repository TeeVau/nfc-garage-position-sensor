void logTagState(const char* source) {
  char msg[48];
  snprintf(msg, sizeof(msg), "TAG %d %u %s %s", stableIndex, indexToPercent(stableIndex), dirCode(direction), source);
  logLine(msg);
}

void setupNfc() {
  char msg[48];

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

void pollNfc() {
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
}

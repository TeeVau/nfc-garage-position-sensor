const uint8_t TAGS[TAG_COUNT][TAG_UID_LENGTH] = {
  // Ordered from 0 % opening (closed) to 100 % opening (open).
  {0x04, 0x29, 0xCB, 0x3E, 0xD4, 0x2A, 0x81},  // 0 %
  {0x04, 0x61, 0xC2, 0x3E, 0xD4, 0x2A, 0x81},  // 12 %
  {0x04, 0xAD, 0xBC, 0x3E, 0xD4, 0x2A, 0x81},  // 25 %
  {0x04, 0x2D, 0xB7, 0x3E, 0xD4, 0x2A, 0x81},  // 37 %
  {0x04, 0xDC, 0xAE, 0x3E, 0xD4, 0x2A, 0x81},  // 50 %
  {0x04, 0x21, 0xA7, 0x3E, 0xD4, 0x2A, 0x81},  // 62 %
  {0x04, 0x2F, 0xA3, 0x3E, 0xD4, 0x2A, 0x81},  // 75 %
  {0x04, 0x11, 0x9E, 0x3E, 0xD4, 0x2A, 0x81},  // 87 %
  {0x04, 0x96, 0x9A, 0x3E, 0xD4, 0x2A, 0x81}   // 100 %
};

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

  for (uint8_t i = 0; i < TAG_COUNT; i++) {
    if (memcmp(uid, TAGS[i], TAG_UID_LENGTH) == 0) {
      return static_cast<int8_t>(i);
    }
  }

  return -1;
}

const char* dirCode(Direction dir) {
  switch (dir) {
    case DIR_OPENING: return "opening";
    case DIR_CLOSING: return "closing";
    case DIR_STOPPED: return "stopped";
    default: return "unknown";
  }
}

uint16_t directionToZigbeeValue(Direction dir) {
  switch (dir) {
    case DIR_OPENING: return 1;
    case DIR_CLOSING: return 2;
    case DIR_STOPPED: return 3;
    default: return 0;
  }
}

uint8_t indexToPercent(int8_t index) {
  if (index < 0 || TAG_COUNT <= 1) {
    return 0;
  }

  uint8_t rawPercent = static_cast<uint8_t>((index * 100) / (TAG_COUNT - 1));
  return INDEX_INCREASES_WHEN_OPENING ? rawPercent : static_cast<uint8_t>(100 - rawPercent);
}

uint8_t openingPercentToZigbeeLiftPercent(uint8_t openingPercent) {
  if (openingPercent > 100) {
    openingPercent = 100;
  }

  // The Zigbee Window Covering cluster reports lift percentage with inverse
  // semantics compared to our opening percentage: 0 = open, 100 = closed.
  return static_cast<uint8_t>(100 - openingPercent);
}

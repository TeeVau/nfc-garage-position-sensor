const uint8_t TAGS[TAG_COUNT][TAG_UID_LENGTH] = {
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
    case DIR_OPENING: return "up";
    case DIR_CLOSING: return "dn";
    case DIR_STOPPED: return "stp";
    default: return "?";
  }
}

uint8_t indexToPercent(int8_t index) {
  if (index < 0 || TAG_COUNT <= 1) {
    return 0;
  }

  return static_cast<uint8_t>((index * 100) / (TAG_COUNT - 1));
}

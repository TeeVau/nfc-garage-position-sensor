void handleShortButtonPress() {
  logLine("BTN short");
  printZigbeeStatus();
}

void pollButton() {
  if (digitalRead(BUTTON_PIN) != LOW) {
    return;
  }

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

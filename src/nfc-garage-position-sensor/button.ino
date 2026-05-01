void handleShortButtonPress() {
  logLine("BTN short");
  printRuntimeState("btn");
  printZigbeeStatus();
  publishCurrentPosition("btn");
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
    if ((millis() - startTime) > ZB_FACTORY_RESET_HOLD_MS) {
      logLine("ZB reset");
      Zigbee.factoryReset();
      factoryResetTriggered = true;
      delay(30000);
      break;
    }
  }

  if (factoryResetTriggered) {
    return;
  }

  handleShortButtonPress();
}

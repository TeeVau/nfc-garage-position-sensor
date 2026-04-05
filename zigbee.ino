const char* roleCode(esp_zb_nwk_device_type_t role) {
  switch (role) {
    case ESP_ZB_DEVICE_TYPE_COORDINATOR: return "zc";
    case ESP_ZB_DEVICE_TYPE_ROUTER: return "zr";
    case ESP_ZB_DEVICE_TYPE_ED: return "zed";
    default: return "?";
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

void setupZigbee() {
  char msg[48];

  bool ok = zbCovering.setManufacturerAndModel(ZB_MFR, ZB_MODEL);
  snprintf(msg, sizeof(msg), "ZB mm %u", ok ? 1 : 0);
  logLine(msg);

  zbCovering.setCoveringType(ROLLERSHADE);
  /*
  operational - Operational status
  online - Online status
  commands_reversed - Commands reversed flag
  lift_closed_loop - Lift closed loop flag
  tilt_closed_loop - Tilt closed loop flag
  lift_encoder_controlled - Lift encoder controlled flag
  tilt_encoder_controlled - Tilt encoder controlled flag
  */
  zbCovering.setConfigStatus(true, true, false, true, false, false, false);
  
  /*
  motor_reversed - Motor reversed flag
  calibration_mode - Calibration mode flag
  maintenance_mode - Maintenance mode flag
  leds_on - LEDs on flag
*/
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
}

void pollZigbee() {
  if (!Zigbee.started()) {
    return;
  }

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

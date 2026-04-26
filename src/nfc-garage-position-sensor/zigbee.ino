static constexpr uint16_t ZB_BASIC_SW_BUILD_ID_ATTR = 0x4000;
static constexpr uint16_t ZB_COORDINATOR_SHORT_ADDR = 0x0000;
static constexpr uint8_t ZB_COORDINATOR_ENDPOINT = 1;

bool GarageZigbeeWindowCovering::setSoftwareBuildId(const char* buildId) {
  if (buildId == nullptr) {
    return false;
  }

  size_t buildIdLength = strlen(buildId);
  if (buildIdLength > ZB_MAX_NAME_LENGTH) {
    log_e("Software build ID is too long");
    return false;
  }

  char zbBuildId[ZB_MAX_NAME_LENGTH + 2];
  zbBuildId[0] = static_cast<char>(buildIdLength);
  memcpy(zbBuildId + 1, buildId, buildIdLength);
  zbBuildId[buildIdLength + 1] = '\0';

  esp_zb_attribute_list_t* basicCluster =
    esp_zb_cluster_list_get_cluster(_cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  if (basicCluster == nullptr) {
    log_e("Failed to get basic cluster for software build ID");
    return false;
  }

  esp_err_t ret = esp_zb_basic_cluster_add_attr(basicCluster, ZB_BASIC_SW_BUILD_ID_ATTR, (void*)zbBuildId);
  if (ret != ESP_OK) {
    log_e("Failed to add software build ID to basic cluster: 0x%x: %s", ret, esp_err_to_name(ret));
    return false;
  }

  return true;
}

bool GarageZigbeeWindowCovering::reportLiftPercentage() {
  esp_zb_zcl_report_attr_cmd_t report_attr_cmd;
  memset(&report_attr_cmd, 0, sizeof(report_attr_cmd));

  // Zigbee2MQTT can finish interview successfully yet still skip the converter
  // configure() step. In that case bindings/reporting remain empty, so we send
  // the report directly to the coordinator endpoint instead of relying on them.
  report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID;
  report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
  report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_WINDOW_COVERING;
  report_attr_cmd.zcl_basic_cmd.src_endpoint = _endpoint;
  report_attr_cmd.zcl_basic_cmd.dst_endpoint = ZB_COORDINATOR_ENDPOINT;
  report_attr_cmd.zcl_basic_cmd.dst_addr_u.addr_short = ZB_COORDINATOR_SHORT_ADDR;
  report_attr_cmd.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;

  esp_zb_lock_acquire(portMAX_DELAY);
  esp_err_t ret = esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
  esp_zb_lock_release();

  if (ret != ESP_OK) {
    log_e("Failed to report lift percentage: 0x%x: %s", ret, esp_err_to_name(ret));
    return false;
  }

  return true;
}

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

void printRuntimeState(const char* source) {
  char msg[64];

  snprintf(
    msg,
    sizeof(msg),
    "DBG %s up=%lus st=%u conn=%u rd=%u",
    source,
    millis() / 1000UL,
    Zigbee.started() ? 1 : 0,
    Zigbee.connected() ? 1 : 0,
    zigbeeReady ? 1 : 0
  );
  logLine(msg);

  snprintf(
    msg,
    sizeof(msg),
    "DBG idx=%d pend=%d cnt=%u open=%u",
    stableIndex,
    pendingIndex,
    pendingCount,
    currentOpeningPercentage
  );
  logLine(msg);

#if BLE_DEBUG_ENABLED
  snprintf(
    msg,
    sizeof(msg),
    "DBG ble cli=%u adv=%s",
    bleClientConnected ? 1 : 0,
    bleDeviceName
  );
  logLine(msg);
#endif
}

void publishCurrentPosition(const char* source) {
  char msg[64];

  if (!zigbeeReady) {
    snprintf(msg, sizeof(msg), "ZB skip %s", source);
    logLine(msg);
    return;
  }

  if (stableIndex < 0) {
    snprintf(msg, sizeof(msg), "ZB open=? lift=? %s", source);
    logLine(msg);
    return;
  }

  currentOpeningPercentage = indexToPercent(stableIndex);
  uint8_t zigbeeLiftPercent = openingPercentToZigbeeLiftPercent(currentOpeningPercentage);
  bool setOk = zbCovering.setLiftPercentage(zigbeeLiftPercent);
  bool reportOk = setOk && zbCovering.reportLiftPercentage();

  snprintf(
    msg,
    sizeof(msg),
    "ZB open=%u lift=%u %s set=%u rpt=%u",
    currentOpeningPercentage,
    zigbeeLiftPercent,
    source,
    setOk ? 1 : 0,
    reportOk ? 1 : 0
  );
  logLine(msg);
}

void setupZigbee() {
  char msg[64];

  bool ok = zbCovering.setManufacturerAndModel(ZB_MFR, ZB_MODEL);
  snprintf(msg, sizeof(msg), "ZB mm %u", ok ? 1 : 0);
  logLine(msg);

  zbCovering.setVersion(SOFTWARE_APPLICATION_VERSION);
  logLine("ZB appv ok");

  // setVersion() only fills Basic.appVersion. Zigbee2MQTT still requests
  // Basic.swBuildId separately for Firmware-ID, so we publish both.
  ok = zbCovering.setSoftwareBuildId(SOFTWARE_VERSION);
  snprintf(msg, sizeof(msg), "ZB swid %u", ok ? 1 : 0);
  logLine(msg);

  ok = zbCovering.setPowerSource(ZB_POWER_SOURCE_MAINS);
  snprintf(msg, sizeof(msg), "ZB pwr %u", ok ? 1 : 0);
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

  Zigbee.setPrimaryChannelMask(ZB_PRIMARY_CHANNEL_MASK);
  Zigbee.setTimeout(ZB_JOIN_TIMEOUT_MS);
  snprintf(msg, sizeof(msg), "ZB cfg mask=0x%08lX timeout=%lus", ZB_PRIMARY_CHANNEL_MASK, ZB_JOIN_TIMEOUT_MS / 1000UL);
  logLine(msg);

  if (!Zigbee.begin()) {
    logLine("ERR zb begin timeout");
    logLine("ZB offline");
    setStatusLedError();
    return;
  }

  logLine("ZB join...");
}

void pollZigbee() {
  uint32_t now = millis();

  if (!Zigbee.started()) {
    zigbeeStackStarted = false;
    if (now - lastZigbeeStatusMs >= ZB_STATUS_INTERVAL_MS) {
      lastZigbeeStatusMs = now;
      logLine("ZB wait start");
      printRuntimeState("wait");
    }
    return;
  }

  if (!zigbeeStackStarted) {
    zigbeeStackStarted = true;
    logLine("ZB started");
    printRuntimeState("start");
  }

  bool connected = Zigbee.connected();

  if (connected && !zigbeeReady) {
    zigbeeReady = true;
    Serial.println();
    logLine("ZB up");
    printZigbeeStatus();
    printRuntimeState("up");
    publishCurrentPosition("boot");
  } else if (!connected && zigbeeReady) {
    zigbeeReady = false;
    logLine("ZB down");
    printRuntimeState("down");
  }

  if (now - lastZigbeeStatusMs >= ZB_STATUS_INTERVAL_MS) {
    lastZigbeeStatusMs = now;
    printZigbeeStatus();
    printRuntimeState("tick");
  }
}

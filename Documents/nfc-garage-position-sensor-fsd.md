# NFC Garage Position Sensor - Functional Specification Document (FSD)

## 1. System Overview

### Purpose

The NFC Garage Position Sensor shall detect the garage door position from fixed NFC tags, convert that position into a logical opening percentage, and publish the result over Zigbee as a window covering device. The firmware shall also provide direct local diagnostics through serial logs and the onboard WS2812 status LED.

### Problem Statement

The system is intended to provide reliable local door-position feedback without mechanical encoders or cloud dependencies. It shall make the current door state visible both to Zigbee consumers such as Zigbee2MQTT and directly on the device through simple LED indications.

### Users / Stakeholders

- Device maintainer / firmware developer
- Local homeowner or operator
- Home automation consumers via Zigbee2MQTT or MQTT-based integrations
- FHEM users consuming Zigbee2MQTT MQTT topics

### Goals

- Report door opening percentage in the project semantic `0 = closed`, `100 = open`
- Expose compatible Zigbee metadata and attributes for Zigbee2MQTT
- Provide a visible local status indication for boot, startup/join, ready, closed, open, and error states
- Keep field diagnostics possible without BLE or a permanently attached serial monitor

### Non-Goals

- Motor control of the garage door
- Zigbee OTA firmware updates
- Rich UI, mobile app, or cloud backend

### High-Level System Flow

1. On boot, the ESP32-C6 initializes serial logging, the status LED, PN532, Zigbee, and optional BLE debug.
2. The PN532 reads NFC tag UIDs and maps them to a stable position index.
3. The stable index is converted into an opening percentage.
4. The opening percentage is translated into the Zigbee Window Covering cluster-facing lift value.
5. Zigbee consumers receive the current logical position, while the onboard LED indicates local runtime state.

## 2. System Architecture

### 2.1 Logical Architecture

- NFC acquisition subsystem: PN532 over SPI reads passive tags
- Position state subsystem: `pendingIndex` and `stableIndex` confirm a valid door position
- Zigbee publishing subsystem: local `ZigbeeWindowCovering` extension exposes the door position and firmware metadata
- Diagnostics subsystem: serial logs, optional BLE UART, and the onboard WS2812 status LED

### 2.2 Hardware / Platform Architecture

- MCU: ESP32-C6 (`ESP32C6 Dev Module`)
- NFC reader: PN532 via SPI
- Local input: onboard button for diagnostics / factory reset
- Local indicator: onboard WS2812 RGB LED via `RGB_BUILTIN`
- Wireless protocol: Zigbee end device mode

### 2.3 Software Architecture

- Entry point: `src/nfc-garage-position-sensor/nfc-garage-position-sensor.ino`
- NFC handling: `src/nfc-garage-position-sensor/nfc_logic.ino`
- Tag map and percent conversion: `src/nfc-garage-position-sensor/tag_map.ino`
- Zigbee setup and publishing: `src/nfc-garage-position-sensor/zigbee.ino`
- Local LED diagnostics: `src/nfc-garage-position-sensor/status_led.ino`
- BLE debug transport: `src/nfc-garage-position-sensor/ble_debug.ino`
- Button handling and factory reset: `src/nfc-garage-position-sensor/button.ino`

Boot sequence:

1. Start serial logging
2. Flash the onboard LED briefly for boot visibility
3. Show a guaranteed visible blue startup window of approximately 3 seconds before PN532 and Zigbee initialization continue
4. Initialize PN532
5. Initialize Zigbee and attempt network start/join
6. Start BLE debug if enabled
7. Enter the main polling loop

Persistence / storage:

- No persistent local state is currently stored by the sketch

Update model:

- USB flashing through `arduino-cli`
- Production firmware releases use Semantic Versioning 2.0.0 (`MAJOR.MINOR.PATCH`)
- Firmware release history is maintained in `CHANGELOG.md` using the Keep a Changelog format
- OTA over temporary Wi-Fi AP is deferred for a future phase

## 3. Implementation Phases

### 3.1 Phase 1 - Infrastructure Foundation

Scope:

- ESP32-C6 firmware scaffold
- PN532 SPI integration
- Zigbee end-device setup
- Build, flash, and monitor scripts

Deliverables:

- Working firmware skeleton
- Local serial diagnostics
- Basic Zigbee interview support

Exit criteria:

- Firmware compiles for `esp32:esp32:esp32c6`
- Device boots and logs over serial
- Device can join a Zigbee network

Dependencies:

- ESP32 Arduino core with Zigbee support
- PN532 library

### 3.2 Phase 2 - Core Functionality

Scope:

- Stable NFC tag confirmation logic
- Door position mapping to `0..100 %`
- Zigbee Window Covering publishing
- Basic `swBuildId` support for Zigbee2MQTT Firmware-ID
- Onboard WS2812 status LED

Deliverables:

- Position sensor behavior over Zigbee
- Local LED diagnostics for major runtime states
- Repository documentation and setup instructions

Exit criteria:

- Known tags produce the expected logical position
- Zigbee2MQTT shows usable metadata and position values
- Local LED states are visible and match documented meanings

Dependencies:

- Valid tag layout
- Joinable Zigbee network

### 3.3 Phase 3 - Extensions / Enhancements

Scope:

- OTA over temporary Wi-Fi AP
- Finer-grained LED patterns for intermediate states or tag events
- Additional recovery and service tooling

Deliverables:

- Update mode specification
- Extended maintenance workflow

Exit criteria:

- Update flow documented and tested
- Recovery procedures validated

Dependencies:

- OTA-capable partition strategy
- Security decision for temporary AP and updater page

## 4. Functional Requirements

### 4.1 Functional Requirements (FR)

- FR-1.1 [Must]: The system shall read passive NFC tags through the PN532 over SPI.
- FR-1.2 [Must]: The system shall map each known tag UID to a deterministic door-position index.
- FR-1.3 [Must]: The system shall confirm a new candidate position only after `INDEX_CONFIRM_COUNT` repeated reads.
- FR-1.4 [Must]: The system shall convert the confirmed position index into a logical opening percentage where `0 = closed` and `100 = open`.
- FR-1.5 [Must]: The system shall publish the current position through the Zigbee Window Covering endpoint on endpoint `10`.
- FR-1.6 [Must]: The system shall expose firmware/software version metadata so Zigbee2MQTT can resolve Firmware-ID.
- FR-1.7 [Must]: The system shall provide a visible boot indication on the onboard WS2812 LED.
- FR-1.8 [Must]: The system shall provide a visible blue startup/join indication that remains noticeable even when Zigbee rejoins quickly.
- FR-1.9 [Must]: The system shall indicate normal ready operation with an unobtrusive idle LED state.
- FR-1.10 [Must]: The system shall indicate confirmed closed and confirmed non-closed states with distinct stable LED colors.
- FR-1.11 [Must]: The system shall indicate fatal startup failures through a visible red error blink pattern.
- FR-1.12 [Should]: The system should support a local factory reset through a long button press.
- FR-1.13 [Should]: The system should keep serial diagnostics available for runtime debugging.
- FR-1.14 [May]: The system may provide optional BLE debug notifications when enabled at build time.
- FR-1.15 [Should]: The project documentation should provide a tested FHEM `MQTT2_DEVICE` example that consumes the Zigbee2MQTT main topic and availability subtopic separately.

### 4.2 Non-Functional Requirements (NFR)

- NFR-1.1 [Must]: The firmware shall compile with the documented ESP32-C6 Arduino configuration.
- NFR-1.2 [Must]: Runtime polling shall remain non-blocking during normal operation except for short startup indication delays.
- NFR-1.3 [Must]: The LED in normal operation shall remain off or dim enough to avoid becoming distracting.
- NFR-1.4 [Should]: The system should recover from USB reset and rejoin scenarios without requiring a clean flash.
- NFR-1.5 [Should]: Local diagnostics should remain understandable without requiring external BLE tooling.
- NFR-1.6 [Should]: Documentation should describe flashing, monitoring, Zigbee behavior, and local LED states.
- NFR-1.7 [Must]: The startup indication window shall remain human-visible even when a Zigbee rejoin would otherwise complete too quickly to observe.
- NFR-1.8 [Must]: Production firmware releases shall use Semantic Versioning 2.0.0 (`MAJOR.MINOR.PATCH`) as defined by <https://semver.org/>.
- NFR-1.9 [Must]: Firmware release history shall be maintained in a `CHANGELOG.md` file using the Keep a Changelog format defined by <https://keepachangelog.com/>.

### 4.3 Constraints

- The project must run in ESP32-C6 Zigbee end-device mode.
- The project must use the onboard RGB LED interface exposed by the ESP32 Arduino core when available.
- The current implementation must preserve the project semantic `0 = closed`, `100 = open` even though the cluster-facing lift percentage is inverted internally.
- The project currently uses static tag definitions compiled into firmware.
- The current startup flow intentionally delays PN532 and Zigbee initialization by the configured visible blue startup window.

## 5. Risks, Assumptions, and Dependencies

### Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Zigbee join behavior varies with router/coordinator availability | Medium | High | Keep serial diagnostics and visible startup/join LED feedback |
| NFC boundary reads may cause transient false candidates | Medium | Medium | Require repeated reads before accepting a new stable position |
| Onboard LED timing may be too short for a human-visible state transition | Low | Medium | Keep a guaranteed visible startup window |

### Assumptions

- The installed ESP32 Arduino core exposes `RGB_BUILTIN` and `rgbLedWrite()` for the onboard LED.
- Zigbee2MQTT remains the primary integration target for user-visible metadata.
- The garage door has a fixed physical tag layout from closed to open.

### Dependencies

- ESP32 Arduino core with Zigbee support
- Adafruit PN532 library
- Reachable Zigbee network infrastructure

## 6. Interface Specifications

### 6.1 External Interfaces

| Interface | Direction | Purpose | Notes |
|-----------|-----------|---------|-------|
| SPI to PN532 | Bidirectional | NFC tag detection | Uses configured SCK/MISO/MOSI/SS pins |
| Zigbee Window Covering endpoint 10 | Outbound | Position/state reporting | End-device mode |
| Serial USB | Outbound | Diagnostics and runtime logs | 115200 baud |
| Onboard WS2812 LED | Outbound | Local device status | Human-visible runtime state |
| Zigbee2MQTT MQTT device topic | Outbound via Zigbee2MQTT | Home automation consumption in FHEM or other MQTT clients | Main payload includes `position`, `state`, `linkquality`, and `last_seen` |
| Zigbee2MQTT MQTT availability subtopic | Outbound via Zigbee2MQTT | Online/offline indication for MQTT consumers | JSON payload uses `{"state":"online|offline"}` semantics |

### 6.2 Internal Interfaces

| Producer | Consumer | Purpose |
|----------|----------|---------|
| `src/nfc-garage-position-sensor/nfc_logic.ino` | `src/nfc-garage-position-sensor/tag_map.ino` | UID-to-index lookup and percent conversion |
| `src/nfc-garage-position-sensor/nfc_logic.ino` | `src/nfc-garage-position-sensor/zigbee.ino` | Publish updated confirmed position |
| `src/nfc-garage-position-sensor/nfc-garage-position-sensor.ino` | `src/nfc-garage-position-sensor/status_led.ino` | Startup sequencing and loop polling |
| `src/nfc-garage-position-sensor/zigbee.ino` / `src/nfc-garage-position-sensor/nfc_logic.ino` | `src/nfc-garage-position-sensor/status_led.ino` | Error state activation |

### 6.3 Data Models / Schemas

| Field | Source | Meaning |
|-------|--------|---------|
| `stableIndex` | NFC logic | Last confirmed physical position |
| `pendingIndex` | NFC logic | Candidate next position |
| `position` | Project semantic | Opening percentage, `0 = closed`, `100 = open` |
| `lift` | Zigbee cluster-facing value | Inverted internal lift percentage for window covering reporting |
| `swBuildId` | Basic cluster | Firmware-ID for Zigbee2MQTT |
| `availability.state` | Zigbee2MQTT availability subtopic | Online/offline indication intended for a dedicated consumer-side availability reading |

### 6.4 Commands / Opcodes

| Command / Action | Trigger | Effect |
|------------------|---------|--------|
| Short button press | Local button | Prints Zigbee status |
| Long button press | Local button | Triggers Zigbee factory reset |

## 7. Operational Procedures

### Flashing / Deployment

1. Build with `.\tools\firmware\build.ps1`
2. Upload with `.\tools\firmware\flash.ps1`
3. Use `.\tools\firmware\flash-clean.ps1` only when a clean re-pair is required

### Provisioning / Configuration

- Select board `ESP32C6 Dev Module`
- Select Zigbee mode `Zigbee ED`
- Select partition scheme `zigbee`
- Keep the configured tag table aligned with the physical tag order

### Normal Operation

1. Device boots and flashes white briefly
2. Device shows a visible blue startup/join window before PN532 and Zigbee initialization continue
3. Device joins or rejoins the Zigbee network
4. Device reads NFC tags continuously
5. Device publishes confirmed position changes and updates LED state

### Maintenance

- Use serial monitor for detailed logs
- Use the LED when a serial console is not available
- Re-pair in Zigbee2MQTT after significant endpoint or metadata changes
- For FHEM, subscribe to the main Zigbee2MQTT device topic and the `/availability` subtopic separately to avoid overwriting the cover `state` reading with online/offline state
- For each production firmware release, assign a Semantic Versioning 2.0.0 version
- Record release notes in `CHANGELOG.md` using Keep a Changelog section structure

### Recovery

- Use long button press for factory reset
- If pairing state is inconsistent, remove the device from Zigbee2MQTT and re-interview it
- If firmware changes require a fresh network state, use `tools/firmware/flash-clean.ps1`

## 8. Verification and Validation

### 8.1 Phase 1 Verification

| Test ID | Feature | Procedure | Success Criteria |
|---------|---------|-----------|-----------------|
| TC-1.1 | Build environment | Run `.\tools\firmware\build.ps1` | Sketch compiles for ESP32-C6 |
| TC-1.2 | Flash workflow | Run `.\tools\firmware\flash.ps1` on the device | Upload completes and device reboots |
| TC-1.3 | Serial diagnostics | Run `.\tools\firmware\monitor.ps1` or direct serial read | Boot and runtime logs are visible |

### 8.2 Phase 2 Verification

| Test ID | Feature | Procedure | Success Criteria |
|---------|---------|-----------|-----------------|
| TC-2.1 | Closed tag mapping | Present the `0 %` tag | Logs and Zigbee publish `position = 0` |
| TC-2.2 | Open tag mapping | Present the `100 %` tag | Logs and Zigbee publish `position = 100` |
| TC-2.3 | Intermediate tags | Move through intermediate tags | Position changes follow the configured table |
| TC-2.4 | Firmware-ID metadata | Join the device in Zigbee2MQTT | Firmware-ID resolves from `swBuildId` |
| TC-2.5 | LED boot state | Power-cycle the device | Short white flash is visible |
| TC-2.6 | LED startup/join state | Power-cycle the device | Blue startup/join indication remains visible |
| TC-2.7 | LED closed/open states | Move from closed to open and back | Closed and non-closed states use distinct colors |
| TC-2.8 | Error LED state | Force PN532 or Zigbee startup failure in a controlled test | Red blink pattern appears |
| TC-2.9 | Release versioning | Review the release artifact version string for a production release | Version follows `MAJOR.MINOR.PATCH` without additional format deviations |
| TC-2.10 | Release history documentation | Review `CHANGELOG.md` for a production release | File exists and release entries follow the Keep a Changelog structure |
| TC-2.11 | FHEM MQTT mapping | Apply the documented `MQTT2_DEVICE` example to the Zigbee2MQTT topics | `position`, `state`, `linkquality`, and `availability` update as intended and `devStateIcon` shows icon plus text |

### 8.3 Acceptance Tests

- AT-1: Device boots, rejoins Zigbee, and reports a valid position without requiring a clean flash.
- AT-2: A local user can distinguish boot, startup/join, closed, open, and error conditions from the LED alone.
- AT-3: Zigbee2MQTT displays the device with correct metadata and usable position semantics.

### 8.4 Traceability Matrix

| Requirement | Priority | Test Case(s) | Status |
|------------|----------|-------------|--------|
| FR-1.1 | Must | TC-2.1, TC-2.2, TC-2.3 | Covered |
| FR-1.2 | Must | TC-2.1, TC-2.2, TC-2.3 | Covered |
| FR-1.3 | Must | TC-2.3 | Covered |
| FR-1.4 | Must | TC-2.1, TC-2.2, TC-2.3 | Covered |
| FR-1.5 | Must | TC-2.1, TC-2.2, TC-2.3 | Covered |
| FR-1.6 | Must | TC-2.4 | Covered |
| FR-1.7 | Must | TC-2.5 | Covered |
| FR-1.8 | Must | TC-2.6 | Covered |
| FR-1.9 | Must | TC-2.6, AT-2 | Covered |
| FR-1.10 | Must | TC-2.7 | Covered |
| FR-1.11 | Must | TC-2.8 | Covered |
| FR-1.12 | Should | TC-1.3, AT-1 | Covered |
| FR-1.13 | Should | TC-1.3 | Covered |
| FR-1.14 | May | --- | Optional |
| FR-1.15 | Should | TC-2.11 | Covered |
| NFR-1.1 | Must | TC-1.1 | Covered |
| NFR-1.2 | Must | TC-2.3, TC-2.6 | Covered |
| NFR-1.3 | Must | TC-2.6, TC-2.7 | Covered |
| NFR-1.4 | Should | AT-1 | Covered |
| NFR-1.5 | Should | AT-2 | Covered |
| NFR-1.6 | Should | Review of repo docs | Covered |
| NFR-1.7 | Must | TC-2.6 | Covered |
| NFR-1.8 | Must | TC-2.9 | Covered |
| NFR-1.9 | Must | TC-2.10 | Covered |

## 9. Troubleshooting Guide

| Symptom | Likely Cause | Diagnostic Steps | Corrective Action |
|---------|-------------|-----------------|-------------------|
| Device does not show Firmware-ID in Zigbee2MQTT | `swBuildId` not exposed or stale interview | Check fresh pairing and Zigbee interview | Re-pair and verify current firmware |
| Position appears inverted in Zigbee2MQTT | Consumer-side cover inversion | Check `invert_cover` setting | Keep `invert_cover = false` for this project |
| Blue startup indicator is not visible | Startup state too short or board not using onboard RGB LED | Power-cycle and inspect early boot visually | Keep `STATUS_LED_STARTUP_VISIBLE_MS` non-zero |
| No local LED activity | Board macro or RGB LED path unavailable | Check core support for `RGB_BUILTIN` and serial logs | Verify board/core setup and onboard LED support |
| Position jumps unexpectedly | Neighboring tags read during movement | Inspect `TAG` logs near the transition | Re-check physical tag spacing or confirmation count |
| FHEM shows `online` or `offline` as the cover `state` | Main topic and availability subtopic were mapped to the same reading | Inspect the `MQTT2_DEVICE` readingList and check the `availability` reading separately | Map `$DEVICETOPIC/availability` to a dedicated `availability` reading |

## 10. Appendix

### Key Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `ZIGBEE_COVERING_ENDPOINT` | `10` | Zigbee endpoint |
| `ZB_JOIN_TIMEOUT_MS` | `240000` | Zigbee join timeout |
| `STATUS_LED_BOOT_FLASH_MS` | `80` | Boot flash duration |
| `STATUS_LED_STARTUP_VISIBLE_MS` | `3000` | Guaranteed visible startup window |
| `STATUS_LED_START_BLINK_MS` | `700` | Blue startup/join blink interval |
| `STATUS_LED_PAIRING_BLINK_MS` | `180` | Pairing blink interval |
| `STATUS_LED_ERROR_BLINK_MS` | `250` | Error blink interval |

### Pinout Summary

| Signal | Pin |
|--------|-----|
| `PIN_SPI_SCK` | `20` |
| `PIN_SPI_MISO` | `19` |
| `PIN_SPI_MOSI` | `18` |
| `PIN_PN532_SS` | `14` |
| `BUTTON_PIN` | `9` |

### LED State Summary

| State | LED behavior |
|-------|--------------|
| Boot | Short white flash |
| Startup / join | Slow blue blink |
| Pairing not ready after stack start | Fast blue blink |
| Ready without confirmed position | Very dim green |
| Confirmed closed position | Green |
| Confirmed non-closed position | Orange |
| Error | Red blink |

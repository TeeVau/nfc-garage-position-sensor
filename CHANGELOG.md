# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2026-05-01

### Added

- Final 10-tag on-metal UID layout with the field-tested garage-door mapping
  `0/11/22/33/44/56/67/78/89/100 %`.
- Generic capture helpers for short serial and MQTT debug sessions.
- Versioned local build artifacts under `bin/` for release-oriented firmware
  packaging.

### Changed

- Promoted the project to the current field-tested release baseline.
- Simplified the maintenance model back to USB flashing only; the temporary
  Wi-Fi / HTTP OTA updater path was removed from the firmware and tooling.
- Refreshed README, Zigbee2MQTT notes, tag-layout documentation, and the FSD
  to match the final release behavior.

### Fixed

- Zigbee runtime reporting now stays reliable even when Zigbee2MQTT interviews
  the device successfully but skips the converter `configure()` step.
- BLE-debug builds now keep the verbose Zigbee neighbor / route diagnostics
  isolated from standard production builds.

## [0.2.5] - 2026-04-25

### Added

- Onboard WS2812 status LED feedback for boot, startup/join, ready, closed,
  open, and error states.
- Zigbee firmware metadata publishing for both `appVersion` and `swBuildId` so
  Zigbee2MQTT can resolve the Firmware-ID.
- Repository documentation for status LED behavior, Zigbee2MQTT setup, and the
  functional specification.

### Changed

- Zigbee position reporting and project documentation were aligned to the
  logical semantics `0 = closed` and `100 = open`.
- The sketch structure was modularized across dedicated Arduino tabs for NFC,
  Zigbee, LED, button, and debug responsibilities.

### Fixed

- Zigbee2MQTT metadata handling so `Firmware-ID` is no longer left unresolved
  when `appVersion` alone is insufficient.
- BLE debug connection stability during local diagnostics.

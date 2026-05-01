# Contributing

Thanks for helping improve the NFC Garage Position Sensor.

## Before You Start

- Read the [README](./README.md) and the relevant docs under [docs](./docs/).
- Check existing issues before opening a new one.
- Keep the project maker-friendly: simple setup, clear docs, and stable behavior matter more than cleverness.

## Reporting Bugs

Please include:

- the exact board and PN532 module you used
- firmware version
- Zigbee2MQTT version
- a short reproduction flow
- relevant serial or Zigbee2MQTT output

Use the bug report template whenever possible.

## Submitting Changes

1. Keep changes focused.
2. Update docs when behavior or setup changes.
3. Avoid committing local logs, build output, or temporary debug captures.
4. Preserve the user-facing semantics `0 = closed` and `100 = open` unless the change explicitly redefines the project.

## Pull Request Expectations

- Explain what changed and why.
- Describe how the change was validated.
- Call out any hardware assumptions or limitations.

## Scope Guidance

Good contribution areas:

- bug fixes
- documentation improvements
- hardware reproducibility improvements
- Zigbee2MQTT compatibility hardening

Please discuss larger architectural changes in an issue before investing significant implementation effort.

# Bill Of Materials

This BOM covers one complete NFC Garage Position Sensor build.

## Summary

- Target board: `ESP32C6 Dev Module`
- NFC reader: `PN532` in SPI mode
- Quantity: `1` build
- Integration target: `Zigbee2MQTT`

## Core Components

| Qty | Component | Recommended specification | Purpose |
|-----|-----------|---------------------------|---------|
| 1 | ESP32-C6 development board | `ESP32C6 Dev Module` compatible board with onboard WS2812 LED | Main controller and Zigbee radio |
| 1 | PN532 NFC module | SPI-capable breakout/module | Reads the fixed NFC tags |
| 10 | NFC tags | Consistent tag type across the whole travel path | Position reference points |
| 1 | USB cable | Matching your ESP32-C6 board connector | Flashing and power |

## Wiring And Installation Parts

| Qty | Component | Recommended specification | Purpose |
|-----|-----------|---------------------------|---------|
| 1 set | Jumper wires | Female/female or mixed, depending on board and module headers | SPI wiring |
| 1 | Mounting set | Double-sided tape, screws, clips, or custom bracket | Reader and controller mounting |
| 1 | Cable management set | Zip ties, clips, or adhesive anchors | Strain relief and routing |

## Optional Parts

| Qty | Component | When to use it | Notes |
|-----|-----------|----------------|-------|
| 1 | Small enclosure | If the ESP32-C6 needs dust or touch protection | Keep USB access available |
| 1 | PN532 bracket or holder | If the reader needs rigid alignment | Especially useful in vibrating installations |
| 1 | External 5 V power supply | If USB power is not practical in the final installation | Keep a common ground |

## Known Working Hardware Notes

- The firmware assumes an ESP32-C6 board with onboard RGB LED support exposed by the Arduino core.
- The NFC reader must be configured for SPI.
- Use the same NFC tag family for all positions to keep behavior consistent.

## Procurement Tips

- Prefer one known PN532 module type and buy a spare if the installation is hard to access later.
- Buy a few extra tags so you can replace damaged or badly performing ones without reworking the project concept.
- If the garage environment is dusty or humid, prioritize mounting and cable protection early.

## Not Included

This repo currently does not ship:

- a PCB design
- a mandatory enclosure
- a Fritzing source file
- release-ready hardware photos

The project is designed around straightforward maker wiring rather than a custom board.

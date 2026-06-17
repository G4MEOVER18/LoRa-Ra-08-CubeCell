# Abhängigkeiten

Reproduzierbarer Build über **PlatformIO** mit der **heltec-cubecell**-Plattform.
Konfiguration in `platformio.ini` gepinnt.

| Abhängigkeit | Wert | Zweck |
|--------------|------|-------|
| Plattform | `heltec-cubecell` | CubeCell ASR6502 (ARM Cortex-M0+) Toolchain |
| Board | `cubecell_board` | Heltec CubeCell HTCC-AB01 |
| Framework | `arduino` | CubeCell-Arduino-API (`LoRaWan_APP.h`, `Radio`) |
| LoRa-Region | `EU433` | passend zu 433.775 MHz |

**Keine externen Libraries** — `LoRaWan_APP.h` (Radio-API) ist Teil des
CubeCell-Frameworks.

## Flash-Hinweis

CubeCell verwendet **nicht** den ESP32-Flash-Weg (`esptool`/`.bin`). Der Build
erzeugt:

- `firmware.cyacd` — natives CubeCell-Format, flashbar mit **CubeCellUpdater**
  (Heltec GUI) oder `cubecellflash` über den Cypress/PSoC-Bootloader.
- `firmware.hex` — Intel-HEX als Alternative.

Direktes Flashen aus PlatformIO: `pio run -t upload` (Board angeschlossen).

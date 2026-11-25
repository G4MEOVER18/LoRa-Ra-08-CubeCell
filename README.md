<div align="center">

# LoRa-Ra-08-CubeCell

[![CI](https://github.com/G4MEOVER18/LoRa-Ra-08-CubeCell/actions/workflows/platformio-build.yml/badge.svg)](https://github.com/G4MEOVER18/LoRa-Ra-08-CubeCell/actions/workflows/platformio-build.yml)

**LoRa-RX-Finder für Heltec CubeCell (ASR6502) bei 433.775 MHz** — empfängt Pakete und zeigt die Signalstärke per LED (hot/cold) und Serial.

[![Plattform](https://img.shields.io/badge/Plattform-CubeCell%20ASR6502-blue?style=flat-square)](.)
[![Build](https://img.shields.io/badge/Build-PlatformIO%20(heltec--cubecell)-orange?style=flat-square&logo=platformio)](https://platformio.org)
[![Frequenz](https://img.shields.io/badge/LoRa-433.775%20MHz-green?style=flat-square)](.)
[![Lizenz](https://img.shields.io/badge/Lizenz-MIT-green?style=flat-square)](LICENSE)

</div>

---

Empfängt im Dauer-RX-Modus LoRa-Pakete auf **433.775 MHz** und signalisiert die Empfangsqualität (RSSI) — praktisch zum Auffinden/Ausrichten eines Senders („hot/cold"-Suche) per LED und serieller Ausgabe. Läuft auf dem **Heltec CubeCell HTCC-AB01** (ASR6502, ARM Cortex-M0+).

## Hardware

| Parameter | Wert |
|-----------|------|
| Board | Heltec CubeCell HTCC-AB01 (`cubecell_board`) |
| Funk | LoRa @ 433.775 MHz (Region EU433) |
| Anzeige | RGB-LED (hot/cold) + Serial @ 115200 |

## Fertige Firmware

Vorkompilierte Images unter [**Releases**](https://github.com/G4MEOVER18/LoRa-Ra-08-CubeCell/releases/latest):

| Datei | Flashen mit |
|-------|-------------|
| **`…_v1.0.0.cyacd`** | CubeCellUpdater (Heltec) oder `cubecellflash` — natives CubeCell-Format |
| **`…_v1.0.0.hex`** | Intel-HEX (alternative Flash-Tools) |

> CubeCell (ASR6502) nutzt **kein** ESP32-`.bin`/`esptool` — das `.cyacd`-Format wird über den Cypress/PSoC-Bootloader geflasht.

## Selbst bauen (PlatformIO)

```bash
pio run                       # bauen
pio run -t upload             # flashen (Board angeschlossen)
pio device monitor -b 115200  # serielle Ausgabe
```

Plattform und Region sind in `platformio.ini` gepinnt (heltec-cubecell, EU433). Details: [`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md).

> **Hinweis zu `versions/` und `legacy/`:** ältere Experimentierstände. Der gebaute Stand ist `src/LoRaV4/`.

## Lizenz

MIT – siehe [LICENSE](LICENSE). © 2026 Yanis Ameseder

---

## Kontakt

**Entwickler:** Yanis Ameseder · **E-Mail:** [g4me.over.18@gmail.com](mailto:g4me.over.18@gmail.com)

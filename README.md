# Totally-Spies-ComPowder

A DIY smartwatch style handheld built around an ESP32-S3 and a 1.28" round touch display, housed in a custom 3D-printed clamshell case inspired by the "Compowder" from *Totally Spies*.

![status](https://img.shields.io/badge/status-in%20progress-yellow)
![platform](https://img.shields.io/badge/platform-ESP32--S3-blue)

## Overview

ComPowder is a pocket sized, app based device with a circular touchscreen UI, physical buttons, capacitive touch gestures, and an IMU for gesture input, all packed into a round clamshell enclosure that opens like a compact mirror. The firmware is built around [LVGL](https://lvgl.io/) for a fluid, app-switching interface.

## Hardware

- **MCU**: ESP32-S3 (N16R8 16MB flash / 8MB PSRAM)
- **Display**: 1.28" round TFT, GC9A01 driver, 240×240, SPI
- **Touch**: Capacitive touch controller (I2C)
- **IMU**: QMI8658 (I2C) for wrist gesture input
- **Input**: 3 physical side buttons + touchscreen
- **Audio**: Speaker output (planned)
- **LEDs**: 2× WS2812 addressable RGB
- **Storage**: microSD card slot
- **Power**: 3.7V LiPo battery, USB-C charging/programming
- **Enclosure**: Custom 3D-printed round clamshell case with living hinge, designed to fit all onboard components

### Pinout

| Signal | GPIO |
|---|---|
| TFT DC | 18 |
| TFT CS | 2 |
| TFT SCK | 3 |
| TFT MOSI | 10 |
| TFT RST | 21 |
| TFT Backlight | 42 |
| Touch SDA / SCL | 8 / 9 |
| Touch RST / INT | 0 / 11 |
| Button Up / Power / Down | 14 / 15 / 16 |
| WS2812 LED | 46 |

## Software

- Arduino IDE, ESP32 core (Espressif Arduino-ESP32)
- [LVGL](https://github.com/lvgl/lvgl) — UI framework
- [Adafruit GC9A01A](https://github.com/adafruit/Adafruit_GC9A01A) / Arduino_GFX display driver
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) RGB LED control

## Apps

- **Radar**: animated scanning UI (WiFi/BLE nearby-device signal strength mapping, in progress)
- **Net Scanner**: visualizes WiFi routers and BLE devices around you on the round display, showing signal strength to help find dead zones
- **Spotify Remote**: play/pause/skip via touch, volume control via wrist-tilt (IMU), using the Spotify Web API over WiFi (BLE media control isn't possible on ESP32-S3, which lacks Bluetooth Classic/AVRCP)

## 3D-Printed Case

Parametric OpenSCAD model of a round clamshell enclosure:
- Top shell houses the ESP32-S3 board and display
- Bottom shell houses the battery, speaker, and external USB-C port
- Interlocking printed hinge (pin-based) with a dedicated cable pass-through gap
- Snap latch closure on the front edge

## Status / Roadmap

- [x] Display bring-up (GC9A01 over SPI)
- [x] Touch + button input
- [x] Base app-switching architecture
- [x] Migrate UI to LVGL
- [ ] Real WiFi/BLE radar scanning
- [ ] IMU integration (wrist-gesture volume control)
- [ ] Spotify Web API integration (OAuth + playback control)
- [ ] Final wiring through the case hinge
- [ ] Case fit/assembly pass with real components

## License

TBD

---

Documented as part of a build series — see the project's video updates for step-by-step progress.

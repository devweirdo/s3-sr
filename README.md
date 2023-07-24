# Speech recognition on ESP32-S3-DevKitC-1 + INMP441

## Overview

Speech recognition example for ESP32-S3-DevKitC-1 with INMP441 MEMS microphone using [esp-sr](https://github.com/espressif/esp-sr).

### Notes:

- MultiNet6 EN + WakeNet will be used in this example.

- Only **ESP-IDF v5.x** is currently supported for this example.

## How to use the example

### Hardware Required

- ESP32-S3-DevKitC-1
- INMP441 MEMS microphone

### Hardware connection

The connection between ESP Board and INMP441 is as follows:
```
       ESP Board                       INMP441
┌──────────────────────┐              ┌────────────────────┐
│             GND      ├─────────────►│ GND                │
│                      │              │                    │
│             3V3      ├─────────────►│ VDD                │
│                      │              │                    │
│             IO41     ├─────────────►│ SD                 │
│                      │              │                    │
│             IO42     ├─────────────►│ SCK                │
│                      │              │                    │
│             IO40     |◄─────────────┤ WS                 │
│                      │              │                    │
│             GND      ├─────────────►│ L/R                │
└──────────────────────┘              └────────────────────┘
```
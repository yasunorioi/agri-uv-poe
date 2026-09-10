# agri-uv-poe

[🇯🇵 日本語](README_ja.md) · **English**

M5Stack ATOM PoE Kit + DFRobot Gravity UV Index Sensor **SEN0636** (240–370nm,
UV/UVA/UVB/UVC, UV Index 0–11 + Risk Level 0–4 + raw value mV) → MQTT + UECS-CCM.
It sits as a thin layer on top of the
[agri-node-poe-core](https://github.com/yasunorioi/agri-node-poe-core)
library. The visual jig for checking values is
[agri-uv-bringup](https://github.com/yasunorioi/agri-uv-bringup) (M5 Basic / AtomS3R).

## Hardware

- **MCU**: M5Stack ATOM Lite (ESP32-PICO-D4)
- **PoE / Ethernet**: M5Stack ATOM PoE Base (W5500 on SPI)
- **Sensor**: DFRobot Gravity UV Index Sensor **SEN0636**
  - Keep the mode selector switch on the **UART side** (read via Modbus RTU)

### Wiring (ATOM Grove → sensor 4-pin)

```
G26 (RX) ← sensor pin2 (C/T = sensor TX)
G32 (TX) → sensor pin1 (D/R = sensor RX)
5V        → sensor pin3
GND       → sensor pin4
```

Since UART does not use SPI, there is no conflict with the W5500 (PoE). This makes it straightforward to turn into a PoE node.

## Protocol

Modbus RTU, **9600 8N1, slave 0x23**

| input reg | Content |
|-----------|------|
| `0x06` | UV raw (mV, 0–3300) |
| `0x07` | UV Index (0–11) |
| `0x08` | Risk Level (0–4 = Low / Moderate / High / Very High / Extreme) |

The library [`DFRobot/DFRobot_UVIndex240370Sensor`](https://github.com/DFRobot/DFRobot_UVIndex240370Sensor)
contains a stray `#include "String.h"` referencing a header that does not exist in the `.h/.cpp`, which breaks on the ESP32 core, so the
pre-build hook [`patch_dfrobot.py`](patch_dfrobot.py) strips the offending line inside `.pio/libdeps`.

## Configuration (NVS persistence)

`Preferences` namespace `uv-cfg`. Edit from the Web UI at `/config`:

- **Common**: Node ID, hostname, MQTT host/port/user/pass/topic prefix/interval,
  UECS-CCM enable/interval/room/region/priority/node-type
- **Sensor-specific**:
  - `Order (UV Index)` — CCM order of the `InUvIndex` channel (default 1)
  - `Order (UV Risk)` — CCM order of the `InUvRisk` channel (default 2)

## Publishing

- **MQTT**: retained JSON on the topic prefix (default `agriha/h01/sensor/Uv`)
  `{node_id, uptime_s, sensor_ok, uv_index, uv_risk, risk, uv_mv}`
- **UECS-CCM**: broadcast + multicast of `InUvIndex.<ntype>` (0–11) and `InUvRisk.<ntype>` (0–4).
  Since the UECS standard has no UV type, custom naming is used (ArSprout just receives it; can be changed later).

## Build / Flash

```sh
pio run -t upload
```

> 🛠 **Build environment (shared Windows / Linux) and first-time Linux setup (udev, etc.)** →
> [agri-node-poe-core/docs/cross-platform-build.md](https://github.com/yasunorioi/agri-node-poe-core/blob/main/docs/cross-platform-build.md)

> ATOM Lite is fixed at upload_speed=115200 (230400 or higher tends to fail).
> It boots even without a sensor connected, re-handshaking every 1 second while keeping `sensor_ok=false`.

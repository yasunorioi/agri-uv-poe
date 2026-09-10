# agri-uv-poe

**日本語** · [🇬🇧 English](README_en.md)

M5Stack ATOM PoE Kit + DFRobot Gravity UV Index Sensor **SEN0636**（240–370nm、
UV/UVA/UVB/UVC、UV Index 0–11 + Risk Level 0–4 + 生値 mV）→ MQTT + UECS-CCM。
[agri-node-poe-core](https://github.com/yasunorioi/agri-node-poe-core)
ライブラリ上に薄く乗っているだけ。値の目視治具は
[agri-uv-bringup](https://github.com/yasunorioi/agri-uv-bringup)（M5 Basic / AtomS3R）。

## ハードウェア

- **MCU**: M5Stack ATOM Lite (ESP32-PICO-D4)
- **PoE / Ethernet**: M5Stack ATOM PoE Base (W5500 on SPI)
- **センサー**: DFRobot Gravity UV Index Sensor **SEN0636**
  - モード切替スイッチを **UART 側**にしておくこと（Modbus RTU で読む）

### 配線（ATOM Grove → センサー 4-pin）

```
G26 (RX) ← sensor pin2 (C/T = sensor TX)
G32 (TX) → sensor pin1 (D/R = sensor RX)
5V        → sensor pin3
GND       → sensor pin4
```

UART は SPI を使わないので W5500（PoE）と競合しない。素直に PoE ノードにできる。

## プロトコル

Modbus RTU, **9600 8N1, slave 0x23**

| input reg | 内容 |
|-----------|------|
| `0x06` | UV raw (mV, 0–3300) |
| `0x07` | UV Index (0–11) |
| `0x08` | Risk Level (0–4 = Low / Moderate / High / Very High / Extreme) |

ライブラリ [`DFRobot/DFRobot_UVIndex240370Sensor`](https://github.com/DFRobot/DFRobot_UVIndex240370Sensor)
は `.h/.cpp` に存在しないヘッダ `#include "String.h"` が混入していて ESP32 core で死ぬため、
pre-build フック [`patch_dfrobot.py`](patch_dfrobot.py) が `.pio/libdeps` 内の該当行を剥がす。

## 設定（NVS 永続化）

`Preferences` ネームスペース `uv-cfg`。Web UI の `/config` から編集:

- **共通**: Node ID, hostname, MQTT host/port/user/pass/topic prefix/interval,
  UECS-CCM enable/interval/room/region/priority/node-type
- **センサー固有**:
  - `Order (UV Index)` — `InUvIndex` チャネルの CCM order（既定 1）
  - `Order (UV Risk)` — `InUvRisk` チャネルの CCM order（既定 2）

## 配信

- **MQTT**: topic prefix（既定 `agriha/h01/sensor/Uv`）に retained JSON
  `{node_id, uptime_s, sensor_ok, uv_index, uv_risk, risk, uv_mv}`
- **UECS-CCM**: `InUvIndex.<ntype>`（0–11）と `InUvRisk.<ntype>`（0–4）を broadcast +
  multicast。UECS 標準に UV 型は無いので独自命名（ArSprout は受けるだけ、後で変更可）。

## ビルド / 書き込み

```sh
pio run -t upload
```

> 🛠 **ビルド環境（Windows / Linux 共用）・Linux 初回セットアップ（udev 等）** →
> [agri-node-poe-core/docs/cross-platform-build.md](https://github.com/yasunorioi/agri-node-poe-core/blob/main/docs/cross-platform-build.md)

> ATOM Lite は upload_speed=115200 固定（230400 以上は失敗しやすい）。
> センサー未接続でも起動し、1 秒ごとに再ハンドシェイクして `sensor_ok=false` を維持する。

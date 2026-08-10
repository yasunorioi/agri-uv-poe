// sensors.h — DFRobot SEN0636 UV Index sensor over Modbus RTU (UART).
//
// The sensor talks Modbus RTU, 9600 8N1, slave 0x23 on a HardwareSerial
// remapped to the ATOM Grove port (RX=G26, TX=G32). The DFRobot library
// wraps DFRobot_RTU; readXxxData() returns 0 on a bus timeout, so a stuck
// bus reads as UV Index 0 — we lean on the begin() handshake for the
// sensor_ok flag and re-handshake when it drops.
//
// Wiring (Grove → sensor 4-pin, sensor mode switch must be on UART):
//   G26 (RX) ← sensor pin2 (C/T = sensor TX)
//   G32 (TX) → sensor pin1 (D/R = sensor RX)
//   5V        → sensor pin3
//   GND       → sensor pin4

#pragma once

#include <Arduino.h>
#include <DFRobot_UVIndex240370Sensor.h>
#include "config.h"

static const int PIN_UART_RX = 26;   // ATOM Grove, sensor TX
static const int PIN_UART_TX = 32;   // ATOM Grove, sensor RX

extern DFRobot_UVIndex240370Sensor g_uv;   // constructed on Serial2 in main.cpp

extern bool     g_uv_ok;
extern uint16_t g_uv_index;    // 0–11
extern uint16_t g_uv_risk;     // 0–4 (Low/Moderate/High/Very High/Extreme)
extern uint16_t g_uv_mv;       // raw mV, 0–3300

inline const char *uvRiskLabel(uint16_t r) {
  switch (r) {
    case 0: return "Low";
    case 1: return "Moderate";
    case 2: return "High";
    case 3: return "Very High";
    case 4: return "Extreme";
    default: return "?";
  }
}

inline bool sensorsBegin() {
  Serial2.begin(9600, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  g_uv_ok = g_uv.begin();
  Serial.printf("[SENS] SEN0636 UART G%d/G%d 9600 8N1 slave 0x23: %s\n",
                PIN_UART_RX, PIN_UART_TX, g_uv_ok ? "OK" : "MISSING");
  return g_uv_ok;
}

inline void sensorsPoll() {
  if (!g_uv_ok) {
    // Re-handshake in case the sensor was hot-plugged / power-cycled.
    sensorsBegin();
    if (!g_uv_ok) return;
  }
  g_uv_mv    = g_uv.readUvOriginalData();
  g_uv_index = g_uv.readUvIndexData();
  g_uv_risk  = g_uv.readRiskLevelData();
}

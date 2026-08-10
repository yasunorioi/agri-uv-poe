// mqtt_pub.h — UV JSON payload to the configured topic prefix.

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AgriNode.h>
#include "config.h"
#include "sensors.h"

inline bool mqttPublishUv() {
  if (!agri::MQTT::hasHost(g_cfg.common) || !agri::MQTT::connected()) return false;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  root["node_id"]   = g_cfg.common.node_id;
  root["uptime_s"]  = millis() / 1000;
  root["sensor_ok"] = g_uv_ok;
  if (g_uv_ok) {
    root["uv_index"] = g_uv_index;
    root["uv_risk"]  = g_uv_risk;
    root["risk"]     = uvRiskLabel(g_uv_risk);
    root["uv_mv"]    = g_uv_mv;
  }

  char payload[224];
  size_t n = serializeJson(doc, payload, sizeof(payload));
  bool ok = agri::MQTT::mqtt.publish(g_cfg.common.mqtt_topic_prefix,
                                     (const uint8_t*)payload, n, true);
  Serial.printf("[MQTT] %s %s (%u bytes)\n",
                g_cfg.common.mqtt_topic_prefix, ok ? "OK" : "FAIL",
                (unsigned)n);
  return ok;
}

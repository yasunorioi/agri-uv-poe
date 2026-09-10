// agri-uv-poe — M5 ATOM PoE UV Index node.
// DFRobot Gravity UV Index Sensor SEN0636 (240–370nm) over Modbus RTU.

#include <Arduino.h>
#include <AgriNode.h>

#include "config.h"
#include "sensors.h"
#include "mqtt_pub.h"
#include "ccm_pub.h"

const char *FW_NAME     = "agri-uv-poe";
const char *FW_VERSION  = "0.1.0";
const char *FW_REPO     = "yasunorioi/agri-uv-poe";
const char *FW_BIN_NAME = "agri-uv-poe.bin";

AppConfig g_cfg;

// Sensor talks Modbus RTU on Serial2 (remapped to the Grove port in
// sensorsBegin). The library object lives here so sensors.h can stay a
// multiply-included header.
DFRobot_UVIndex240370Sensor g_uv(&Serial2);

bool     g_uv_ok    = false;
uint16_t g_uv_index = 0;
uint16_t g_uv_risk  = 0;
uint16_t g_uv_mv    = 0;

static String renderDashboardSensors() {
  String s; s.reserve(280);
  s = F("<h3>UV Index</h3><table>");
  if (g_uv_ok) {
    s += "<tr><th>UV Index</th><td>"; s += g_uv_index; s += "</td></tr>";
    s += "<tr><th>Risk</th><td>"; s += uvRiskLabel(g_uv_risk);
    s += " ("; s += g_uv_risk; s += ")</td></tr>";
    s += "<tr><th>Raw</th><td>"; s += g_uv_mv; s += " mV</td></tr>";
  } else {
    s += "<tr><th>SEN0636</th><td>NOT detected</td></tr>";
  }
  s += F("</table>");
  return s;
}

static String renderConfigSensorRows() {
  String s;
  auto row = [&](const char *label, const String &input) {
    s += "<tr><th>"; s += label; s += "</th><td>"; s += input; s += "</td></tr>";
  };
  row("Order (UV Index)",
      "<input type=number name=ccm_oidx value='" + String(g_cfg.ccm_order_index) + "'>");
  row("Order (UV Risk)",
      "<input type=number name=ccm_orsk value='" + String(g_cfg.ccm_order_risk) + "'>");
  return s;
}

static void applyConfigSensorForm(const String &body) {
  g_cfg.ccm_order_index = (int16_t)agri::parseFormInt(body, "ccm_oidx", g_cfg.ccm_order_index);
  g_cfg.ccm_order_risk  = (int16_t)agri::parseFormInt(body, "ccm_orsk", g_cfg.ccm_order_risk);
}

static void addStatusFields(JsonObject doc) {
  doc["sensor_ok"] = g_uv_ok;
  if (g_uv_ok) {
    doc["uv_index"] = g_uv_index;
    doc["uv_risk"]  = g_uv_risk;
    doc["uv_mv"]    = g_uv_mv;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n=== %s v%s ===\n", FW_NAME, FW_VERSION);

  agri::Led::begin();
  loadConfig();
  Serial.printf("[CFG] node=%s mqtt_host=%s ccm=%s\n",
                g_cfg.common.node_id,
                g_cfg.common.mqtt_host[0] ? g_cfg.common.mqtt_host : "(unset)",
                g_cfg.common.ccm_enabled ? "on" : "off");

  sensorsBegin();

  agri::Network::begin(g_cfg.common.hostname);
  agri::Network::waitForLease();

  agri::ccmBegin();
  agri::MQTT::begin();

  agri::WebHooks hooks;
  hooks.nodeTitle             = [](){ return FW_NAME; };
  hooks.renderDashboardSensors= renderDashboardSensors;
  hooks.renderConfigSensorRows= renderConfigSensorRows;
  hooks.applyConfigSensorForm = applyConfigSensorForm;
  hooks.addStatusFields       = addStatusFields;
  hooks.saveConfig            = [](){ saveConfig(); };
  agri::WebUI::begin(g_cfg.common, hooks, FW_NAME, FW_VERSION);

  agri::mdnsBegin(g_cfg.common.hostname);
  agri::otaBegin(g_cfg.common.hostname);

  agri::OTA::begin(FW_REPO, FW_BIN_NAME, FW_VERSION);
  agri::OTA::checkLatest();

  Serial.println("[BOOT] ready");
}

void loop() {
  agri::otaHandle();
  agri::OTA::poll();
  agri::WebUI::handle(agri::Network::link_up, agri::Network::have_lease);
  // No DHCP lease for a grace period (cable out / no LAN / just-unboxed) -> raise
  // a WPA2 SoftAP (SSID = hostname) serving the WebUI so the node can be
  // configured wirelessly. Torn down automatically once Ethernet gets a lease.
  agri::ProvisionAP::poll(agri::Network::have_lease, g_cfg.common.hostname);

  uint32_t now = millis();

  static uint32_t lastSensorPoll = 0;
  if (now - lastSensorPoll >= 1000) {
    lastSensorPoll = now;
    sensorsPoll();
  }

  if (agri::networkUp() && agri::MQTT::hasHost(g_cfg.common)) {
    if (!agri::MQTT::connected()) {
      static uint32_t lastTry = 0;
      if (now - lastTry > 5000) { lastTry = now; agri::MQTT::reconnect(g_cfg.common); }
    } else {
      agri::MQTT::loop();
      static uint32_t lastPub = 0;
      uint32_t interval = (uint32_t)g_cfg.common.mqtt_interval_s * 1000UL;
      if (now - lastPub >= interval) {
        lastPub = now;
        if (mqttPublishUv()) agri::Led::flashPublish();
      }
    }
  }

  if (agri::networkUp() && g_cfg.common.ccm_enabled) {
    static uint32_t lastCcm = 0;
    uint32_t interval = (uint32_t)g_cfg.common.ccm_interval_s * 1000UL;
    if (now - lastCcm >= interval) {
      lastCcm = now;
      if (ccmPublish()) agri::Led::flashPublish();
    }
  }

  agri::LedState desired;
  if (!agri::networkUp())                                                 desired = agri::LED_NO_LINK;
  else if (!g_uv_ok)                                                      desired = agri::LED_NO_SENSOR;
  else if (agri::MQTT::hasHost(g_cfg.common) && !agri::MQTT::connected()) desired = agri::LED_NO_MQTT;
  else                                                                    desired = agri::LED_OK;
  agri::Led::set(desired);

  static uint32_t lastStatus = 0;
  if (now - lastStatus >= 30000) {
    lastStatus = now;
    Serial.printf("[STATUS] link=%d lease=%d mqtt=%d uv=%d idx=%u risk=%u mv=%u up=%lus\n",
                  agri::Network::link_up, agri::Network::have_lease,
                  agri::MQTT::connected(), g_uv_ok, g_uv_index, g_uv_risk, g_uv_mv,
                  (unsigned long)(now / 1000));
  }

  delay(20);
}

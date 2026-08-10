// config.h — agri-uv-poe NVS-backed config.
//
// Two CCM channels: InUvIndex (0–11) and InUvRisk (0–4). No calibration —
// the SEN0636 reports UV Index and Risk Level directly over Modbus, so the
// only sensor-specific config is the CCM order for each channel.

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <AgriCommonConfig.h>

struct AppConfig {
  agri::CommonConfig common;
  int16_t            ccm_order_index;   // InUvIndex.<ntype>
  int16_t            ccm_order_risk;    // InUvRisk.<ntype>
};

extern AppConfig g_cfg;

inline void setDefaults() {
  agri::commonDefaults(g_cfg.common,
                       "uv_node_01", "agri-uv-01",
                       "agriha/h01/sensor/Uv",
                       /*default_ccm_region=*/11);
  g_cfg.ccm_order_index = 1;
  g_cfg.ccm_order_risk  = 2;
}

inline void loadConfig() {
  setDefaults();
  Preferences p;
  if (!p.begin("uv-cfg", true)) return;
  agri::commonLoad(g_cfg.common, p);
  g_cfg.ccm_order_index = p.getShort("ccm_oidx", g_cfg.ccm_order_index);
  g_cfg.ccm_order_risk  = p.getShort("ccm_orsk", g_cfg.ccm_order_risk);
  p.end();
}

inline bool saveConfig() {
  Preferences p;
  if (!p.begin("uv-cfg", false)) return false;
  agri::commonSave(g_cfg.common, p);
  p.putShort("ccm_oidx", g_cfg.ccm_order_index);
  p.putShort("ccm_orsk", g_cfg.ccm_order_risk);
  p.end();
  return true;
}

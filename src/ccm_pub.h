// ccm_pub.h — UV (InUvIndex / InUvRisk) UECS-CCM publisher.
//
// UECS has no standard type for UV, so we use custom names InUvIndex and
// InUvRisk with the configured node-type suffix (default → InUvIndex.cMC).
// ArSprout just receives them; the identifiers can change later without harm.

#pragma once

#include <Arduino.h>
#include <AgriNode.h>
#include "config.h"
#include "sensors.h"

inline bool ccmPublish() {
  if (!g_cfg.common.ccm_enabled || !g_uv_ok) return false;

  char idx[8]; snprintf(idx, sizeof(idx), "%u", g_uv_index);
  char rsk[8]; snprintf(rsk, sizeof(rsk), "%u", g_uv_risk);

  String xml = agri::ccmEnvelopeOpen();
  xml += agri::ccmDatumNT("InUvIndex", g_cfg.common.ccm_ntype,
                          g_cfg.common.ccm_room, g_cfg.common.ccm_region,
                          g_cfg.ccm_order_index, g_cfg.common.ccm_priority, idx);
  xml += agri::ccmDatumNT("InUvRisk", g_cfg.common.ccm_ntype,
                          g_cfg.common.ccm_room, g_cfg.common.ccm_region,
                          g_cfg.ccm_order_risk, g_cfg.common.ccm_priority, rsk);
  xml += agri::ccmEnvelopeClose();
  return agri::ccmSend(xml);
}

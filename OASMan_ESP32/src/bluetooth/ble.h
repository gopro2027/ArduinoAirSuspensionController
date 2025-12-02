#ifndef ble2_h
#define ble2_h

#include <Arduino.h>
#include "btstack.h"

#include "btstack_run_loop.h"
#include "btstack_run_loop_freertos.h"
#include "ble/att_db.h"
#include "btstack_util.h"
#include "att_db_util.h"
#include <string.h>

#include "hci_dump.h"
#include "hci_transport.h"
#include "hci_transport_h4.h"
#include "hci_transport_em9304_spi.h"

#include "hci.h"
#include "gap.h"
#include "l2cap.h"
#include "sm.h"
#include "att_server.h"

#include "btstack_defines.h"
#include "ble/att_db_util.h"

#include "airSuspensionUtil.h"
#include <BTOas.h>
#include "components/manifold.h"
#include "tasks/tasks.h"
#include <vector>

#include "oasman_service.gatt.h"

#include <unordered_map>
#include <set>

#include "bp32.h"

#ifdef WIFI_OTA_ENABLE
#include "webOTA/webOTA.h"
#endif

#include "directdownload.h"

void ble_setup();
void ble_loop();

#endif
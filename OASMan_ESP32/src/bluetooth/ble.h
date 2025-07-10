#ifndef ble2_h
#define ble2_h

#include <Arduino.h>
#include "btstack.h"
#include "btstack_run_loop.h"
#include "ble/att_db.h"
#include "btstack_util.h"
#include "att_db_util.h"
#include <string.h>

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

void ble_setup();
void ble_loop();

#endif
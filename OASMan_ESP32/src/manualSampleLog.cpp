// This file is 100% ai generated but it is completely containerized to this file other than the 2 exported functions in the header which are each used once.
// It's only intention is to log the start, end, tank, and timing for the manual up and down arrows. I think this code may be way overkill though.

#include "manualSampleLog.h"
#include <user_defines.h>
#include <math.h>
#include "airSuspensionUtil.h"    // getWheel / getManifold / getCompressor
#include "manifoldSaveData.h"     // recordLearnSample, getheightSensorMode
#include "components/manifold.h"  // Manifold::get
#include "components/solenoid.h"  // Solenoid::getAIIndex

#if USE_MANUAL_SAMPLE_LOGGING

// Producer (BLE callback thread) fills these at valve edges; consumer (owning wheel task) drains them in
// manualSampleServiceWheel. One spinlock guards the whole array (dual-core safe, tiny critical sections).
struct ManualValveState
{
    bool open = false;        // currently held open (we saw its open edge, not yet its close)
    bool pendingRead = false; // a completed move is waiting for its settle-read
    uint32_t openMs = 0;
    uint32_t closeMs = 0;
    uint32_t holdMs = 0;
    uint8_t startPsi = 0;     // captured at the open edge (cached read)
    uint16_t tankPsi = 0;     // captured at the open edge (cached settled tank)
    uint8_t others = 0;       // same-direction other corners open during this move
};

static ManualValveState g_manual[SOLENOID_COUNT];
static portMUX_TYPE g_manualMux = portMUX_INITIALIZER_UNLOCKED;

// IN valves (up/fill) are the even solenoid indices (bits 0,2,4,6); OUT (down/exhaust) are the odd ones.
static const unsigned int MANUAL_UP_MASK = (1u << FRONT_PASSENGER_IN) | (1u << REAR_PASSENGER_IN) | (1u << FRONT_DRIVER_IN) | (1u << REAR_DRIVER_IN);
static const unsigned int MANUAL_DOWN_MASK = (1u << FRONT_PASSENGER_OUT) | (1u << REAR_PASSENGER_OUT) | (1u << FRONT_DRIVER_OUT) | (1u << REAR_DRIVER_OUT);

static uint8_t countBits(unsigned int v)
{
    uint8_t c = 0;
    while (v)
    {
        c += (uint8_t)(v & 1u);
        v >>= 1;
    }
    return c;
}

void manualSampleOnValveEdge(int solenoidIndex, bool opening, unsigned int incomingBittset)
{
    if (solenoidIndex < 0 || solenoidIndex >= SOLENOID_COUNT)
    {
        return;
    }

    if (opening)
    {
        // Height-sensor mode reports bag "pressure" as a height %, not psi - not usable as an AI sample.
        if (getheightSensorMode())
        {
            return;
        }

        byte wheelNum = (byte)(solenoidIndex / 2);         // FP_IN/OUT->FP, RP_IN/OUT->RP, ...
        bool isUp = (solenoidIndex % 2) == 0;              // even index = IN valve = fill/up
        unsigned int mask = isUp ? MANUAL_UP_MASK : MANUAL_DOWN_MASK;

        // Cached reads only (no ADC) so this is safe on the BTstack callback. The corner and tank are
        // settled at press-start, so the cached values are accurate. The end pressure is read fresh after
        // the valve closes and settles (see manualSampleServiceWheel).
        uint8_t startPsi = (uint8_t)lroundf(getWheel(wheelNum)->getSelectedInputValue());
        uint16_t tankPsi = (uint16_t)lroundf(getCompressor()->getTankPressure());
        uint8_t sameDir = countBits(incomingBittset & mask);
        uint8_t others = sameDir > 0 ? (uint8_t)(sameDir - 1) : 0; // exclude this valve itself

        portENTER_CRITICAL(&g_manualMux);
        g_manual[solenoidIndex].open = true;
        g_manual[solenoidIndex].pendingRead = false; // abandon any prior unread close on a rapid re-press
        g_manual[solenoidIndex].openMs = millis();
        g_manual[solenoidIndex].startPsi = startPsi;
        g_manual[solenoidIndex].tankPsi = tankPsi;
        g_manual[solenoidIndex].others = others;
        portEXIT_CRITICAL(&g_manualMux);
    }
    else
    {
        portENTER_CRITICAL(&g_manualMux);
        if (g_manual[solenoidIndex].open)
        {
            uint32_t now = millis();
            g_manual[solenoidIndex].open = false;
            g_manual[solenoidIndex].closeMs = now;
            g_manual[solenoidIndex].holdMs = now - g_manual[solenoidIndex].openMs;
            g_manual[solenoidIndex].pendingRead = true;
        }
        portEXIT_CRITICAL(&g_manualMux);
    }
}

void manualSampleServiceWheel(byte wheelNum)
{
    for (int k = 0; k < 2; k++)
    {
        int sol = (int)wheelNum * 2 + k;
        if (sol < 0 || sol >= SOLENOID_COUNT)
        {
            continue;
        }

        // Claim a ready sample under the lock, then do the slow work (ADC read + SPIFFS write) outside it.
        bool ready = false;
        uint8_t startPsi = 0, others = 0;
        uint16_t tankPsi = 0;
        uint32_t holdMs = 0;

        portENTER_CRITICAL(&g_manualMux);
        if (g_manual[sol].pendingRead && (millis() - g_manual[sol].closeMs) >= MANUAL_SETTLE_MS)
        {
            g_manual[sol].pendingRead = false;
            ready = true;
            startPsi = g_manual[sol].startPsi;
            tankPsi = g_manual[sol].tankPsi;
            others = g_manual[sol].others;
            holdMs = g_manual[sol].holdMs;
        }
        portEXIT_CRITICAL(&g_manualMux);

        if (!ready || holdMs > MANUAL_MAX_HOLD_MS)
        {
            continue; // nothing pending, or a pathological held-open move we drop
        }

        SOLENOID_AI_INDEX aiIndex = getManifold()->get(sol)->getAIIndex();
        if (aiIndex >= AI_MODEL_UNDEFINED)
        {
            continue; // no model for this valve (should not happen for the 8 real solenoids)
        }

        getWheel(wheelNum)->readInputs();
        uint8_t endPsi = (uint8_t)lroundf(getWheel(wheelNum)->getSelectedInputValue());

        // recordLearnSample's own guards drop <=1 psi and wrong-direction moves and route file-vs-queue.
        recordLearnSample(aiIndex, startPsi, endPsi, tankPsi, holdMs, others);

        Serial.printf("Manual AI sample: model %i  {%u -> %u, tank %u, %ums, others %u}\n",
                      (int)aiIndex, startPsi, endPsi, tankPsi, holdMs, others);
    }
}

#else

void manualSampleOnValveEdge(int, bool, unsigned int) {}
void manualSampleServiceWheel(byte) {}

#endif

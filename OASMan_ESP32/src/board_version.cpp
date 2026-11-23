#include "board_version.h"
#include <user_defines.h>

BoardRevision boardRevision = BOARD_PRE_V5;
bool boardRevisionUnknown = false;
int boardRevisionMilliVolts = -1;

#ifdef BOARD_VERSION_5
// Nominal HW_VERSION mV per revision, index 0 = BOARD_V5_REV0; nearest slot wins
static const uint16_t boardRevisionNominalMilliVolts[] = {
    300, 356, 423, 489, 550, 632, 710, 792, 868, 963, 1063,
    1170, 1280, 1395, 1511, 1630, 1765, 1925, 2073, 2235, 2401};
static const int boardRevisionCount = sizeof(boardRevisionNominalMilliVolts) / sizeof(boardRevisionNominalMilliVolts[0]);
#define HW_VERSION_MIN_MV 150  // below this the divider is shorted or missing
#define HW_VERSION_MAX_MV 2480 // above this is past REV20's slot
#endif

void detectBoardRevision()
{
#ifdef BOARD_VERSION_5
    analogReadMilliVolts(HW_VERSION_PIN); // throwaway first read
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++)
    {
        sum += analogReadMilliVolts(HW_VERSION_PIN);
    }
    boardRevisionMilliVolts = sum / 16;

    if (boardRevisionMilliVolts < HW_VERSION_MIN_MV || boardRevisionMilliVolts > HW_VERSION_MAX_MV)
    {
        boardRevisionUnknown = true;
        boardRevision = BOARD_V5_REV0;
    }
    else
    {
        int best = 0;
        for (int i = 1; i < boardRevisionCount; i++)
        {
            if (abs(boardRevisionMilliVolts - (int)boardRevisionNominalMilliVolts[i]) < abs(boardRevisionMilliVolts - (int)boardRevisionNominalMilliVolts[best]))
            {
                best = i;
            }
        }
        boardRevision = (BoardRevision)(BOARD_V5_REV0 + best);
    }

    if (boardRevisionUnknown)
    {
        log_w("HW_VERSION %d mV matches no known board, falling back to V5 rev0", boardRevisionMilliVolts);
    }
    else
    {
        log_i("HW_VERSION %d mV -> V5 rev%d", boardRevisionMilliVolts, boardRevision - BOARD_V5_REV0);
    }
#else
    boardRevision = BOARD_PRE_V5;
#endif
}

#ifndef board_version_h
#define board_version_h

#include <Arduino.h>

// Ordered so checks like `boardRevision >= BOARD_V5_REV3` work; resistor table in PCB/ESP32_30PIN_PCB/HW_VERSION_revisions.md
enum BoardRevision : uint8_t
{
    BOARD_PRE_V5 = 0, // v2-v4 builds, HW_VERSION not read
    BOARD_V5_REV0,    // 0.300V, V5.0 board
    BOARD_V5_REV1,
    BOARD_V5_REV2,
    BOARD_V5_REV3,
    BOARD_V5_REV4,
    BOARD_V5_REV5,
    BOARD_V5_REV6,
    BOARD_V5_REV7,
    BOARD_V5_REV8,
    BOARD_V5_REV9,
    BOARD_V5_REV10,
    BOARD_V5_REV11,
    BOARD_V5_REV12,
    BOARD_V5_REV13,
    BOARD_V5_REV14,
    BOARD_V5_REV15,
    BOARD_V5_REV16,
    BOARD_V5_REV17,
    BOARD_V5_REV18,
    BOARD_V5_REV19,
    BOARD_V5_REV20,
};

extern BoardRevision boardRevision;
extern bool boardRevisionUnknown;  // reading matched no slot, boardRevision fell back to BOARD_V5_REV0
extern int boardRevisionMilliVolts; // raw HW_VERSION reading, -1 when not read

void detectBoardRevision();
inline BoardRevision getBoardRevision() { return boardRevision; }

#endif

#include "BAT_Driver.h"

float BAT_analogVolts = 0;

// No battery sense on this board -- see BAT_Driver.h.
void BAT_Init(void) {}

float BAT_Get_Volts(void)
{
  return BAT_analogVolts;  // always 0.0
}

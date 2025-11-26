#include "Buzzer.h"

void Buzzer_On(void)
{
    Set_EXIO(TCA9554_EXIO8,true);
}
void Buzzer_Off(void)
{
    Set_EXIO(TCA9554_EXIO8,false);
}
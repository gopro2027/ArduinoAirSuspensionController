#pragma once
namespace waveshare_3p5 {
  void waveshare_3p5_init(); // call in setup() BEFORE smartdisplay_init()
  void waveshare_3p5_loop(); // call in loop() if you use PMIC IRQ polling
}

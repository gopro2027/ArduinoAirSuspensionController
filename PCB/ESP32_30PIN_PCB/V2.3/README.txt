changelog:
Added accessory wire input and internal mosfet to turn ittself off so it can stay on for a bit after the car turns off

This version is not very recommended. If you can please wait until we have a new version that fixes the issues.
If you insist though:

1. You must do some slight modifications (deletions). The result is that the board will turn on with the accessory wire, but you won't be able to keep it on after the car has turned off. That means the only benefit of 2.3 over 2.2 is that you don't need an external relay (which is still great)
2. The modification if you must remove the 150R resistor furthest in the top right of the board (see discord accouncements for picture) You must also bend or cut off the d12 pin on your ESP32.
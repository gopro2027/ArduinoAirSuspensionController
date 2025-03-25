# Manifold Assemmbly

Get the 3d prints to print from [here](https://github.com/gopro2027/ArduinoAirSuspensionController/tree/main/3d%20Prints)<br>
There are 2 versions, Tall and Short. The tall version is standard with good room for everything to fit well inside of it. The short is 2cm shorter, does not fit the lcd screen, and is there just because, but is not particularly recommended. Required to put something like foam padding between the pcb and the manifold if you use the short version.<br>
Print them in a plastic with medium-high heat resistance such as PETG or ABS<br>
Print the lid upside down.<br>
For the hat please choose the version with or without the screen. Print upright<br>
The base prints upright<br>
<br>
You can attach the screen to the 3d printed hat with m2 screws. More info below.<br>
You can attach the pcb board to the 3d printed lid with m3 screws.<br>
<br>
On the manifold:<br>port 1 goes to front passenger (right)<br>port 2 goes to rear passenger (right)<br>port 3 goes to front driver (left)<br>port 4 goes to rear driver (left)<br>
The ports on the opposite side will correspond to the pressure sensors for the same bags. **Note:** If you are unsure which pressure sensor goes to which, just install them in any order and run the pressure sensor detection upon first startup.<br>
Please see the annotated diagram for a full breakdown<br>

![diagram](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/ebay_manifold_diagram.png)<br>
![3d render](https://github.com/user-attachments/assets/d2d7824f-42e0-421b-8ad5-1d28122957a1)
![physical pic](https://github.com/user-attachments/assets/8e0fe6bb-de3b-4c31-9984-bb5e53541474)

<br>
If you intend to utilize the built in lcd screen, follow these instructions:<br>
Solder the 4 wires and jst connector properly so that the wire names match the board.<br>
Loosely snug down the screen to the manifold, emphasis on loosely! Just barely snug. The screens are very fragile, particularly on the lower 2 corners of the screen.<br>
Once the screen is positioned, put some dabs of super glue on the inside nuts to hold it in place.<br>
Please note that when using the screen you will need to slightly bend back some of the mosfets in order to not have them touch the screen.<br>
<br>

![lcd screen](https://github.com/user-attachments/assets/3112dd67-7735-4416-a94f-17ceded052f8)<br>
<br>
To finish up the install, use the scews and nuts supplied with the manifold to screw it all to the 3d printed base.<br>
You can place the 4 nuts into the base and then screw them down through the top.<br>
Please only snug it enough as to not break any of the plastic.<br>
* Note if you are assembling the short version: I have become aware of an issue where the force to screw through the o-ring in the nut is enough to damage the plastic (PETG) in the base and spin the nut. This was fixed in the tall version by making it 0.5mm thicker. This was not fixed in the short version as it is an older model, so just take extra precaution when screwing it down to not damage it.<br>
<br>

![final build](https://github.com/user-attachments/assets/3c4304e9-fc82-4eab-ac79-86b6b12efc2b)<br>
<br>

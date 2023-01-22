# ArduinoAirSuspensionController
Air Suspension Controller

Parts list (screenshots in images folder):

Generic ebay manifold - $100 version wires only https://www.ebay.com/itm/324238773355<br>
Arduino nano (offbrand old bootloader works fine, many places to obtain)<br>
HC-06 Bluetooth https://www.amazon.com/gp/product/B074J5WMH1<br>
5v to 3v board https://www.amazon.com/gp/product/B07CP4P5XJ/<br>
i2c display https://www.amazon.com/gp/product/B09JWLDK9F/<br>
12v to 5v converter https://www.ebay.com/itm/234840680851<br>
5 300psi pressure sensors (links vary... make sure to check reviews some suck. 200psi also works just modify code)<br>
8 FQP30N06 Mosfet https://www.ebay.com/itm/153938539967<br>
8 Fuses https://www.ebay.com/itm/273505548256<br>
8 1N4007 Diode https://www.ebay.com/itm/393973959592?var=662531753553<br>
8 10k ohm resistors https://www.ebay.com/itm/324683051749?var=513694032962<br>
8 150 ohm resistors https://www.ebay.com/itm/324683051749?var=513694032918<br>
Also need loosely:<br>
Various wires<br>
Various pipes and connectrs<br>
Relay for turning the manifold on and off<br>
3D printer for hat and base<br>
Android device to control arduino<br>

Loose steps:
1. Order parts, order circuit board on JLPCB website, 3d print parts
2. Solder parts to circuit board, flip all switches to off
    1. For the manifold connector, wire the white wires (abcdefgh) in alphabetical order on the board, so 1 is a, 2 is b, and so on. The 9th wire goes to the spot right above the rest of the wires.
    2. The pressure sensors go in order... 1(manifold) -> FP (board) ... 4 -> RD. In general the order of everything is FRONT PASSENGER, REAR PASSENGER, FRONT DRIVER, REAR DRIVER abbreviatted FP, RP, FD, RD, sorry for the odd order it happened by accident
4. Configure code:
    1. get mac address of HC-06 bluetooth device by using an app or sommething to find it.
    2. Put the mac address in the android code, replacing the old mac address
    3. (Optional) Update the passwords in the app and arduino code if you want
    4. Write code to arduino
5. Circuit board single switch off, on the double switch make the top one on and bottom one off (this will power the arduino by 5v from the buck converter. 12 is bottom switch if you want to use that instead but you can ignore it)

Please feel free to reach out to me for help, I'd love to assist someone else in building it. The best way to contact me is to join my discord server https://discord.gg/N8uvR9PSuw and then send me a direct message gopro_2027#4805 , or shoot you shot and just send me a message directly without the server but I might not see it



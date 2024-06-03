# ArduinoAirSuspensionController
Air Suspension Controller

**Important Info To Potential Creators:** <br>
THIS IS THE NANO VERSION. All future nano based code (arduino and possibly android code) will be put here and the main branch will contain the code for the esp32. The nano version works fine but if you are able to wait for the esp32 version it would be recommended.
Future updates to the android app will be maintained on the main branch and may or may not work with the nano, but guarenteed the app on this branch will work on the nano.

**Overview:** <br>
This is intended as a DIY replacement for products such as Airlift 3P ($1500) or Airtek Stage 2+ ($1000) with a build cost of roughly $500. Combined with the customizability of open sourced code, I hope this is a tempting option for some DIY-ers out there. This github repo includes an android app and arduino code along with some 3d printable files and a PCB. The PCB is pre-made and ready for upload in JLCPCB to order and should hopefully make the electrinics/soldering learning curve a lot smaller for the average diy-er, as compared to having to solder every wire by scratch according to a diagram. This was designed as an upgrade from airtekk stage 1, which is just bags, tank, compressor, tubing and fittings, guages and manual paddle switches. So that is what you would need already prior to creating this, similarly to what you would need for one of the on-the-shelf manifolds listed above.

**Parts list (screenshots in images folder):** <br>
Generic ebay manifold - $100 version wires only https://www.ebay.com/itm/324238773355<br>
Arduino nano (offbrand old bootloader works fine, many places to obtain)<br>
HC-06 Bluetooth https://www.amazon.com/gp/product/B074J5WMH1<br>
5v to 3v board https://www.amazon.com/gp/product/B07CP4P5XJ/<br>
i2c display https://www.amazon.com/gp/product/B09JWLDK9F/<br>
12v to 5v converter https://www.ebay.com/itm/234840680851<br>
5 300psi pressure sensors (links vary... make sure to check reviews some suck. 200psi also works just modify code)<br>
8 FQP30N06L Mosfet https://www.ebay.com/itm/153938539967<br>
8 Fuses https://www.ebay.com/itm/273505548256<br>
8 1N4007 Diode https://www.ebay.com/itm/393973959592?var=662531753553<br>
8 10k ohm resistors https://www.ebay.com/itm/324683051749?var=513694032962<br>
8 150 ohm resistors https://www.ebay.com/itm/324683051749?var=513694032918<br>
Also probably want a 12v voltage stabalizer<br>
Also need loosely:<br>
Some pin headers to make the arduino easily replacable in case it gets fried accidentally<br>
Various wires<br>
Various pipes and connectrs<br>
Relay for turning the manifold on and off<br>
3D printer for hat and base<br>
Android device to control arduino<br>

**Loose steps:** <br>
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

**Optional but recommended:** <br>
If you have 2 arduino's I highly recommend you update the bootloader from the old bootloader to optiboot by following these instructions: https://www.youtube.com/watch?v=1TM-ADHb5Dk&ab_channel=DesignBuildDestroy
Currently, pin 13 is utilized and on the old bootloader this will cause the rear driver side air bag to briefly air out on startup. With the new bootloader, this issue is resolved.


![app_airsuspension](https://user-images.githubusercontent.com/7937950/236578835-0e3a208d-48cf-48e8-a882-4479f1afe35c.png)
![car_airsuspension](https://user-images.githubusercontent.com/7937950/236578918-bfa39ad6-a3b5-4d52-b36a-be34e8c608af.png)

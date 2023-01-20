# ArduinoAirSuspensionController
Air Suspension Controller

Parts list (screenshots in images folder):
Generic ebay manifold - $100 version wires only https://www.ebay.com/itm/324238773355
Arduino nano (offbrand old bootloader works fine, many places to obtain)
HC-06 Bluetooth https://www.amazon.com/gp/product/B074J5WMH1
5v to 3v board https://www.amazon.com/gp/product/B07CP4P5XJ/
i2c display https://www.amazon.com/gp/product/B09JWLDK9F/
12v to 5v converter https://www.ebay.com/itm/234840680851
5 300psi pressure sensors (links vary... make sure to check reviews some suck. 200psi also works just modify code)
8 FQP30N06 Mosfet https://www.ebay.com/itm/153938539967
8 Fuses https://www.ebay.com/itm/273505548256
8 1N4007 Diode https://www.ebay.com/itm/393973959592?var=662531753553
8 10k ohm resistors https://www.ebay.com/itm/324683051749?var=513694032962
8 150 ohm resistors https://www.ebay.com/itm/324683051749?var=513694032918
Also need loosely:
Various wires
Relay for turning the manifold on and off
3D printer for hat and base
Android device to control arduino

Loose steps:
1. Order parts, order circuit board on JLPCB website, 3d print parts
2. Solder parts to circuit board, flip all switches to off
3. Configure code:
  1. get mac address of HC-06 bluetooth device by using an app or sommething to find it.
  2. Put the mac address in the android code, replacing the old mac address
  3. (Optional) Update the passwords in the app and arduino code if you want
  4. Write code to arduino
4. Circuit board single switch off, on the double switch make the top one on and bottom one off (this will power the arduino by 5v from the buck converter. 12 is bottom switch if you want to use that instead but you can ignore it)



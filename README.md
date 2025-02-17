# Open Air Suspension Management (OAS-Man)
![image](https://github.com/user-attachments/assets/2990d62f-51a5-47d8-a54a-e71efa27a2cd)
<!---![image](https://github.com/user-attachments/assets/ea535981-4f51-4782-8f93-9eb1126dc81b)-->


<a href="https://discord.gg/pUf7FmHKpg">
Please click here to join the discord server!!!<br>
<img src="https://github.com/user-attachments/assets/30a534ca-611c-4ea8-afde-941e5e2d78f3" alt="DiscordLogo" >
</a> 
<br>
<br>

### Important Info To Potential Creators:
V2 (ESP32 version) is almost ready. Some testing has been done, and my car has successfully driven some miles on it. Tweaks and software changes to come. It is more-or-less ready to build, please join the discord for more information.<br>

### Overview:
This is intended as a DIY replacement for products such as Airlift 3P(3H*) ($1500) or Airtek Stage 2+ ($1000) with a build cost of less than $500. Combined with the customizability of open sourced code, I hope this is a tempting option for some DIY-ers out there.<br>
Our goal with this project is to provide the car community with a modern fully customizable well documented air suspension system with a budget build cost in mind, all not bound by the limitations of a single company. The rest of your car is custom to you, why not your air suspension too?<br>
- You want your car in the background? You got it.<br>
- You want to put your logo on the manifold? You got it.<br>
- You want your controller to look like a gameboy? Sure why not! **<br>
- Proprietary software broke and there's no update? Not an issue here.<br>
- Sensor broke in your proprietary manifold and you have to buy a whole new one for $1000? Not an issue here, just replace the sensor for $10.<br>
- Want to use bespoke heavy duty valves but also have a nice user interface? Look no further than OAS-Man!<br>
- Control your system with a PS3 controller? Yep, [we have that](https://www.youtube.com/shorts/fbXJVwzc6P0)<br>
- Restomodding an old air suspension system? OAS-Man may be the solution!<br>

This github repo includes an android app and arduino code along with some 3d printable files and a PCB. The PCB is pre-made and ready for upload in JLCPCB to order. You will still need to supply the rest of your system which is not covered in this project, such as struts/bags, tank, compressor, tubing and fittings, ect. So that is what you would need in addition to this, similarly to what you would need for one of the on-the-shelf manifolds. If you are unsure what to purchase, I suggest getting the `Airtekk Stage 1 Kit` as that will just about cover everything you need.<br>
<sub>* height sensors not yet supported in software. ** physical controller not yet designed</sub><br>

### Branch Information:
**main branch = ESP32** (Current full version of OASMan)<br>
**dev branch = ESP32** (Bleeding edge updates to OASMAN)<br>
**nano-v1 branch = Arduino Nano** (outdated, no longer maintained, only exists for legacy purposes)<br>
**Other branches = current dev work** (Other dev work which you can ignore)<br>
<br>
### Folder Information:
**tutorial** - Contains important build instructions (must read)<br>
**OASMan_ESP32** - The main board project<br>
**Wireless_Controller** - The project for the wireless controller of the main board<br>
**builds** - Mobile App Builds (apk's ect)<br>
**ESP32_SHARED_LIBS** - Contains files used between the main board and controller projects. Notably the config file which you may or may not need to edit.<br>
**LCDImageCreatorTool** - Code generator for making images for the tiny 128x64 screen on the manifold.<br>
**MobileApp** - New flutter mobile app using BLE<br>
**AirSuspension** - Legacy android app using traditional bluetooth. Will be deleted in the future.<br>
**3d Prints** - Contains files for 3d printing (more info in tutorials)<br>
**PCB** - PCB files. More info in tutorial<br>
**PS3_Controller_Tool** - Contains instructions & exe for setting the MAC address of your ps3 controller to work with OASMan<br>
**photos** - photos<br>
<br>

### Please [click here to view the comprehensive tutorial](/tutorial/README.md) documentation!<br>
<br>

<!---
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
-->
![OAS-Man Final v2](https://github.com/user-attachments/assets/7cc1af3d-1113-4094-8ff5-7ee16f282eb0)
![manifold](https://github.com/user-attachments/assets/d93784e5-7e5e-4bb0-891a-8a2a8e4d4da0)
![app_airsuspension](https://user-images.githubusercontent.com/7937950/236578835-0e3a208d-48cf-48e8-a882-4479f1afe35c.png)
<!--![car_airsuspension](https://user-images.githubusercontent.com/7937950/236578918-bfa39ad6-a3b5-4d52-b36a-be34e8c608af.png)-->
![oasman mascot car](https://github.com/user-attachments/assets/aef9e896-0be0-4203-92d2-81836c27fd5d)

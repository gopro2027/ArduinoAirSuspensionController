# Open Air Suspension Management (OAS-Man)
[![image](https://github.com/user-attachments/assets/2990d62f-51a5-47d8-a54a-e71efa27a2cd)](https://oasman.dev) 
<!---![image](https://github.com/user-attachments/assets/ea535981-4f51-4782-8f93-9eb1126dc81b)-->


###  [ <img src="https://github.com/user-attachments/assets/d6edf30f-0ae1-477c-ac8e-d6a4e7444bd0" alt="discord icon" width="40"/> Please click here to join the Discord server](https://discord.gg/pUf7FmHKpg) 
###  [ <img src="https://github.com/user-attachments/assets/04ea84f8-5977-46fa-a044-c5cdf82e4bf3" alt="tutorial icon" width="40"/> Click here to view the comprehensive build tutorial and documentation!](https://oasman.dev)
###  [ <img src="https://github.com/user-attachments/assets/82590a11-25d7-4a6f-9b23-a3656df55c48" alt="update icon" width="40"/> Click here for the oasman installer/update page](https://oasman.dev/flash)

### Important Info To Potential Creators:
Please join the discord for more specific information before you built it!<br>
There may be important information to share before you begin purchasing parts.<br>

### Overview:
This is intended as a DIY replacement for products such as Airlift 3P/3H ($1500) or Airtek Stage 2+ ($1000) with a build cost of less than $500. Combined with the customizability of open sourced code, I hope this is a tempting option for some DIY-ers out there.<br>
Our goal with this project is to provide the car community with a modern fully customizable well documented air suspension system with a budget build cost in mind, all not bound by the limitations of a single company. The rest of your car is custom to you, why not your air suspension too?<br>
- Want to customize the app to match your ride? You got it.<br>
- Did your proprietary air suspension software stop working and there's no update? Not an issue here!<br>
- Sensor broke in your proprietary manifold and you have to buy a whole new one for $1000? Not an issue here, just replace the sensor for $10.<br>
- Want to use bespoke heavy duty valves but also have a nice user interface? Look no further than OAS-Man!<br>
- Control your system with a PS3/PS4/Xbox controller? Yep, [we have that](https://www.youtube.com/shorts/fbXJVwzc6P0)<br>
- Restomodding an old air suspension system? OAS-Man may be the solution!<br>
- Literally any idea you've ever wanted to try with air suspension? OASMan is the best place to start.<br>

This github repo includes an android app<sup>1</sup> and esp32 code along with some 3d printable files and a PCB. The PCB is pre-made SMD and ready for upload in JLCPCB with assembly so minimal-to-no soldering required. There is also a pre-made touch screen device you can order ($25 to $40) which is used as the dedicated controller<sup>2</sup>. There is an easy to use webpage to upload the software so there is no coding required. You will still need to supply the rest of your system which is not covered in this project, such as struts/bags, tank, compressor, tubing and fittings, ect. So that is what you would need in addition to this, similarly to what you would need for one of the on-the-shelf manifolds. If you are unsure what to purchase, I suggest referencing (or purchasing!) the [Airtekk Stage 1 Kit](https://www.airtekk.com/product-p/uni-stage1-kit.htm) as that will just about cover everything you need.<br>
<sub>1. The mobile app is behind on development and lacks various features of the controller software, and currently only has an android release. Recommended to use the touch screen controller for full functionality.</sub><br>
<sub>2. Please view the tutorial and build instructions for more info.</sub>

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
**MobileApp** - New flutter mobile app using BLE (incomplete at the time of this writing)<br>
**3d Prints** - Contains files for 3d printing (more info in tutorials)<br>
**PCB** - PCB files. More info in tutorial<br>
**PS3_Controller_Tool** - Contains instructions & exe for setting the MAC address of your ps3 controller to work with OASMan<br>
**photos** - photos<br>
<br>

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
<!--![wireless_contoller](https://github.com/user-attachments/assets/ef3c085b-e8ef-4365-bd6b-093a90eec54d)
![manifold](https://github.com/user-attachments/assets/d93784e5-7e5e-4bb0-891a-8a2a8e4d4da0)
![app_airsuspension](https://user-images.githubusercontent.com/7937950/236578835-0e3a208d-48cf-48e8-a882-4479f1afe35c.png)-->
<!--![car_airsuspension](https://user-images.githubusercontent.com/7937950/236578918-bfa39ad6-a3b5-4d52-b36a-be34e8c608af.png)-->
<!--![oasman mascot car](https://github.com/user-attachments/assets/aef9e896-0be0-4203-92d2-81836c27fd5d)-->

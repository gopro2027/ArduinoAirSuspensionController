# Open Air Suspension Management (OAS-Man)
![image](https://github.com/user-attachments/assets/2990d62f-51a5-47d8-a54a-e71efa27a2cd)
<!---![image](https://github.com/user-attachments/assets/ea535981-4f51-4782-8f93-9eb1126dc81b)-->

<a href="https://discord.gg/pUf7FmHKpg">
Please click here to join the discord server!!!<br>
<img src="https://seeklogo.com/images/D/discord-logo-134E148657-seeklogo.com.png" alt="DiscordLogo" width="100" height="100">
</a> 

<br>
<br>

### Important Info To Potential Creators:
We are in the process of making the new ESP32 version, aka V2! Please join the discord for more information before you start your build.<br>

### Overview:
This is intended as a DIY replacement for products such as Airlift 3P(3H*) ($1500) or Airtek Stage 2+ ($1000) with a build cost of less than $500. Combined with the customizability of open sourced code, I hope this is a tempting option for some DIY-ers out there.<br>
Out goal with this project is to provide the car community with a modern fully customizable well documented air suspension system with a budget build cost in mind, all not bound by the limitations of a single company. The rest of your car is custom to you, why not your air suspension too?<br>
- You want your car in the background? You got it.<br>
- You want to put your logo on the manifold? You got it.<br>
- You want your controller to look like a gameboy? Sure why not! **<br>
- Proprietary software broke and there's no update? Not an issue here.<br>
- Sensor broke in your proprietary manifold and you have to buy a whole new one for $1000? Not an issue here, just replace the sensor for $10.<br>
- Want to use bespoke heavy duty valves but also have a nice user interface? Look no further than OAS-Man!<br>
- Restomodding an old air suspension system? OAS-Man may be the solution!<br>

This github repo includes an android app and arduino code along with some 3d printable files and a PCB. The PCB is pre-made and ready for upload in JLCPCB to order. You will still need to supply the rest of your system which is not covered in this project, such as struts/bags, tank, compressor, tubing and fittings, ect. So that is what you would need in addition to this, similarly to what you would need for one of the on-the-shelf manifolds. If you are unsure what to purchase, I suggest getting the `Airtekk Stage 1 Kit` as that will just about cover everything you need.<br>
<sub>* height sensors not yet supported in software. ** physical controller not yet designed</sub><br>

### Branch Information:
**main branch = ESP32** (Current full version of OASMan)<br>
**nano-v1 branch = Arduino Nano** (outdated, no longer maintained)<br>
**Other branches = current dev work** (Other dev work which you can ignore)<br>
<br>
Additionally, the android app code does not have as strict designation and will be kept updated on the main branch however will likely work on the nano (with the exception of new bluetooth commands not supported on the nano) until otherwise stated.

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


![app_airsuspension](https://user-images.githubusercontent.com/7937950/236578835-0e3a208d-48cf-48e8-a882-4479f1afe35c.png)
![car_airsuspension](https://user-images.githubusercontent.com/7937950/236578918-bfa39ad6-a3b5-4d52-b36a-be34e8c608af.png)

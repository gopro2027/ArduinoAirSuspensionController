# PS3 Controller Setup
### Setup:
If you plan to use a ps3 controller, use the sixaxis pair tool to change your mac address of your PS3 controller to your ESP32's mac address<br>
You can use a phone app called nRF Connect to obtain the mac address of the system<br>
<br>
![Screenshot_20250322_171036_nRF Connect (2)](https://github.com/user-attachments/assets/0a26b60f-f8ef-4f27-9dff-c2e1ca890823)<br>
<br>
![example](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/refs/heads/main/PS3_Controller_Tool/SixaxisPairTool_rename_example.png)<br>
<a href="https://github.com/gopro2027/ArduinoAirSuspensionController/raw/refs/heads/main/PS3_Controller_Tool/SixaxisPairToolSetup-0.3.1.exe">Download Sixaxis Pair Tool here</a><br>
<br>
If using the legacy android app (unlikely), the PS3 controller must be connected before the app is connected in order to function properly! You can have both connected at the same time as long as you connect the controller first.<br>
<br>
Additional information that explains why the ps3 controller is special: <a href="https://bluepad32.readthedocs.io/en/latest/pair_ds3/">https://bluepad32.readthedocs.io/en/latest/pair_ds3/</a><br>
<br>
### Controls:
**L1 + R1**: enable/disable the controller (enabled by default)<br>
**Start + Select**: turn off controller (this actually reboots the manifold which causes the controller to turn off)<br>
<br>
Button inputs: (not really useful on the controller but they are there anyways)<br>
**X**: goto preset 1<br>
**Square**: goto preset 2<br>
**Triangle**: goto preset 3<br>
**Circle**: goto preset 4<br>
**Dpad Up**: air up to current preset<br>
**Dpad Down**: air out<br>
<br>
Joystick inputs:<br>
**left joystick - left**: air up left side of car<br>
**left joystick - right**:  air out left side of car<br>
**left joystick - up**: air up front side of car<br>
**left joystick - down**: air down front side of car<br>
**right joystick - left**: air out right side of car<br>
**right joystick - right**: air up right side of car<br>
**right joystick - up**: air up rear side of car<br>
**right joystick - down**: air down rear side of car<br>
<br>
Your battery level is also indicated by how many led's show up on the top of the controller.<br>
The lights will flash periodically when armed (enabled), and be solid when unarmed (disabled).<br>
4 led's = full<br>
3 led's = high<br>
2 led's = low<br>
1 led = dying<br>
0 led's = shutting down<br>
Moving led's = charging<br>

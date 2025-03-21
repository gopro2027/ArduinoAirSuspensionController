# PS3 Controller Setup

If you plan to use a ps3 controller, use the sixaxis pair tool to change your mac address of your PS3 controller to your ESP32's mac address (Tip: You can get the mac from the bluetooth page of the android app!)<br>
![example](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/refs/heads/main/PS3_Controller_Tool/SixaxisPairTool_rename_example.png)<br>
If using the legacy android app, the PS3 controller must be connected before the app is connected in order to function properly! You can have both connected at the same time as long as you connect the controller first.<br>
<br>
Controls:<br>
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

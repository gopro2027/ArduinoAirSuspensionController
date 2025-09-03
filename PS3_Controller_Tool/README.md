# Videogame Controller Usage
OASMan implements a library called [Bluepad32](https://bluepad32.readthedocs.io/en/latest/) in order to allow a multitude of controllers to connect to our air suspension system!
<img width="512" height="384" alt="image" src="https://github.com/user-attachments/assets/9d18df13-a463-4ef8-9556-bc83d4e6a9f8" />
<br>
Thank you to [ricardoquesada](https://github.com/ricardoquesada/bluepad32) for the creation of this library!

### How to pair a controller
DISCLAIMER: Always store controllers in a safe location when driving so they do not get enabled accidentally while driving. Do not use controllers while driving. Use at your own risk.
1. Put your car in park for pairing. Pairing a new controller may potentially disrupt bluetooth signal, and in the worst case, cause oasman to reboot. This shouldn't pose any safety issues, but please be careful.
3. You need to go into your oasman touch screen control and go to settings and click "Allow New Controller". This will put oasman into pairing mode.
4. Grab your controller and follow the manufacturers instructions on how to put it into pairing/sync mode. You can also find instructions for this on [bluepad32's wiki](https://bluepad32.readthedocs.io/en/latest/supported_gamepads/)
Sometimes there may be issues during syncing new controllers. You may find better results if you disconnect other controllers before trying to sync a new controller.

Once your controller is paired, to reconnect it again, simply turn it on like normal and oasman will accept the connection.

There is an arbitrarily chosen max of 20 paired controllers in OASMan. You can reset all paired controllers in the settings page of the touch screen controller.

Oasman technically supports multiple controllers being connected at the same time.

Not all controllers are supported, such as keyboards, but I did specifically add [support for the wii fit balance board](https://www.instagram.com/p/DNy50Yu3hv6/)

### Special instructions for PS3 Controllers
If you plan to use a ps3 controller, use the sixaxis pair tool to change your mac address of your PS3 controller to your ESP32's mac address<br>
You can use a phone app called nRF Connect to obtain the mac address of the oasman system<br>
<br>
<img height="300" alt="image" src="https://github.com/user-attachments/assets/0a26b60f-f8ef-4f27-9dff-c2e1ca890823" />
<img height="300" alt="image" src="https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/refs/heads/main/PS3_Controller_Tool/SixaxisPairTool_rename_example.png" />

<a href="https://github.com/gopro2027/ArduinoAirSuspensionController/raw/refs/heads/main/PS3_Controller_Tool/SixaxisPairToolSetup-0.3.1.exe">Download Sixaxis Pair Tool here</a><br>
<br>
Additional information that explains why the ps3 controller is special and linux setup: <a href="https://bluepad32.readthedocs.io/en/latest/pair_ds3/">https://bluepad32.readthedocs.io/en/latest/pair_ds3/</a><br>
<br>
### Controller Controls
**L1 + R1**: arm/disarm the controller (armed by default)<br>
**System Button (aka playstation button or xbox button)**: disconnect controller<br>
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
**Controllers with only one joystick**: Devices such as the nintendo switch joycon, you can press and hold the **B** button to make your joystick swap from one side to the other while holding it.<br>
<br>
**Wii Fit Balance Board**: Left side controls front of car, right side controls rear of car<br>
<br>
There is also an easter egg. See if you can find it!<br>

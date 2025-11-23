# Wiring
### PCB assembly

The latest version of the PCB is SMD and intended to be assembled by the factory that assembles the PCB so no board assembly is required. Just place in the ESP32.<br>

<!--The pcb assembly is relatively straightforward when following as marked on the board.<br>
If you have any questions, please feel free to ask on the discord server.<br>
If you are wondering which soldering iron to use, I personally recommend using a TS100 soldering iron and some led solder.<br>
Here is a video of me assembling my 2.0 board, if you wish to follow along:<br>
<br>
[OAS-Man (Open Air Suspension Management) PCB Assembly<br>![OAS-Man (Open Air Suspension Management) PCB Assembly](https://github.com/user-attachments/assets/1cabc97b-822e-4c43-a45e-32a945192d54)](https://www.youtube.com/watch?v=XGFra2Tvlkg&ab_channel=gopro_2027)
<br> -->

### I/O Pin Wiring
For your main 6 pin i/o:<br>
For the power wires and compressor and auxillary wires you will want to use a 6 wire bundled 18 guage wire as listed in the parts<br>
Attach all 6 wires from the power connector, one to constant 12v battery, one to ground, and one to the accessory wire which turns on with the key<br>
Suggested / typical color coding:
- Red: 12v battery
- Black: ground
- Yellow: ACC
- White: Compressor
- Orange: Auxillary (Optional)
- Green: E-brake (Optional)

The compressor output should be wired to a relay as follows:
- Relay pin 86: Wire to the oasman board compressor output, 18 guage
- Relay pin 85: Wire to cars ACC wire (or constant 12v), 18 guage
- Relay pin 30: Wire to cars battery with a wire thich enough to handle the amps of the compressor.
- Relay pin 87: Wire to compressor positive wire.
Please make sure you have the proper fuses for the compressor and battery wires.

<sup>* Note: Although 85 typically goes to gnd, if we wired it this way the relays internal flyback diode dumps to the compressor and causes the compressor to briefly blip on upon shutdown. We instead wire it as mentioned above and utilize the oasman boards internal flyback diode to not cause this issue with the compressor.</sup><br>
<sup>See video [here](https://www.youtube.com/watch?v=QAD8vDHTbzo&ab_channel=WiringRescue) for additional wiring info on a negative triggered relay, where the oasman compressor output wire is acting as the switch in this video.</sup>

For the Auxillary wire, it is optional. But you would ideally connect it to a relay in the same manner that you could with the compressor output. You may use the auxillary wire to control anything you'd like from your wireless oasman controller, such as an electric water trap or a light bar. Completely optional though!

The e-brake wire is also optional. It should be connected so that when you pull the e-brake, this wire is connected directly to gnd.
<!-- Note to self: my car is this but im not sure it's correct even though it works: 86 goes to manifold compressor output, 85 goes to +12v battery, 30 to gnd, 87 to compressor -->
<br>

![image](https://github.com/user-attachments/assets/50b68f7a-0a54-43dc-b1a4-f5247a3677fd)




### Pressure and Height Sensor Wiring
Both the pressure and height sensors use 3 wires each. GND, 5V, and Data.<br>
Please attach 3 pin JST connectors to each so that they align with the same wires on the PCB.<br>
Obviously the height sensors are totally optional.<br>
If you are using the listed 5 sensor kit and choose to not cut off the oem connector and re-pin them to 3 pin, the board supports that too, it has a special connector (grey 8 pin connector in the pic below) just for compattability with this sensor kit. Just remember to run the pressure sensor detection after install to detect which sensors go to which wheels!<br>
<br>

![image](https://github.com/user-attachments/assets/ee75aa04-6dc4-40bc-8031-bf125134489d)


### Manifold Wiring Harness Assembly (Valvetable)
#### Summary
The wiring was designed to be as easy as we could possibly make it. The pins on the PCB are 1 to 1 with the pins on the recommended manifold's connector. However, still, this is the most difficult part of the whole project so please pay attention, especially if you are running a different manifold.<br>
#### Brief overview
The board mimics the wires coming out of the manifold connector. If you imagine hanging the connector above the pcb, straightening out all the wires so they don't cross, then connecting them, that is the order in which the wires need to be connected.
The 3 top wires go in the same order on a 3 pin jst connector, and the 6 bottom wires go in the same order on a 6 pin jst connector. The only catch is to make sure you dont't accidentally put the connector upside down. <br>Simple?<br>Alright, lets get into the details...<br><br>
#### Detailed Tutorial
So the wiring harness is split up into 2 jst connectors, because it's a 2x6 connector but 3 of the pins are not used.<br>
So on the one half with the 3 pins in a row and 3 unused pins, we will connect them to a 3 pin jst connector in the same order (no crossed wires) as so:<br>
<br>
![3 pin on manifold harness](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/3pins.jpeg)<br>
<br>
Please note the orientation of the connectors so that you don't accidentally have the order flipped. You want to make sure the connector on the board connects to the wires in the same order and not upside down. If you follow exactly as the pictures on this page show, you should be fine.<br>
You can double check all of the wiring by cross referencing the information supplied in the images following and labels on the PCB.<br>
The other half of the harness is 6 pins in a row, so we will connect all 6 to a 6 pin jst connector, also in the same order with no wires crossed. Again making sure to not have the connector upside down in respect to how you have it soldered on the board.<br>
<br>
![6 pin on manifold harness](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/6pins.jpeg)<br>
<br>
You can see for example how the #6 pin on the pinout is connected to the spot that will plug into the RDO on the PCB from the image shown next.<br>
If attached as shown in the video, they should connect to the board with the other side of the jst connector in this orientation:<br>
<br>
![valvetable jst orientation on board](https://github.com/user-attachments/assets/80ee6d81-c4e6-4dda-82e5-b8fba95e1eb9)<br>
<br>
<br>
Here is the full pinout of wiring harness that comes with the manifold for a full cross reference:<br>
<sub>* Note that this is looking into the connector from the female side, with the holes</sub><br>
<br>
![manifold connector wiring cross reference](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/pcb_valvetable_pinout.png)<br>
<br>
And here is my annotated documentation that may also be of use for a double check:<br>
<sub>* Note that this is looking into the connector from the male side, with the wires</sub><br>
<br>
![manifold documentation annotated](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/ebay_manifold_diagram.png)<br>
<br>
The final result will look something like this:<br>
<br>
![wiring harness final result](https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/main/photos/esp32/ValvetableAndManifold/manifold_final_wiring.jpg)<br>
<br>
After completing all the jst connectors, I put hot glue on the ends of them to keep the wires from being stressed too much when moved around and assembled:<br>
![20241210_125036](https://github.com/user-attachments/assets/10d32557-2c95-4b84-b409-b76db983f35b)<br>
<br>

### Congratulations on assembling the wiring!


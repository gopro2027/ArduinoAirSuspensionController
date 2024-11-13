# Wiring
### PCB assembly
The pcb assembly is relatively straightforward when following as marked on the board.<br>
If you have any questions, please feel free to ask on the discord server.<br>
If you are wondering which soldering iron to use, I personally recommend using a TS100 soldering iron and some led solder.<br>
Here is a video of me assembling my board, if you wish to follow along:<br>
<br>
[OAS-Man (Open Air Suspension Management) PCB Assembly<br>![OAS-Man (Open Air Suspension Management) PCB Assembly](https://img.youtube.com/vi/XGFra2Tvlkg/0.jpg)](https://www.youtube.com/watch?v=XGFra2Tvlkg&ab_channel=gopro_2027)
<br>

### Pressure and Height Sensor Wiring
Both the pressure and height sensors use 3 wires each. GND, 5V, and Data.<br>
Please attach 3 pin JST connectors to each so that they align with the same wires on the PCB.<br>
Obviously the height sensors are totally optional.<br>
<br>

### Manifold Wiring Harness Assembly (Valvetable)
The wiring was designed to be as easy as possible. The 3 top wires go in the same order on a 3 pin jst connector, and the 6 bottom wires go in the same order on a 6 pin jst connector. The only catch it to make sure it doesn't connect to the board upside down (aka reverse order) or you will have to cut the pins off and flip the connector over and insert them in the reverse order.<br>Simple?<br>Alright, lets get into the details...<br><br>
So the wiring harness is split up into 2 parts, because it's a 2x6 connector but 3 of the pins are not used.<br>
So on the one half with the 3 pins in a row and 3 unused pins, we will connect them to a 3 pin jst connector in the same order (no crossed wires) as so:<br>
<br>
![3 pin on manifold harness](/photos/esp32/ValvetableAndManifold/3pins.jpeg)<br>
<br>
Please note the orientation of the connectors so that you don't accidentally have the order flipped. You want to make sure the connector on the board connects to the wires in the same order and not upside down. If you follow exactly as the pictures on this page show, you should be fine.<br>
You can double check all of the wiring by cross referencing the information supplied in the images following and labels on the PCB.<br>
The other half of the harness is 6 pins in a row, so we will connect all 6 to a 6 pin jst connector, also in the same order with no wires crossed. Again making sure to not have the connector upside down in respect to how you have it soldered on the board.<br>
<br>
![6 pin on manifold harness](/photos/esp32/ValvetableAndManifold/6pins.jpeg)<br>
<br>
You can see for example how the #6 pin on the pinout is connected to the spot that will plug into the RDO on the PCB from the image shown next.<br>
If attached as shown in the video, they should connect to the board with the other side of the jst connector in this orientation:<br>
<br>
![valvetable jst orientation on board](/photos/esp32/ValvetableAndManifold/board_jst_placement.png)<br>
<br>
All of the JST connectors on the board, with the exception of the oled screen JST which is sideways, will be installed in this orientation with the clip side pointing 'up' in respect to having the board placed as so the text is right side up and readable.<br>
<br>
Here is the full pinout of wiring harness that comes with the manifold for a full cross reference:<br>
<br>
![manifold connector wiring cross reference](/photos/esp32/ValvetableAndManifold/pcb_valvetable_pinout.png)<br>
<br>
And here is my annotated documentation that may also be of use for a double check:<br>
<br>
![manifold documentation annotated](/photos/esp32/ValvetableAndManifold/ebay_manifold_diagram.png)<br>
<br>
The final result will look something like this:<br>
<br>
![wiring harness final result](/photos/esp32/ValvetableAndManifold/manifold_final_wiring.jpg)<br>
<br>
### Congratulations on assembling the wiring!

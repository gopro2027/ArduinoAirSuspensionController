# Parts list

Here are the parts required to build the system.

### PCB Parts (For the SMD pcb)

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| Main PCB | 1 | ![image](https://github.com/user-attachments/assets/0d5fbee0-e9ce-47ec-bf6a-8231146091f3) | [files](https://github.com/gopro2027/ArduinoAirSuspensionController/tree/main/PCB/ESP32_30PIN_PCB/) | The new pre assembled SMD pcb cost a decent amount to order straight from JLCPCB so join the discord and find a reseller to pick one up cheaper than ordering the minimum qty from jlcpcb directly if possible. Please read the readme files! As of 4/29/2025 the latest version is 3.2 |
| ESP32 30pin |     1    | ![image](https://github.com/user-attachments/assets/a30c7c9f-4fdf-4c74-ace4-a51401cb46fd) | [amazon](https://www.amazon.com/Teyleten-Robot-ESP-WROOM-32-Development-Microcontroller/dp/B08246MCL5/) [aliexpress](https://www.aliexpress.us/item/3256806150650156.html) | The project currently uses an esp32 wroom 30pin |
| JST-XH 2.54mm  |    Assorted (2-6)     | ![image](https://github.com/user-attachments/assets/f83d2230-f445-4e51-9718-cc29dfb26ecd) | [amazon](https://www.amazon.com/Connector-Housing-Adapter-Terminal-Connectors/dp/B0BZDCGJ32/) | You will need multiple of each size, so it's best to purchase an assorted pack that includes 2,3,4, and 6.<br> 4pin x 1 for main i/o<br>4pin x 1 for screen<br>3pin x 11 for valvetable, pressure sensors, height sensors, power<br> 6pin x 1 for other half of valvetable |
| 3a fuse | 1 | ![image](https://github.com/user-attachments/assets/53e73bb0-d1ce-4a3c-92dd-98fcaf59c8d1) | - | 3 amp fuse for the board |
| 128x64 I2C OLED |     1    | ![image](https://github.com/user-attachments/assets/6c5eb13b-6e0c-4c4f-b4ad-c075ef5d964e) | [amazon](https://www.amazon.com/gp/product/B09JWLDK9F/) [aliexpress](https://www.aliexpress.com/item/1005005241315177.html) | -*Optional*- Depending on which one you purchase, you may need to change the i2c address in the esp32 configuration file. Some have solderable address configuration on the back of the screen/board |

<!--| 5x20 3a fuse |   1      | ![image](https://github.com/user-attachments/assets/eb0c0166-c211-451b-9961-f7c7b3954020) | [lcsc fuse](https://www.lcsc.com/product-detail/Disposable-fuses_Reomax-STC1300_C5381727.html) | Fuse for the board |-->

### Controller Parts
| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| ESP32-2432S032C | 1 | ![image](https://github.com/user-attachments/assets/ebe637e1-e9b9-4a58-8b3b-ed3ad396a6df) | [amazon](https://www.amazon.com/dp/B0CLGDHS16) | This is the default 3.2" screen the controller code is designed for. You can find it on many websites. |
| Battery | 1 | ![image](https://github.com/user-attachments/assets/f6bf7070-4a0c-4cad-a41a-d973b082fe83) | [amazon](https://www.amazon.com/dp/B0DPZVBKMY) | -*Optional*- You need a small battery (600ma or more) with a jst 1.25 connector if you want the controller to work without being plugged in (aka wirelessly). |
| 3D printed case | 1 | - | - | You are going to want to 3d print the files for assembling the case. If you don't own a 3d printer, you will want to order these files printed online or from one of the discord members |


### Manifold Parts

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| (Option #1) Generic Ebay Manifold & Pressure sensors kit | 1 | ![image](https://github.com/user-attachments/assets/9b4588cb-15ce-406a-bedc-2fd31b9598e9) | [amazon](https://www.amazon.com/Display-Suspension-Manifold-Controller-Solenoid/dp/B0F1STPNBH)<!--[ebay](https://www.ebay.com/itm/324238773355)--> | This is a nice kit of manifold + pressure sensors for $150. Of you can purchase them separate like option #2 below. The guage is not used btw! |
| (Option #2.a) Generic Ebay Manifold | 1 | ![image](https://github.com/user-attachments/assets/d73bb078-ae31-4118-87ea-b4fd3c6de5ba) | [ebay](https://www.ebay.com/itm/175635451020)<!--[ebay](https://www.ebay.com/itm/324238773355) price went up $20 on this one --> | All the documentation is based on this manifold and it's wiring. If you use a different manifold you need to follow the wiring diagrams on the PCB carefully |
| (Option #2.b) 200PSI pressure sensor | 5 | ![image](https://github.com/user-attachments/assets/a231e73f-f6e8-48e3-80ee-d0554a466072) | [amazon](https://www.amazon.com/dp/B0BYHW1R2Z) | This is a kit, we will not be using the screen, but it is cheaper than buying individually and more reliable than purchasing the sensors from a random supplier, as they often end up being duds. This kit also includes plenty of wiring, eliminating the need to purchase more wiring for the pressure sensors |
| 22 Gauge 4 or 5 Conductor Electrical Wire | 10ft+ | ![image](https://github.com/user-attachments/assets/d6ff4300-0bce-49bf-948e-1de7f52de4c5) | [amazon](https://www.amazon.com/dp/B0CFJXMDT3?smid=A6X70Q6OCPVWR&ref_=chk_typ_imgToDp&th=1) | You need 4 wires to handle 12v, ground, acc, and compressor |
| Various 1/8 NPT PTC connectors | 0-12 | - | - | Depending on your setup you may configure the ports differently. Easiest route is using all PTC and 1/4" hoses to everything. You can pick most of these up at your local hardware store, too |
| 3D printed manifold base hat and lid | 1 | - | - | You are going to want to 3d print the files for assembling the manifold and pcb together in a nice package. If you don't own a 3d printer, you will want to order these files printed online or from one of the discord members |
<!-- | 12V Stabilizer | 1 | ![20241205_131439](https://github.com/user-attachments/assets/77595e2d-b98f-428d-9c74-689fca6c2d4f) | [amazon](https://www.amazon.com/YABOANG-9-40V-12V3A-Transformer-Instruments/dp/B0CZ8GYJC5?th=1) | Car electronics are not a perfect 12v, would be a good idea to run one of these on the 12v input to stabilize the voltage and help prevent possible issues | -->

### Additional Parts

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| Relay | 1 | ![image](https://github.com/user-attachments/assets/d14dc4ea-0366-496f-81c5-a614b326eb65) | [amazon](https://www.amazon.com/gp/product/B07T35K8S2) | You also may want a relay to control the compressor (Please double check your requirements for the compressor relay and amps!!!!! Requirements vary depending on your compressor setup, which is out of the scope of OAS-Man. OAS-Man only supplies the signal of when to turn on the compressor, not the full power.) |

<!--
### (Outdated) PCB Board v2.0 required parts
| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| FQP30N06L mosfet     |     11    | ![image](https://github.com/user-attachments/assets/a3c97586-92e9-4a2b-ac86-40f317dc5ced) | [ebay](https://www.ebay.com/itm/171395648380) | Linked is a pack of 50 | 
| CN3903 5V buck converter |    1     | ![image](https://github.com/user-attachments/assets/3115a607-604a-47f0-9996-0a4113feee0c) | [amazon](https://www.amazon.com/dp/B0C69DP5RK)  [aliexpress](https://www.aliexpress.us/item/3256802647847616.html) | No pin headers required, surface mount | 
| 2.54mm pin headers |    Assorted      | ![image](https://github.com/user-attachments/assets/4e66a6ba-f1f2-48e0-b0ac-b6fcbb015b4c) | [amazon](https://www.amazon.com/gp/product/B0BX865TRT/ref=ewc_pr_img_3?smid=A1F2OO11SCJ7V3&th=1) [aliexpress](https://pl.aliexpress.com/item/1005006468451122.html) | You need 2 x 15 female headers for mounting the esp32 so there is space under it. Also recommended to mount the ads1115's on these too. | 
| ADS1115 |    2     | ![image](https://github.com/user-attachments/assets/31fdfe86-8bbc-4eab-8224-19a8565d94ff) | [amazon](https://www.amazon.com/gp/product/B0CNV9G4K1) [aliexpress](https://pl.aliexpress.com/item/1005006074781549.html) | You can opt to purchase and install only 1 if you disable the height sensor code in the esp32 configuration file. | 
| 4 Channels IIC I2C Logic Level Converter Bi-Directional 3.3V-5V Shifter Module |   1      | ![image](https://github.com/user-attachments/assets/3a9aa1b9-83d0-4efb-8649-06e1c845b9c5) | [amazon](https://www.amazon.com/dp/B07F7W91LC) [aliexpress](https://pl.aliexpress.com/item/1005005984772131.html) | Pin headers required (as pictured) | 
| PTF-77 Fuse Holder and 5x20 3a fuse |   1      | ![image](https://github.com/user-attachments/assets/6d67dbb4-81c1-4b15-9bf6-52c54b36e47f) | [lcsc holder](https://www.lcsc.com/product-detail/Fuseholders_XFCN-PTF-77_C717030.html)<br> [lcsc fuse](https://www.lcsc.com/product-detail/Disposable-fuses_Reomax-STC1300_C5381727.html) | Purchase from lcsc with the resistors and maybe the diodes too. You can skip the fuse holder and solder in the fuse directly if you prefer. Or skip entirely, solder in a wire, and wire a fuse externally. | 
| 1/4w 12x 10K, 11x 150R, 1x 1K, 1x 2K |   Assorted      | ![image](https://github.com/user-attachments/assets/7492bb33-a26f-439b-a489-2b17d04ce9f2) | [lcsc 10k](https://www.lcsc.com/product-detail/Through-Hole-Resistors_VO-MF1-4W-10K-1-ST52_C2903232.html)<br> [lcsc 150R](https://www.lcsc.com/product-detail/Through-Hole-Resistors_VO-MF1-4W-150-1-ST52_C2903242.html)<br> [lcsc 1K](https://www.lcsc.com/product-detail/Through-Hole-Resistors_VO-MF1-4W-1K-1-ST52_C2903245.html)<br> [lcsc 2K](https://www.lcsc.com/product-detail/Through-Hole-Resistors_VO-MF1-4W-2K-1-ST52_C2903278.html) | 1/4w, 1% resistors (blue), purchase 100 of each from lcsc for cheap | 
| 1N4007 diode |    13     | ![image](https://github.com/user-attachments/assets/daae6c1d-b7ca-4330-b604-794b2f88379b) | [ebay](https://www.ebay.com/itm/235235061574) [aliexpress](https://pl.aliexpress.com/item/1005006003747959.html) [lcsc](https://www.lcsc.com/product-detail/Diodes-General-Purpose_Yangzhou-Yangjie-Elec-Tech-1N4007_C2986225.html) | Linked is a pack of 100 | 
-->

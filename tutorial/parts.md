# Parts list

### PCB Parts

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| FQP30N06L mosfet     |     9    | ![image](https://github.com/user-attachments/assets/a3c97586-92e9-4a2b-ac86-40f317dc5ced) | [ebay](https://www.ebay.com/itm/171395648380) [aliexpress](https://pl.aliexpress.com/item/4000830087947.html) | Linked is a pack of 50 but you may want to find a smaller pack of 10 |
| ESP32 30pin |     1    | ![image](https://github.com/user-attachments/assets/a30c7c9f-4fdf-4c74-ace4-a51401cb46fd) | [amazon](https://www.amazon.com/Teyleten-Robot-ESP-WROOM-32-Development-Microcontroller/dp/B08246MCL5/) [aliexpress](https://www.aliexpress.us/item/3256806150650156.html) | The project currently uses an esp32 wroom 30pin |
| JST-XH 2.54mm  |    Assorted (2-6)     | ![image](https://github.com/user-attachments/assets/f83d2230-f445-4e51-9718-cc29dfb26ecd) | [amazon](https://www.amazon.com/Connector-Housing-Adapter-Terminal-Connectors/dp/B0BZDCGJ32/) | You will need multiple of each size, so it's best to purchase an assorted pack that includes 2,3,4, and 6.<br> 2 x 2 for power and compressor out<br>3 x 10 for valvetable, pressure sensors, height sensors<br>4 x 1 for screen<br> 6 x 1 for other half of valvetable |
| 9x 10K, 9x 150R, 1x 1K, 1x 2K |   Assorted      | - | - | You can try to find a mixed pack that includes all of these on ebay. Usually they will come in sets of 10 resistors for each value, which is enough for this project. Or you can try to purchase them individually but they will have a significant markup |
| 1N4007 diode |    10     | ![image](https://github.com/user-attachments/assets/daae6c1d-b7ca-4330-b604-794b2f88379b) | [ebay](https://www.ebay.com/itm/235235061574) [aliexpress](https://pl.aliexpress.com/item/1005006003747959.html) | Linked is a pack of 100 but you may want to find a smaller pack of 10 |
| 128x64 I2C OLED |     1    | ![image](https://github.com/user-attachments/assets/6c5eb13b-6e0c-4c4f-b4ad-c075ef5d964e) | [amazon](https://www.amazon.com/gp/product/B09JWLDK9F/) [aliexpress](https://www.aliexpress.com/item/1005005241315177.html) | Optional if you decide to disable it in the esp32 config. Depending on which one you purchase, you may need to change the i2c address in the esp32 configuration file. Some have solderable address configuration on the back of the screen/board |
| CN3903 5V buck converter |    1     | ![image](https://github.com/user-attachments/assets/3115a607-604a-47f0-9996-0a4113feee0c) | [amazon](https://www.amazon.com/dp/B0C69DP5RK)  [aliexpress](https://www.aliexpress.us/item/3256802647847616.html) | No pin headers required, surface mount |
| 2.54mm pin headers |    Assorted      | ![image](https://github.com/user-attachments/assets/f4404869-27d4-40ed-8003-937a9998015d) | [amazon](https://www.amazon.com/gp/product/B07MN2MF1D) [aliexpress](https://pl.aliexpress.com/item/1005006468451122.html) | You only need 2 x 15 female headers for mounting the esp32. Technically this is optional, but highly recommended in case your esp32 ever needs replaced so you don't have to rebuild the whole board. It also gives plenty of space to mount the components underneath the esp32 |
| ADS1115 |    2     | ![image](https://github.com/user-attachments/assets/31fdfe86-8bbc-4eab-8224-19a8565d94ff) | [amazon](https://www.amazon.com/gp/product/B0CNV9G4K1) [aliexpress](https://pl.aliexpress.com/item/1005006074781549.html) | You can opt to purchase and install only 1 if you disable the height sensor code in the esp32 configuration file. |
| 4 Channels IIC I2C Logic Level Converter Bi-Directional 3.3V-5V Shifter Module |   1      | ![image](https://github.com/user-attachments/assets/3a9aa1b9-83d0-4efb-8649-06e1c845b9c5) | [amazon](https://www.amazon.com/dp/B07F7W91LC) [aliexpress](https://pl.aliexpress.com/item/1005005984772131.html) | Pin headers required (as pictured) |


### Manifold Parts

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| Generic Ebay Manifold | 1 | ![image](https://github.com/user-attachments/assets/d73bb078-ae31-4118-87ea-b4fd3c6de5ba) | [ebay](https://www.ebay.com/itm/324238773355) | All the documentation is based on this manifold and it's wiring. If you use a different manifold you need to follow the wiring diagrams on the PCB carefully |
| 200PSI pressure sensor | 5 | ![image](https://github.com/user-attachments/assets/a231e73f-f6e8-48e3-80ee-d0554a466072) | [amazon](https://www.amazon.com/dp/B0BYHW1R2Z) | This is a kit, we will not be using the screen, but it is cheaper than buying individually and more reliable than purchasing the sensors from a random supplier, as they often end up being duds. This kit also includes plenty of wiring, eliminating the need to purchase more wiring for the pressure sensors |
| Various 1/8 NPT PTC connectors | 0-12 | - | - | Depending on your setup you may configure the ports differently. Easiest route is using all PTC and 1/4" hoses to everything |
| 3D printed manifold base hat and lid | 1 | - | - | You are going to want to 3d print the files for assembling the manifold and pcb together in a nice package. If you don't own a 3d printer, you will want to order these files printed online or from one of the discord members |
| Main PCB | 1 | - | - | Order the pcb from jlcpcb (or whatever you prefer) in the minimum quantity of 5 (about $30) or purchase one from a discord member. More info on the other tutorial pages. |

### Additional Parts

| Part Name  | qty | image | Link | info |
| --------- | ------- | ------- | ------- | ------- |
| Relay | 2 | ![image](https://github.com/user-attachments/assets/d14dc4ea-0366-496f-81c5-a614b326eb65) | [amazon](https://www.amazon.com/gp/product/B07T35K8S2) | You will likely want to have a main relay to run the 12v wiring from, just like any other aftermarket accessory component on a vehicle. You also may want a relay to control the compressor (Please double check your requirements for the compressor relay and amps!!!!! Requirements vary depending on your compressor setup, which is out of the scope of OAS-Man. OAS-Man only supplies the signal of when to turn on the compressor, not the full power.) |

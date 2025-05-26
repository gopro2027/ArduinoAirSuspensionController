image_creator.py generates the binary code from images to use in OASMan_ESP32/src/bitmaps.h for the mini OLED screen<br>
font_to_image_creator.py is a quick utility to render ttf files as PNG images to then use in image_creator.py<br>
Supplied are some corvette themed fonts named 'Grand Sport'. If you are going to use commercially you need to follow the instructions here https://www.iconian.com/commercial.html (essentially pay the guy $20)<br>
<br>
Description:<br>
Creates image code for use inside the 128x64 screen in the oasman esp32 code<br>
Please run the python file with python 3 and input the parameters as prompted<br>
Result image will be put into output folder (for reference only). ESP32 code will be displayed in the console.<br>
<br>
Alternatively, here appears to be a tutorial on how to do the same thing but with a gui tool: https://www.instructables.com/Display-Your-Photo-on-OLED-Display/ 

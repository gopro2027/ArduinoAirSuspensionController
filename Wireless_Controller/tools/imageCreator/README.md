Get a good side shot photo of your car ready. Preferrably aired up so your whole tire is showing.

pip install -r requirements.txt

run the python script

Select which controller you are using at the top

Click the empty area to import your image

Resize window as large as you can to make process easier

Select the wheels first as instructed. Click the center of the wheel then 2 outside points on the edge of the tire. You will do the left first. Click done after you're satisfied with the positioning.

Repeat for the right tire

Now carefully outline your car by clicking 1 dot at a time. Outline everything except the wheels. Click done once you've gone around the whole car.

Now you have to position and scale your car to fit within the image. Ideally you will get the wheel wells to match up as close as possible. Note: Making the car the full width of the area will make it the full width of the device, so you may not want to make it fit the entire area edge to edge.

Click done to export the image.

Next you will be on the wheels page. The wheels are automatically scaled to the same scale as your car was on the page before. A slightly outline of your car will be at the top of the screen for wheel well placement. Ideally you will only do X adjustments, but you can do Y adjustments too if necessary.

Click done to export the image. Do not close the program yet, as you may want to make adjustments to the wheel positioning after testing!

Go to https://lvgl.io/tools/imageconverter and set it to RGB565A8 and copy both of the exported png's (found in Wireless_Controller/src/) there and click convert and then copy the resulting files to the same src folder

Go into platformio.ini and uncomment the line that says '-D CUSTOM_CAR_IMAGE' (remove the semicolon at the beginning of the line)

Compile and upload to your device to test. Go back and adjust wheel positioning as necessary.

Wheel well instructions: The way our preset car works, you will have black boxes rendered in place of the wheel wells do it doesn't look as much like your car is floating above the wheels when you are on a high preset. You may want to go into ui_scrPresets.cpp and adjust the values in fender1Offset and fender2Offset to fill out the wheel wells better. Sorry it is not easier. Just keep retrying until you get it looking good. The format is x,y,width,height
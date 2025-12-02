# uncomment these for a new device and set the name etc to iterate through them and resize the 240x320 images to the new size

#device = "ws3p5b"
#newWidth = 320
#newHeight = 480



startX = 240
startY = 320
from PIL import Image
import os

ratioX = newWidth / startX
ratioY = newHeight / startY

def resizeImage(input_image_path):
    # Open the image
    img = Image.open(input_image_path)

    # Define new dimensions (width, height)
    new_size = (int(img.width * ratioX), int(img.height * ratioY))

    # Resize the image
    resized_img = img.resize(new_size)

    print(f"Resized image to: {resized_img.size}")

    # Save the resized image
    resized_img.save(input_image_path)


directory_path = device+"/images/"  # Replace with your directory path

for root, dirs, files in os.walk(directory_path):
    for filename in files:
        full_path = os.path.join(root, filename)
        print(f"File: {full_path}")
        # Perform operations on the file here
        resizeImage(full_path)

# This script is just to create an image from a font that we can then use in the imagte_creator.py script

from PIL import Image, ImageDraw, ImageFont

text = "OAS-MAN"
width, height = 2000, 2000 # Make image really big because it is cropped at the end. Can increase if needed
font_path = 'fonts/grand_sport/grandsportital.ttf'  # Replace with your font file path
font_size = 64

# Create a blank image
image = Image.new('RGBA', (width, height), (255, 255, 255, 255))  # White background

# Create a drawing object
draw = ImageDraw.Draw(image)

try:
    font = ImageFont.truetype(font_path, font_size)
except OSError:
    print(f"Error: Cannot find or open font file at {font_path}. Using default font.")
    font = ImageFont.load_default()

# # Calculate position to center text
# x = (width - text_width) / 2
# y = (height - text_height) / 2

# Get the bounding box of the text
bbox = draw.textbbox((0, 0), text, font=font)

# # Calculate width and height from the bounding box
# text_width = bbox[2] - bbox[0]
# text_height = bbox[3] - bbox[1]

# Draw the text
draw.text((0, 0), text, font=font, fill=(0, 0, 0, 255))  # Black text

image = image.crop(bbox)

# Save the image
image.save('font_to_image_creator_output.png')
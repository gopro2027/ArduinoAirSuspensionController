# create images for the boot sequence and car logo

from PIL import Image
print("Please answer the questions! Tip: You may want to crop your image to the desired final size prior to doing this to avoid scaling issues!")
print("Please type width in pixels for output in multipes of 8 (8, 16, 24... to 128):")
requested_width = int(input())
requested_width = requested_width - requested_width % 8
print("Please type height in pixels for output (1 to 64):")
requested_height = int(input())
print("Would you like the image to be dithered or hard black and white? Type 'y' for dithering and 'n' for hard black and white:")
isHardBW = input() == 'n'
bwthreshold = 200
if isHardBW:
    print("Please enter the threshold for black/white decision (1 to 255) (ex: 200):")
    bwthreshold = int(input())
print("Please type file name or path of image (ex: test_image.png):")
image_path = input()

image = Image.open(image_path)
image = image.resize((requested_width, requested_height))

if isHardBW:
    fn = lambda x : 255 if x > bwthreshold else 0
    image = image.convert('L').point(fn, mode='1')
else:
    image = image.convert('1')

image.save('output/'+image_path)


# output code to console
strbldr = []
strbldr.append("static const unsigned char PROGMEM generated_image[] =\n")
strbldr.append("{\n")
for y in range(0, requested_height):
    for x in range(0, requested_width, 8):
        arr = []
        for i in range(8):
            arr.append('0' if image.getpixel((x+i, y)) == 0xFF else '1')
        binnum = '0b'+''.join(arr)
        strbldr.append(binnum+", ")
    strbldr.append("\n")
strbldr.append("};")
    
result = "".join(strbldr)
print(result)
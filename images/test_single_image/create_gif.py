from PIL import Image, ImageDraw
import numpy as np
import sys
np.random.seed(0)

if (len(sys.argv) < 3):
    print >> sys.stderr, "ERROR. Usage : XXX.sh height_min step"
    exit(1)
height_min = int(sys.argv[1])
step = int(sys.argv[2])
width = 200
print >> sys.stderr, "Creating 10 gif with height = {} to {}".format(height_min, height_min+10*step)
color_1 = (255, 255, 255)

for height in range(height_min, height_min + 10 * step + 1, step):
    im = Image.new('RGB', (width, height), color_1)
    for i in range(width):
        for j in range(height):
            im.putpixel((i,j), (np.random.randint(256), np.random.randint(256), np.random.randint(256)))
    images = [im]
    images[0].save('./{}.gif'.format(height), save_all=True, append_images=images[1:], optimize=False, duration=1, loop=0)

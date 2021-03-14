from PIL import Image, ImageDraw
import numpy as np
import sys
np.random.seed(0)

color_1 = (255, 255, 255)
width_max = 5001
height_max = 5001
im = Image.new('RGB', (width_max, height_max), color_1)
print >> sys.stderr ,"creating {}x{}.gif".format(width_max, height_max)
rand = np.random.randint(low=256, size=(width_max, height_max, 3))
for i in range(width_max):
    for j in range(height_max):
        im.putpixel((i,j), (rand[i][j][0], rand[i][j][1], rand[i][j][2]))

for width in range(1000, width_max+1, 500):
    for height in range(1000, height_max+1, 500):
        print >> sys.stderr ,"creating {}x{}.gif".format(width, height)
        images = [im.crop((0, 0, width, height))]
        images[0].save('./{}x{}.gif'.format(width, height), save_all=True, append_images=images[1:], optimize=False, duration=1, loop=0)

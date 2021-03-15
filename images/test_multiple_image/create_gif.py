from PIL import Image, ImageDraw
import numpy as np
import sys
np.random.seed(0)

height = 100
width = 100
num = 50
nb_images=np.arange(1,num+1,5)

#if (len(sys.argv) < 3):
#    print >> sys.stderr, "ERROR. Usage : XXX.sh height_min step"
#    exit(1)
#height_min = int(sys.argv[1])
#step = int(sys.argv[2])
#width = 200
color_1 = (255, 255, 255)

rand = np.random.randint(256, size=(num, width, height, 3))
images = []

for k in range(len(nb_images)):
    for l in range(nb_images[k]):
        im = Image.new('RGB', (width, height), color_1)
        for i in range(width):
            for j in range(height):
                im.putpixel((i,j), (rand[l][i][j][0], rand[l][i][j][1], rand[l][i][j][2]))
        images.append(im)
        images[0].save('./{}x{}x{}.gif'.format(nb_images[k], width, height), save_all=True, append_images=images[1:], optimize=False, duration=100, loop=0)

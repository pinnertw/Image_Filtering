from PIL import Image, ImageDraw
import numpy as np
np.random.seed(0)

for height in range(200, 5000, 500):
    images = []
    width = 200
    center = width // 2
    color_1 = (0, 0, 0)
    color_2 = (255, 255, 255)

    im = Image.new('RGB', (width, height), color_1)
    for i in range(width):
        for j in range(height):
            im.putpixel((i,j), (np.random.randint(256), np.random.randint(256), np.random.randint(256)))
    images = [im]
    images[0].save('./{}.gif'.format(height), save_all=True, append_images=images[1:], optimize=False, duration=1, loop=0)

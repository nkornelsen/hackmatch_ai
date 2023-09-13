import numpy as np
from PIL import Image as im

arr = np.loadtxt("image_data.txt", dtype=np.uint8)
arr = np.reshape(arr, (240, 145, 3))
arr = arr[...,::-1]

data = im.fromarray(arr)

data.save("image.png")

print(arr)
import socket
import struct
import numpy as np
from PIL import Image

sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)

sock.connect((2, 0x5f0))

data = sock.recv(4)
num = struct.unpack("!i", data)
img_array = np.empty(240*145*3, dtype=np.uint8)
# img_data = sock.recv(16385, socket.MSG_WAITALL)
bytes_left = 240*145*3
# img_dst = np.ravel(img_array)
while bytes_left > 0:
    arr_offset = 240*145*3 - bytes_left
    sock.recv_into(img_array[arr_offset:], min(bytes_left, 16384), socket.MSG_WAITALL)
    bytes_left -= min(bytes_left, 16384)

# img_array = np.frombuffer(img_data, dtype=np.uint8)
img_array = np.reshape(img_array, (240, 145, 3))
img_array = img_array[...,::-1]
im = Image.fromarray(img_array)
im.save("image.png")
    
sock.close()
import tkinter
import socket
import struct
import numpy as np
from PIL import Image, ImageTk

sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
sock.connect((2, 0x5f0))

window = tkinter.Tk()
lmain = tkinter.Label(window)
# lmain.configure(image=im_tk)
# lmain.place(x=0, y=0)

im_tk = None
# def set_image():
#     # global im_tk
#     im_tk = ImageTk.PhotoImage(file="image.png")
#     lmain.configure(image=im_tk)
#     lmain.place(x=0, y=0)
# set_image()

current_score = 0
def video_stream():
    global im_tk
    global current_score
    # print("Recv...")
    score_data = sock.recv(4, socket.MSG_WAITALL)
    score = struct.unpack("!i", score_data)[0]
    if score == -1:
        score_data = sock.recv(4, socket.MSG_WAITALL)
        score = struct.unpack("!i", score_data)[0]
    if score != current_score:
        current_score = score
        print(f"score: {current_score}")
    # bytes_left = 260*145*3
    img_array = np.empty(260*145*3, dtype=np.uint8)
    # img_data = sock.recv(16385, socket.MSG_WAITALL)
    bytes_left = 260*145*3
    # img_dst = np.ravel(img_array)
    while bytes_left > 0:
        arr_offset = 260*145*3 - bytes_left
        sock.recv_into(img_array[arr_offset:], min(bytes_left, 16384), socket.MSG_WAITALL)
        bytes_left -= min(bytes_left, 16384)
    # print("Got frame")
    # img_array = np.frombuffer(img_data, dtype=np.uint8)
    img_array = np.reshape(img_array, (260, 145, 3))
    img_array = img_array[...,::-1]

    # for y in range(260):
    #     for x in range(145):
    #         for c in range(3):
    #             print(img_array[y, x, c], end=' ')
    # print("")

    im = Image.fromarray(img_array)
    im_tk = ImageTk.PhotoImage(image=im)
    lmain.configure(image=im_tk)
    lmain.place(x=0, y=0)
    lmain.after(1, video_stream)

video_stream()

window.mainloop()
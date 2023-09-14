import tkinter
from PIL import Image, ImageTk

window = tkinter.Tk()

im = Image.open("image.png")
im_tk = ImageTk.PhotoImage(image=im)
label = tkinter.Label(window)
label.configure(image=im_tk)
# label.pack()
label.place(x=0, y=0)
window.mainloop()
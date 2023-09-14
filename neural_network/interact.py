import socket
import struct

sock = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)

sock.connect((2, 0x5f0))

while True:
    data = sock.recv(4)
    num = struct.unpack("!i", data)
    print(num)
sock.close()
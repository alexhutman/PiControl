#######################################
# Single character input - from https://stackoverflow.com/a/20865751

class _Getch:
    """Gets a single character from standard input.  Does not echo to the screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): 
        char = self.impl()
        if char == '\x03':
            raise KeyboardInterrupt
        elif char == '\x04':
            raise EOFError
        return char

class _GetchUnix:
    def __init__(self):
        import tty
        import sys

    def __call__(self):
        import sys
        import tty
        import termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch


class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()


getch = _Getch()
#######################################

import socket
import sys
import time

from enum import IntEnum, auto

class PiControlCmd(IntEnum):
        PI_CTRL_HEARTBEAT  = 0      # Client: Send heartbeat so server can disconnect if connection is lost
        PI_CTRL_MOUSE_MV   = auto() # Client: Send x,y of relative position to move mouse to
        PI_CTRL_MOUSE_DOWN = auto() # Client: Say to press mouse down (no data required)
        PI_CTRL_MOUSE_UP   = auto() # Client: Say to press mouse up (no data required)
        PI_CTRL_MOUSE_CLK  = auto() # Client: Say to click (mouseup, then mousedown) mouse (no data required)
        PI_CTRL_KEY_PRESS  = auto() # Client: Send UTF-8 value of key to be pressed (details TBD)

ADDR = "localhost"
PORT = 14741

def create_sock():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv_addr = (ADDR, PORT)
    sock.connect(serv_addr)

    return sock

"""
def receive_chunk(sock, size):
    recv_chunk = sock.recv(size)
    print("SERVER: 0x{}".format(recv_chunk.hex()))
    return recv_chunk
"""

def test_one_msg(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    test_msg = "и".encode("utf8")

    print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(test_msg), test_msg.hex()))
    sock.send(bytes([cmd, len(test_msg)]) + test_msg)
    #recv_cmd = PiControlCmd(int.from_bytes(msg[0], "big")).name

def test_multiple_msgs(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    msg = "Hello, world!"
    for char in msg:
        enc_char = char.encode("utf8")
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(enc_char), enc_char.hex()))
        sock.send(bytes([cmd, len(enc_char)]) + enc_char)
        
def test_continuous_msgs(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    while True:
        char = getch()
        enc_char = char.encode("utf8")
        #print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(enc_char), char))
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(enc_char), char))
        sock.send(bytes([cmd, len(enc_char)]) + enc_char)
        

if __name__ == "__main__":
    sock = create_sock()
    try:
        #test_one_msg(sock)
        #test_multiple_msgs(sock)
        test_continuous_msgs(sock)
    finally:
        print("Closing socket...")
        sock.close()

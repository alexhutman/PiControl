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
        if char == b'\x03':
            raise KeyboardInterrupt
        elif char == b'\x04':
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
        return ch.encode("utf8")


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

PORT = 14741

def parse_args():
    #TODO: maybe use argparse module
    num_args = 1
    if len(sys.argv) != num_args + 1: # +1 since first arg is the filename
        print("Usage: python3 test_server.py <ADDRESS>")
        sys.exit(1)

    return sys.argv[1:]

def create_sock(addr):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv_addr = (addr, PORT)
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
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(char), char.hex()))
        sock.send(bytes([cmd, len(char)]) + char)
        time.sleep(0.3)
        
def test_continuous_msgs(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    while True:
        char = getch()
        #print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(char), char))
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(char), char))
        sock.send(bytes([cmd, len(char)]) + char)

def test_russian(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    for char in "Здравствуйте":
        #print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(char), char))
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(char), char))
        sock.send(bytes([cmd, len(char)]) + char)
        time.sleep(0.5)
        
def test_mouse_move(sock):
    cmd = PiControlCmd.PI_CTRL_MOUSE_MV
    rel_x, rel_y = 1, 1

    rel_mv = rel_x.to_bytes(1, 'big') + rel_y.to_bytes(1, 'big')
    while True:
        print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(cmd.name, len(rel_mv), rel_mv.hex()))
        sock.send(bytes([cmd, len(rel_mv)]) + rel_mv)

        time.sleep(0.1)

if __name__ == "__main__":
    args = parse_args()

    addr = args[0]
    sock = create_sock(addr)
    try:
        #test_one_msg(sock)
        #test_multiple_msgs(sock)
        test_continuous_msgs(sock)
        #test_mouse_move(sock)
        #test_russian(sock)
    finally:
        print("Closing socket...")
        sock.close()

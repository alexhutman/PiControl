#!/usr/bin/env python3

import argparse
import binascii
import socket
import sys
import time

from enum import IntEnum, auto

DEFAULT_PORT = 14741

#######################################
# Single character input - from https://stackoverflow.com/a/20865751
try:
    import msvcrt
except ImportError:
    import tty
    import termios


class _Getch:
    """Gets a single character from standard input.  Does not echo to the screen."""
    def __init__(self):
        self.impl = _GetchWindows() if "msvcrt" in sys.modules else _GetchUnix()

    def __call__(self): 
        char = self.impl()
        if char == b'\x03':
            raise KeyboardInterrupt
        elif char == b'\x04':
            raise EOFError
        return char

class _GetchUnix:
    def __call__(self):
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch.encode("utf8")


class _GetchWindows:
    def __call__(self):
        return msvcrt.getch()


getch = _Getch()
#######################################

class PiControlCmd(IntEnum):
        PI_CTRL_HEARTBEAT  = 0      # Client: Send heartbeat so server can disconnect if connection is lost
        PI_CTRL_MOUSE_MV   = auto() # Client: Send x,y of relative position to move mouse to
        PI_CTRL_MOUSE_DOWN = auto() # Client: Say to press mouse down (no data required)
        PI_CTRL_MOUSE_UP   = auto() # Client: Say to press mouse up (no data required)
        PI_CTRL_MOUSE_CLK  = auto() # Client: Say to click (mouseup, then mousedown) mouse (no data required)
        PI_CTRL_KEY_PRESS  = auto() # Client: Send UTF-8 value of key to be pressed (details TBD)
        PI_CTRL_KEYSYM     = auto() # Client: Send keysym (combination)

def parse_args():
    parser = argparse.ArgumentParser(
            description="Tests the PiControl server",
            formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )

    parser.add_argument("address",
                        type=str,
                        help="IP address of the running PiControl server")

    parser.add_argument("--port",
                        type=int,
                        default=DEFAULT_PORT,
                        help="Port that the running PiControl server is listening on")

    tests = {
        "one":  test_one_msg,
        "mul":  test_multiple_msgs,
        "cont": test_continuous_msgs,
        "ksym": test_keysym,
        "maus": test_mouse_move,
        "rus":  test_russian,
    }
    parser.add_argument("--tests",
                        action="extend",
                        choices=sorted(tests.keys()),
                        type=str,
                        nargs='+',
                        help="Which tests to run. Duplicates are ignored. By default, all tests run.")

    args = parser.parse_args()

    if args.tests is None: # not specified, run all tests
        args.tests = tests.values()
    else:
        args.tests = map(lambda tst: tests[tst], set(args.tests))
    args.tests = list(args.tests)
    return args

def create_sock(addr, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv_addr = (addr, port)
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
    test_msg = "и".encode("utf-8")

    print_pimsg(cmd, test_msg)
    sock.send(serialize_cmd(cmd, test_msg))

def test_multiple_msgs(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    msg = "Hello, world!"
    for char in map(lambda c: c.encode("utf-8"), msg):
        print_pimsg(cmd, char)
        sock.send(serialize_cmd(cmd, char))
        time.sleep(0.3)
        
def test_continuous_msgs(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    while True:
        try:
            char = getch()
        except KeyboardInterrupt:
            break

        print_pimsg(cmd, char)
        sock.send(serialize_cmd(cmd, char))

def test_russian(sock):
    cmd = PiControlCmd.PI_CTRL_KEY_PRESS
    for encoded_char in map(lambda c: c.encode('utf-8'), "Здравствуйте"):
        print_pimsg(cmd, encoded_char)
        sock.send(serialize_cmd(cmd, encoded_char))
        time.sleep(0.5)
        
def test_mouse_move(sock):
    cmd = PiControlCmd.PI_CTRL_MOUSE_MV
    rel_x, rel_y = 1, 1
    rel_mv = rel_x.to_bytes(1, 'big') + rel_y.to_bytes(1, 'big')

    # Just move 25 units (arbitrary number) up and to the right
    for _ in range(25):
        print_pimsg(cmd, rel_mv)
        sock.send(serialize_cmd(cmd, rel_mv))

        time.sleep(0.1)

def test_keysym(sock):
    cmd = PiControlCmd.PI_CTRL_KEYSYM
    msg = "Ctrl+a".encode("utf-8")
    print_pimsg(cmd, msg)
    sock.send(serialize_cmd(cmd, msg))

def print_pimsg(cmd, payload):
    print(f"Sending {cmd.name}, |{payload}| ({binascii.hexlify(payload)}) [payload len: {len(payload)}]")

def serialize_cmd(cmd, msg):
    return bytes([cmd, len(msg)]) + msg

def main():
    args = parse_args()

    sock = create_sock(args.address, args.port)
    try:
        for test in args.tests:
            print(f"Running {test.__name__}...")
            test(sock)
    except BrokenPipeError:
        print("Connection closed by server.")
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        print("Closing socket...")
        sock.shutdown(socket.SHUT_RDWR)
        sock.close()

if __name__ == "__main__":
    main()

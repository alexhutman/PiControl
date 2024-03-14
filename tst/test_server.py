#!/usr/bin/env python3

import argparse
import binascii
import errno
import socket
import sys
import time

from enum import IntEnum, auto
from functools import cached_property

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

class PiControlMessage:
    __slots__ = '__dict__', '_cmd', '_payload', 'payload_len', '_serialized'

    def __init__(self, cmd, payload):
        self.cmd = cmd
        self.payload = payload # should be a bytes list

    def reset_serialization(self):
        self.__dict__.pop("serialized", None)

    @property
    def cmd(self):
        return self._cmd

    @cmd.setter
    def cmd(self, new_cmd):
        self._cmd = new_cmd

        # TODO: Make cleaner? The idea is that reserializing the whole obj is very inefficient if we're only changing the cmd
        if "serialized" in self.__dict__:
            self.serialized[0] = new_cmd

    @property
    def payload(self):
        return self._payload

    @payload.setter
    def payload(self, new_payload):
        self._payload = new_payload
        self.payload_len = len(new_payload)
        self.reset_serialization()

    @cached_property
    def serialized(self):
        self._serialized = bytearray([self.cmd, self.payload_len])
        self._serialized.extend(self.payload)
        return self._serialized

    def __repr__(self):
        return f"{self.__class__.__name__}(cmd={self.cmd.name}, payload_len={self.payload_len}, payload={self.payload}, raw_payload={binascii.hexlify(self.payload)})"

    __str__ = __repr__

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
    msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, "и".encode("utf-8"))
    print(msg)
    sock.sendall(msg.serialized)

def test_multiple_msgs(sock):
    text = "Hello, world!"
    for char in map(lambda c: c.encode("utf-8"), text):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, char)
        print(msg)
        sock.sendall(msg.serialized)
        time.sleep(0.3)
        
def test_continuous_msgs(sock):
    while True:
        try:
            char = getch()
        except KeyboardInterrupt:
            break

        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, char)
        print(msg) # TODO: make normal function (not method) that prints "Sending {msg}"
        sock.sendall(msg.serialized)

def test_russian(sock):
    for encoded_char in map(lambda c: c.encode('utf-8'), "Здравствуйте"):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, encoded_char)
        print(msg)
        sock.sendall(msg.serialized)
        time.sleep(0.5)
        
def test_mouse_move(sock):
    rel_x, rel_y = 1, 1
    rel_mv = rel_x.to_bytes(1, 'big') + rel_y.to_bytes(1, 'big')

    # Just move 25 units (arbitrary number) up and to the right
    for _ in range(25):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_MOUSE_MV, rel_mv)
        print(msg)
        sock.sendall(msg.serialized)

        time.sleep(0.1)

def test_keysym(sock):
    msg = PiControlMessage(PiControlCmd.PI_CTRL_KEYSYM, "Ctrl+a".encode("utf-8"))
    print(msg)
    sock.sendall(msg.serialized)

def shut_down_sock(sock):
    try:
        sock.shutdown(socket.SHUT_RDWR)
    except OSError as err:
        if err.errno != errno.ENOTCONN:
            raise

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
        shut_down_sock(sock)
        sock.close()

if __name__ == "__main__":
    main()

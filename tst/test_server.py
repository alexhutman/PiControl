#!/usr/bin/env python3

import argparse
import asyncio
import binascii
import errno
import sys
import time
import websockets

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
        elif char == b'\x0D':
            # CR -> LF
            char = b'\x0A'
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
        PI_CTRL_HEARTBEAT   = 0      # Client: Send heartbeat so server can disconnect if connection is lost
        PI_CTRL_MOUSE_MV    = auto() # Client: Send x,y of relative position to move mouse to
        PI_CTRL_MOUSE_CLICK = auto() # Client: Say to click (mouseup or mousedown) mouse (left or right button)
                                         # This is 1 byte, where 00000021 the 2 == PiCtrlMouseBtn and the 1 == PiCtrlMouseClick
        PI_CTRL_KEY_PRESS   = auto() # Client: Send UTF-8 value of key to be pressed (details TBD)
        PI_CTRL_KEYSYM      = auto() # Client: Send keysym (combination)

class PiControlMouseBtn(IntEnum):
        PI_CTRL_MOUSE_LEFT  = 0
        PI_CTRL_MOUSE_RIGHT = 1

class PiControlMouseClick(IntEnum):
        PI_CTRL_MOUSE_UP    = 0
        PI_CTRL_MOUSE_DOWN  = 1


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
        "maus-man": test_mouse_move_manual,
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

def get_ws_addr(addr, port):
    return ''.join(["ws://", addr, ":", str(port)])

async def create_websock(addr):
    try:
        sock = await websockets.connect(addr)
    except OSError as os_err:
        if os_err.errno == errno.EHOSTUNREACH:
            raise OSError(f"Could not connect to {addr}! "
                          "Is PiControl running on your Pi? "
                          "Are you connected to the same network?") from os_err
        raise
    return sock

async def close_websock(sock):
    await sock.close()
    return

"""
def receive_chunk(sock, size):
    recv_chunk = sock.recv(size)
    print("SERVER: 0x{}".format(recv_chunk.hex()))
    return recv_chunk
"""

async def test_one_msg(sock):
    msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, "и".encode("utf-8"))
    print(msg)
    await sock.send(msg.serialized)

async def test_multiple_msgs(sock):
    text = "Hello, world!"
    for char in map(lambda c: c.encode("utf-8"), text):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, char)
        print(msg)
        await sock.send(msg.serialized)
        time.sleep(0.3)
        
async def test_continuous_msgs(sock):
    while True:
        try:
            char = getch()
        except KeyboardInterrupt:
            break

        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, char)
        print(msg) # TODO: make normal function (not method) that prints "Sending {msg}"
        await sock.send(msg.serialized)

async def test_russian(sock):
    for encoded_char in map(lambda c: c.encode('utf-8'), "Здравствуйте"):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_KEY_PRESS, encoded_char)
        print(msg)
        await sock.send(msg.serialized)
        time.sleep(0.5)
        
async def test_mouse_move(sock):
    rel_x, rel_y = 1, 1
    rel_mv = rel_x.to_bytes(1, 'big') + rel_y.to_bytes(1, 'big')

    # Just move 25 units (arbitrary number) up and to the right
    for _ in range(25):
        msg = PiControlMessage(PiControlCmd.PI_CTRL_MOUSE_MV, rel_mv)
        print(msg)
        await sock.send(msg.serialized)

        time.sleep(0.002)

async def test_mouse_move_manual(sock):
    n = 10
    valid_arrow_chars = {
            b"\x1b[A": (0, -n), # UP
            b"\x1b[B": (0,  n), # DOWN
            b"\x1b[D": (-n, 0), # LEFT
            b"\x1b[C": (n,  0), # RIGHT
        }
    while True:
        try:
            char = getch()
            if char == b"\x1b": # esc -- an arrow key (hopefully)
                sequence = b''.join([char] + [getch() for _ in range(2)]) # It's 2 more bytes
                if sequence not in valid_arrow_chars:
                    continue
                rel_x, rel_y = valid_arrow_chars[sequence]
                rel_mv = rel_x.to_bytes(1, 'big', signed=True) + rel_y.to_bytes(1, 'big', signed=True)

                msg = PiControlMessage(PiControlCmd.PI_CTRL_MOUSE_MV, rel_mv)
                print(msg)
                await sock.send(msg.serialized)
            elif char == b"\x20": # space
                # Mouse down
                payload =  PiControlMouseBtn.PI_CTRL_MOUSE_LEFT << 1
                payload |= PiControlMouseClick.PI_CTRL_MOUSE_DOWN << 0
                msg = PiControlMessage(PiControlCmd.PI_CTRL_MOUSE_CLICK, payload.to_bytes(1, 'big'))
                print(msg)
                await sock.send(msg.serialized)
                time.sleep(0.002)

                # Mouse up
                payload &= PiControlMouseClick.PI_CTRL_MOUSE_UP << 0
                msg.payload = payload.to_bytes(1, 'big')
                print(msg)
                await sock.send(msg.serialized)
        except KeyboardInterrupt:
            break

async def test_keysym(sock):
    msg = PiControlMessage(PiControlCmd.PI_CTRL_KEYSYM, "Ctrl+a".encode("utf-8"))
    print(msg)
    await sock.send(msg.serialized)

async def main():
    args = parse_args()

    ws_addr = get_ws_addr(args.address, args.port)
    sock = await asyncio.create_task(create_websock(ws_addr))

    try:
        for test in args.tests:
            print(f"Running {test.__name__}...")
            await test(sock)
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        print("Closing socket...")
        await close_websock(sock)

if __name__ == "__main__":
    asyncio.run(main())

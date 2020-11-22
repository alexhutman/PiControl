import socket
import sys

from enum import IntEnum, auto

class PiControlCmd(IntEnum):
	PI_CTRL_SYN_ACK    = 0      # Server: You are now connected - continue
	PI_CTRL_BUSY       = auto() # Server: Someone is already connected - you must disconnect

	PI_CTRL_SYN        = auto() # Client: Request to connect
	PI_CTRL_HEARTBEAT  = auto() # Client: Send heartbeat so server can disconnect if connection is lost
	PI_CTRL_DISCONNECT = auto() # Client: Let server know you are disconnecting

	PI_CTRL_SET_NAME   = auto() # Client: Send current name so server can say who is currently connected on a PI_CTRL_BUSY
	PI_CTRL_MOUSE_MV   = auto() # Client: Send x,y of relative position to move mouse to
	PI_CTRL_MOUSE_DOWN = auto() # Client: Say to press mouse down (no data required)
	PI_CTRL_MOUSE_UP   = auto() # Client: Say to press mouse up (no data required)
	PI_CTRL_MOUSE_CLK  = auto() # Client: Say to click (mouseup, then mousedown) mouse (no data required)
	PI_CTRL_KEY_PRESS  = auto() # Client: Send UTF-8 value of key to be pressed (details TBD)

ADDR = "localhost"
PORT = 14741

CMD = PiControlCmd.PI_CTRL_SET_NAME
TEST_MSG = "Alex's iPhone"

def create_sock():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv_addr = (ADDR, PORT)
    sock.connect(serv_addr)

    return sock

def receive_chunk(sock, size):
    recv_chunk = sock.recv(size)
    print("SERVER: 0x{}".format(recv_chunk.hex()))
    return recv_chunk

sock = create_sock()

try:
    msg = receive_chunk(sock, 256)
    recv_cmd = PiControlCmd(msg[0]).name
    print(recv_cmd)

    print("SENDING [CMD, PAYLOAD_LEN, PAYLOAD] = {}, {}, |{}|".format(CMD.name, len(TEST_MSG), TEST_MSG))
    sock.send(bytes([CMD, len(TEST_MSG)]) + bytes(TEST_MSG, "utf-8"))
    #recv_cmd = PiControlCmd(int.from_bytes(msg[0], "big")).name

finally:
    sock.close()


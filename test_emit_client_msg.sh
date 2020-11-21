#!/usr/bin/env bash

ADDR=localhost
PORT=14741

msg="Hello, this is a test! Carry on."
msg_byte_len=${#msg}
msg_byte_len_hex=$( printf "%x" $msg_byte_len )

printf "Message: |%s|\nIt is %s = 0x%s bytes long.\n" "$msg" "$msg_byte_len" "$msg_byte_len_hex"
printf "%b%s" "\x$msg_byte_len_hex" "$msg" | netcat $ADDR $PORT

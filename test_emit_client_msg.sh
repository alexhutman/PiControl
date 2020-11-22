#!/usr/bin/env bash

ADDR=localhost
PORT=14741

# Input test commands/messages here
# Testing a SET_NAME command for example
cmd=5
msg="Alex's iPhone"

cmd_hex=$( printf "%x" $cmd )
msg_byte_len=${#msg}
msg_byte_len_hex=$( printf "%x" $msg_byte_len )

printf "Message: |%s|\nIt is %s = 0x%s bytes long.\n" "$msg" "$msg_byte_len" "$msg_byte_len_hex"
printf "Command: 0x%s\n" "$cmd_hex"
printf "%b%b%s" "\x$msg_byte_len_hex" "\x$cmd_hex" "$msg" | netcat $ADDR $PORT

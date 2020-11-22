#!/usr/bin/env bash

# NOTE: this currently only makes sense with ASCII characters
ADDR=localhost
PORT=14741

# Input test commands/messages here
# Testing a SET_NAME command for example
cmd=5
msg="Alex's iPhone"

cmd_hex=$( printf "%x" $cmd )
msg_byte_len=${#msg}
msg_byte_len_hex=$( printf "%x" $msg_byte_len )

printf "Command: 0x%s\n" "$cmd_hex"
printf "Payload size: %s = 0x%s\n" "$msg_byte_len" "$msg_byte_len_hex"
printf "Message: |%s|\n" "$msg"

printf "%b%b%s" "\x$cmd_hex" "\x$msg_byte_len_hex" "$msg" | netcat $ADDR $PORT

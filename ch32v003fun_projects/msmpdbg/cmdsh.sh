#!/bin/sh

stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb
echo "cu is starting. Type '~.' to exit."
cu -l /dev/ttyACM0

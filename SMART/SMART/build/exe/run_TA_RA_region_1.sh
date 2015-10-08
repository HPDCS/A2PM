#!/bin/sh
echo "Launching TA..."
./TA 172.31.13.51 > TA_output.log &
sleep 2
echo "Launching RA ..."
./RA > RA_output.log &

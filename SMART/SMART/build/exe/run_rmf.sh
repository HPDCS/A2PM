#!/bin/sh
rm nohup.out
echo "Launching router ..."
pkill router
nohup ./router exhaustive 1 &
echo "Launching forwarder ..."
pkill forwarder
nohup ./forwarder 121.0.0.1 &
echo "Launching monitor ..."
pkill monitor
nohup ./monitor 127.0.0.1 &

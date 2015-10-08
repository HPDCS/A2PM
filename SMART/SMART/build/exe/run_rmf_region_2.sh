#!/bin/sh
rm nohup.out
echo "Launching router ..."
pkill router
nohup ./router exhaustive 1 > router_output.log &
sleep 2
echo "Launching monitor ..."
pkill monitor
nohup ./monitor 127.0.0.1 > monitor_output.log &
sleep 2
echo "Launching forwarder ..."
pkill forwarder
nohup ./forwarder 127.0.0.1 > forwarder_output.log &


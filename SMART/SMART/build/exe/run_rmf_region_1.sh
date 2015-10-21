#!/bin/sh
rm nohup.out
echo "Launching router ..."
pkill router
nohup ./router exhaustive 1 > router_output.log &
sleep 2
echo "Launching monitor ..."
pkill monitor
nohup ./monitor 172.31.13.51 > monitor_output.log &
sleep 2
echo "Launching forwarder ..."
pkill forwarder
nohup ./forwarder 172.31.13.51 > forwarder_output.log &


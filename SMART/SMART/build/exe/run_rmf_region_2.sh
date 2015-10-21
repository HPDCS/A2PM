#!/bin/sh
rm nohup.out
echo "Launching router ..."
pkill router
nohup ./router exhaustive 2 > router_output.log &
sleep 2
echo "Launching monitor ..."
pkill monitor
nohup ./monitor 172.31.39.28 > monitor_output.log &
sleep 2
echo "Launching forwarder ..."
pkill forwarder
nohup ./forwarder 172.31.39.28 > forwarder_output.log &


#!/bin/bash

PLATFORM=$1
TUN_DEV=$2

#change, if needed
LOCAL_INTERFACE="eth0"

#hardcoded in android part, don't change.
TUNNEL_NET="10.10.10.0"
TUNNEL_CIDR="30"
HOST_ADDR="10.10.10.1"
DEVICE_ADDR="10.10.10.2"

if [ "$PLATFORM" = "linux" ]; then
    ifconfig $TUN_DEV $HOST_ADDR/$TUNNEL_CIDR up
    sysctl -w net.ipv4.ip_forward=1
    iptables -I FORWARD -j ACCEPT
    iptables -t nat -I POSTROUTING -s $TUNNEL_NET/$TUNNEL_CIDR -o $LOCAL_INTERFACE -j MASQUERADE
elif [ "$PLATFORM" = "osx" ]; then
    ifconfig $TUN_DEV $HOST_ADDR $DEVICE_ADDR up
else
    exit 1
fi

exit 0


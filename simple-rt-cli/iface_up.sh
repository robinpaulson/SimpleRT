#!/bin/bash

#params from simple-rt-cli
PLATFORM=$1
TUN_DEV=$2

#hardcoded in android part, don't change.
TUNNEL_NET="10.10.10.0"
TUNNEL_CIDR="30"
HOST_ADDR="10.10.10.1"
DEVICE_ADDR="10.10.10.2"

if [ "$PLATFORM" = "linux" ]; then
    #CHANGE, IF NEEDED
    LOCAL_INTERFACE="eth0"

    ifconfig $TUN_DEV $HOST_ADDR/$TUNNEL_CIDR up
    sysctl -w net.ipv4.ip_forward=1
    iptables -I FORWARD -j ACCEPT
    iptables -t nat -I POSTROUTING -s $TUNNEL_NET/$TUNNEL_CIDR -o $LOCAL_INTERFACE -j MASQUERADE

elif [ "$PLATFORM" = "osx" ]; then
    #CHANGE, IF NEEDED
    LOCAL_INTERFACE="en0"

    ifconfig $TUN_DEV $HOST_ADDR $DEVICE_ADDR up
    sysctl -w net.inet.ip.forwarding=1
    sysctl -w net.inet.ip.fw.enable=1
    echo "nat on $LOCAL_INTERFACE from $TUNNEL_NET/$TUNNEL_CIDR to any -> ($LOCAL_INTERFACE)" > /tmp/nat_rules_rt
    pfctl -d
    pfctl -F all
    pfctl -f /tmp/nat_rules_rt -e
else
    exit 1
fi

exit 0


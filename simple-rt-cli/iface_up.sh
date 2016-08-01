#!/bin/bash
# SimpleRT: Reverse tethering utility for Android
# Copyright (C) 2016 Konstantin Menyaev
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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


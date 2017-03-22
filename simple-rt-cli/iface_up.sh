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
TUNNEL_NET=$3
HOST_ADDR=$4
TUNNEL_CIDR=$5
NAMESERVER=$6
LOCAL_INTERFACE=$7

echo configuring:
echo local interface:       $LOCAL_INTERFACE
echo virtual interface:     $TUN_DEV
echo network:               $TUNNEL_NET
echo address:               $HOST_ADDR
echo netmask:               $TUNNEL_CIDR
echo nameserver:            $NAMESERVER

ip l show $LOCAL_INTERFACE > /dev/null
if [ ! $? -eq 0 ]; then
    echo Supply valid local interface!
    exit 1
fi

if [ "$PLATFORM" = "linux" ]; then
    ifconfig $TUN_DEV $HOST_ADDR/$TUNNEL_CIDR up
    sysctl -w net.ipv4.ip_forward=1 > /dev/null
    iptables -I FORWARD -j ACCEPT
    iptables -t nat -I POSTROUTING -s $TUNNEL_NET/$TUNNEL_CIDR -o $LOCAL_INTERFACE -j MASQUERADE
else
    exit 1
fi

exit 0


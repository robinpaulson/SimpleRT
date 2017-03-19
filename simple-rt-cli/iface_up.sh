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

TUNNEL_NET="10.1.1.0"
TUNNEL_CIDR="24"
HOST_ADDR="10.1.1.1"

echo configuring $TUN_DEV

if [ "$PLATFORM" = "linux" ]; then
    #CHANGE, IF NEEDED
    LOCAL_INTERFACE="eth0"

    ifconfig $TUN_DEV $HOST_ADDR/$TUNNEL_CIDR up
    sysctl -w net.ipv4.ip_forward=1
    iptables -I FORWARD -j ACCEPT
    iptables -t nat -I POSTROUTING -s $TUNNEL_NET/$TUNNEL_CIDR -o $LOCAL_INTERFACE -j MASQUERADE
else
    exit 1
fi

exit 0


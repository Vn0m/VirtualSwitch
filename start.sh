#!/bin/bash
set -e

if [ $# -lt 2 ]; then
    echo "Usage: sudo $0 <your_tap_ip> <friend_public_ip:port>"
    echo "Example: sudo $0 10.0.0.1 203.0.113.5:36950"
    exit 1
fi

YOUR_IP=$1
FRIEND_IP=$2

ip tuntap add dev tap0 mode tap 2>/dev/null || true
ip addr flush dev tap0
ip addr add $YOUR_IP/24 dev tap0
ip link set tap0 up

./build/vswitch --local tap0 --stun stun.l.google.com:19302 --udp 0.0.0.0:5000:$FRIEND_IP
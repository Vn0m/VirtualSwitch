#!/bin/bash
set -e

TAP_IP="${TAP_IP:-10.0.0.1}"
PEERS="${PEERS}"
PORT="${PORT:-5000}"

if [ -z "$PEERS" ]; then
    echo "ERROR: PEERS env var are required (comma separated, e.g. 1.2.3.4:5000,5.6.7.8:5000)"
    exit 1
fi

ip tuntap add dev tap0 mode tap
ip addr add "${TAP_IP}/24" dev tap0
ip link set tap0 up

UDP_ARGS=""
IFS=',' read -ra PEER_LIST <<< "$PEERS"
for peer in "${PEER_LIST[@]}"; do
    UDP_ARGS="$UDP_ARGS --udp 0.0.0.0:${PORT}:${peer}"
done

exec vswitch --local tap0 --stun stun.l.google.com:19302 $UDP_ARGS

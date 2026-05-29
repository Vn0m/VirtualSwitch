#!/bin/bash
set -e

TAP_IP="${TAP_IP:-10.0.0.1}"
PEERS="${PEERS}"
PORT="${PORT:-5000}"
OPENVPN_KEY="${OPENVPN_KEY:-}"

if [ -z "$PEERS" ]; then
    echo "ERROR: PEERS env var required (comma separated, e.g. 1.2.3.4:5000,5.6.7.8:5000)"
    exit 1
fi

if [ -z "$OPENVPN_KEY" ]; then
    openvpn --genkey secret /tmp/static.key
    echo "OPENVPN_KEY_GENERATED:$(base64 -w 0 < /tmp/static.key)"
else
    echo "$OPENVPN_KEY" | base64 -d > /tmp/static.key
fi

cat > /tmp/server.conf << EOF
dev tap_vpn
dev-type tap
proto udp
port 1194
secret /tmp/static.key
keepalive 10 60
verb 3
EOF

openvpn --config /tmp/server.conf &

for i in $(seq 1 15); do
    ip link show tap_vpn > /dev/null 2>&1 && break
    sleep 1
done

ip tuntap add tap0 mode tap
ip link set tap_vpn up
ip link set tap0 up
ip link add name br0 type bridge
ip link set tap_vpn master br0
ip link set tap0 master br0
ip link set br0 up

UDP_ARGS=""
IFS=',' read -ra PEER_LIST <<< "$PEERS"
for peer in "${PEER_LIST[@]}"; do
    UDP_ARGS="$UDP_ARGS --udp 0.0.0.0:${PORT}:${peer}"
done

exec vswitch --local tap0 --stun stun.l.google.com:19302 $UDP_ARGS

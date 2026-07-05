#!/bin/bash
set -e

TAP_IP="${TAP_IP:-10.0.0.1}"
PEERS="${PEERS}"
PORT="${PORT:-5000}"
TUNNEL_PORT="${TUNNEL_PORT:-1194}"
KEY="${KEY:-}"

if [ -z "$PEERS" ]; then
    echo "ERROR: PEERS env var required (comma separated, e.g. 1.2.3.4:5000,5.6.7.8:5000)"
    exit 1
fi

ip tuntap add tap0 mode tap
ip addr add 10.0.0.1/24 dev tap0
ip link set tap0 up
if [ -n "$KEY" ]; then
    ip link set tap0 mtu 1414
else
    ip link set tap0 mtu 1454
fi
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 1 > /proc/sys/net/ipv4/conf/tap0/arp_accept
echo 1 > /proc/sys/net/ipv4/conf/all/arp_accept

relay "$TUNNEL_PORT" &

for i in $(seq 1 15); do
    ip link show tun0 > /dev/null 2>&1 && break
    sleep 1
done

ip addr add 10.255.0.1 peer "$TAP_IP" dev tun0
ip link set tun0 up

UDP_ARGS=""
IFS=',' read -ra PEER_LIST <<< "$PEERS"
for peer in "${PEER_LIST[@]}"; do
    UDP_ARGS="$UDP_ARGS --udp 0.0.0.0:${PORT}:${peer}"
done

python3 - "$TAP_IP" << 'PYEOF' &
import socket, struct, time, sys
IP = sys.argv[1]
IFACE = 'tap0'
def send():
    with open(f'/sys/class/net/{IFACE}/address') as f:
        mac = bytes.fromhex(f.read().strip().replace(':', ''))
    ip_b = socket.inet_aton(IP)
    arp = struct.pack('!HHBBH', 1, 0x0800, 6, 4, 2) + mac + ip_b + b'\xff\xff\xff\xff\xff\xff' + ip_b
    eth = b'\xff\xff\xff\xff\xff\xff' + mac + b'\x08\x06' + arp
    s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x0806))
    s.bind((IFACE, 0))
    s.send(eth)
    s.close()
while True:
    try: send()
    except: pass
    time.sleep(5)
PYEOF

KEY_ARGS=""
if [ -n "$KEY" ]; then
    KEY_ARGS="--key $KEY"
fi

exec vswitch --local tap0 --stun stun.l.google.com:19302 $UDP_ARGS $KEY_ARGS

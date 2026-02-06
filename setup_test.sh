#!/bin/bash
set -e

ip netns delete ns1 2>/dev/null || true
ip netns delete ns2 2>/dev/null || true
ip netns delete ns3 2>/dev/null || true
ip netns delete ns4 2>/dev/null || true
ip netns add ns1
ip netns add ns2
ip netns add ns3
ip netns add ns4

for i in {1..10}; do
    if ip link show tap0 &>/dev/null && ip link show tap1 &>/dev/null && ip link show tap2 &>/dev/null && ip link show tap3 &>/dev/null; then
        echo "       TAP devices found!"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "ERROR: TAP devices not found. Make sure vswitch is running!"
        exit 1
    fi
    sleep 0.5
done

ip link set tap0 netns ns1
ip link set tap1 netns ns2
ip link set tap2 netns ns3
ip link set tap3 netns ns4

ip netns exec ns1 ip addr add 10.0.0.1/24 dev tap0
ip netns exec ns1 ip link set tap0 up
ip netns exec ns1 ip link set lo up

ip netns exec ns2 ip addr add 10.0.0.2/24 dev tap1
ip netns exec ns2 ip link set tap1 up
ip netns exec ns2 ip link set lo up

ip netns exec ns3 ip addr add 10.0.0.3/24 dev tap2
ip netns exec ns3 ip link set tap2 up
ip netns exec ns3 ip link set lo up

ip netns exec ns4 ip addr add 10.0.0.4/24 dev tap3
ip netns exec ns4 ip link set tap3 up
ip netns exec ns4 ip link set lo up

echo "=== Setup Complete ==="

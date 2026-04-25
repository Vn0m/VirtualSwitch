#!/bin/bash
set -e

usage() {
    echo "Usage: sudo $0 <test|cleanup>"
    echo "  test    - start vswitch, run all ping tests, then cleanup automatically"
    echo "  cleanup - manually delete namespaces and kill vswitch"
    exit 1
}

cleanup() {
    ip netns delete ns1 2>/dev/null || true
    ip netns delete ns2 2>/dev/null || true
    ip netns delete ns3 2>/dev/null || true
    ip netns delete ns4 2>/dev/null || true
    pkill -f "vswitch.*--local tap0" 2>/dev/null || true
}

test_all() {
    echo "=== Cleaning up any previous state ==="
    cleanup

    echo "=== Starting vswitch with 4 TAPs ==="
    ./build/vswitch --local tap0 --local tap1 --local tap2 --local tap3 &
    VSWITCH_PID=$!

    echo "=== Waiting for TAP devices ==="
    for i in {1..10}; do
        if ip link show tap0 &>/dev/null && ip link show tap1 &>/dev/null && \
           ip link show tap2 &>/dev/null && ip link show tap3 &>/dev/null; then
            echo "TAP devices found!"
            break
        fi
        if [ $i -eq 10 ]; then
            echo "ERROR: TAP devices not found."
            kill $VSWITCH_PID 2>/dev/null
            exit 1
        fi
        sleep 0.5
    done

    echo "=== Setting up network namespaces ==="
    ip netns add ns1
    ip netns add ns2
    ip netns add ns3
    ip netns add ns4

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

    echo ""
    echo "=== Running Tests ==="
    PASS=0
    FAIL=0

    run_test() {
        local desc=$1
        local from=$2
        local to=$3
        printf "  %-30s" "$desc"
        if ip netns exec $from ping -c 1 -W 2 $to &>/dev/null; then
            echo "PASS"
            PASS=$((PASS + 1))
        else
            echo "FAIL"
            FAIL=$((FAIL + 1))
        fi
    }

    run_test "ns1 -> ns2 (10.0.0.1 -> 10.0.0.2)" ns1 10.0.0.2
    run_test "ns1 -> ns3 (10.0.0.1 -> 10.0.0.3)" ns1 10.0.0.3
    run_test "ns1 -> ns4 (10.0.0.1 -> 10.0.0.4)" ns1 10.0.0.4
    run_test "ns2 -> ns3 (10.0.0.2 -> 10.0.0.3)" ns2 10.0.0.3
    run_test "ns2 -> ns4 (10.0.0.2 -> 10.0.0.4)" ns2 10.0.0.4
    run_test "ns3 -> ns4 (10.0.0.3 -> 10.0.0.4)" ns3 10.0.0.4

    echo ""
    echo "=== Results: $PASS passed, $FAIL failed ==="

    echo "=== Cleaning up ==="
    cleanup
    echo "Done."

    [ $FAIL -eq 0 ] && exit 0 || exit 1
}

case "$1" in
    test)    test_all ;;
    cleanup) cleanup; echo "=== Cleanup Complete ===" ;;
    *)       usage ;;
esac

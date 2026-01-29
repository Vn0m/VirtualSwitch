#!/bin/bash

ip netns delete ns1 2>/dev/null || echo "  ns1 not found (already deleted?)"

ip netns delete ns2 2>/dev/null || echo "  ns2 not found (already deleted?)"

echo "=== Cleanup Complete ==="

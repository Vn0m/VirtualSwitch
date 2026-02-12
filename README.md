<div align="center">

# VirtualSwitch

**Layer 2 virtual switch with UDP tunneling for LAN gaming over the internet**

![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Shell](https://img.shields.io/badge/Shell-121011?style=for-the-badge&logo=gnu-bash&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![UDP](https://img.shields.io/badge/UDP-010101?style=for-the-badge&logo=protocol&logoColor=white)
![TAP](https://img.shields.io/badge/TAP%20Devices-000000?style=for-the-badge)

</div>

---

Play Minecraft, Valheim, or any LAN game with friends across the internet without subscriptions or port forwarding. VirtualSwitch makes your computers appear on the same local network.

## Features

- LAN Gaming Over Internet - Play peer-to-peer without dedicated servers
- UDP Tunneling - Transparent frame forwarding across internet
- MAC Learning - Automatic MAC address table with 300s TTL aging
- Efficient - Poll-based event loop for minimal overhead (~0.3MB/s Minecraft)
- Multi-port - Support unlimited local TAPs and UDP peers
- No Setup - Works same machine, LAN, or internet

## Build

```bash
mkdir -p build && cd build
cmake ..
make
sudo ./vswitch --help
```

Requirements: Linux, C++17, CMake 3.15+

## Quick Start

**Computer A:**
```bash
sudo ./vswitch --local tap0 --udp 0.0.0.0:5000:B_IP:5000
sudo ip addr add 10.0.0.1/24 dev tap0
sudo ip link set tap0 up
```

**Computer B:**
```bash
sudo ./vswitch --local tap0 --udp 0.0.0.0:5000:A_IP:5000
sudo ip addr add 10.0.0.2/24 dev tap0
sudo ip link set tap0 up
```

**Test:**
```bash
ping 10.0.0.2  
```

Then start Minecraft on both machines - server appears in LAN list automatically.

Find your public IP:
```bash
curl ifconfig.me
```

## Architecture

**Frame Flow:**
1. App sends packet → TAP device
2. VirtualSwitch reads frame, learns source MAC
3. MAC table lookup for destination
4. Forward to local TAP or encapsulate and send via UDP
5. Remote switch receives UDP, decapsulates, forwards to local TAP
6. Remote app receives normally

**Frame Encapsulation:** `[4-byte length][Ethernet frame]`

## Usage

```
./vswitch [OPTIONS]

  --local <name>            Add local TAP device
  --udp <local_ip:port:remote_ip:port>  Add UDP peer
  --help                    Show help

Example (3+ players):
  sudo ./vswitch --local tap0 \
    --udp 0.0.0.0:5000:B_IP:5000 \
    --udp 0.0.0.0:5001:C_IP:5000
```

## License

This project is licensed under the MIT License - see LICENSE.txt for details.


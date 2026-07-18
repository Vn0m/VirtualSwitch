<div align="center">

<img src="allbluelogo.png" alt="AllBlue logo" width="120" />

# AllBlue

**Layer 2 virtual switch with UDP tunneling for LAN gaming over the internet**

![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Shell](https://img.shields.io/badge/Shell-121011?style=for-the-badge&logo=gnu-bash&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![UDP](https://img.shields.io/badge/UDP-010101?style=for-the-badge&logo=protocol&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white)

![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/macOS-000000?style=for-the-badge&logo=apple&logoColor=white)

</div>

---

Play Terraria, Minecraft, Valheim, or any LAN game with friends across the internet without subscriptions or port forwarding. AllBlue makes your computers appear on the same local network.

## Demo

Two Macs on different networks, playing Terraria over a direct peer-to-peer link:

https://github.com/user-attachments/assets/f8aa9219-1abc-42eb-b71a-9ceac982ec45

Two ways to use it:

- **macOS — AllBlue app**: a Qt GUI that runs the switch in a Docker container and bridges your Mac into the virtual network through a native `utun` interface.
- **Linux — `vswitch` CLI**: the switch runs natively against a kernel TAP device.

## Features

- LAN Gaming Over Internet - Play peer-to-peer without dedicated servers
- NAT Hole Punching - Automatic STUN port probing finds a stable external port through your router
- Keep-Alive Punches - Periodic packets keep NAT mappings alive so connections don't drop
- Port Learning - Adapts to peers whose NAT remaps ports mid-connection
- Optional Encryption (CLI) - XChaCha20-Poly1305 via a pre-shared `--key`; see [CLI Usage](#cli-usage)
- MAC Learning - Automatic MAC address table with 300s TTL aging
- Proxy ARP - Cached IP-to-MAC entries answer ARP locally so peers reconnect fast
- Broadcast/Multicast - ARP and game discovery frames correctly flooded to all peers
- Efficient - Poll-based event loop, zero per-frame heap allocations
- Native macOS Tunnel - Built-in `utun` helper

## macOS (AllBlue app)

Requirements: Docker (OrbStack or Docker Desktop), Qt 6, CMake 3.21+

```bash
docker build -t allblue-vswitch .

cmake -B build && cmake --build build
open build/gui/allblue.app
```

Using it:

1. Launch AllBlue. It picks a persistent virtual IP for your Mac (shown as *virtual address*).
2. Click **+ Add peer** and enter your friend's public `ip:port`.
3. The app starts the switch, probes STUN, and shows *your* public address — copy it and share it with your peer.
4. macOS will ask for your password once: that authorizes the `allblue-utun` helper, which creates the native tunnel interface and routes `10.0.0.0/24` into the virtual LAN.
5. Start your game and join via direct IP.

See [Architecture](#architecture) for how traffic actually flows from your game to your peer.

## Linux (CLI)

Requirements: C++17, CMake 3.21+, libsodium

```bash
mkdir -p build && cd build
cmake ..
make
```

Both peers need each other's public address — the switch prints yours at startup. Run this on **both** machines, using your own local IP and your peer's public address:

```bash
sudo ip tuntap add dev tap0 mode tap
sudo ip addr add 10.0.0.1/24 dev tap0   # use 10.0.0.2 on the other peer
sudo ip link set tap0 up
sudo ip link set tap0 mtu 1454
sudo ./build/vswitch --local tap0 --stun stun.l.google.com:19302 --udp 0.0.0.0:5000:PEER_PUBLIC_IP:5000
```

The switch probes ports automatically and prints your stable public address:
```
Port 5000 -> external 5001, trying next...
Your public address: 1.2.3.4:5002  <-- share this with your peer
```

If your NAT remaps ports (symmetric NAT), the switch warns you and continues — this still works if the other peer has a regular home router. Note: some university and corporate networks block outbound UDP entirely, which prevents direct hole punching regardless of NAT type.

Test with `ping 10.0.0.2`, then start your game and connect via direct IP (`10.0.0.1` / `10.0.0.2`).

To encrypt this link, add `--key <hex_key>` (see [CLI Usage](#cli-usage)) — generate one with `head -c 32 /dev/urandom | xxd -p -c 64` and share it with your peer out of band. The switch sets the TAP MTU for you based on whether a key is present.

## Architecture

![Architecture](architecture.png)

**Frame Flow:**
1. App sends packet → TAP device (Linux) or utun → relay → TAP (macOS)
2. The switch reads the frame, learns the source MAC
3. Broadcast/multicast → flood to all peers. Unicast → MAC table lookup
4. Known destination → forward directly. Unknown → flood to all peers
5. UDP peers: frame is encapsulated and sent over the internet
6. Remote switch decapsulates, forwards to its local TAP → remote app receives normally

**Frame Encapsulation:**
- No encryption: `[4-byte length][Ethernet frame]`
- With encryption: `[4-byte length][24-byte nonce][ciphertext + 16-byte tag]` — XChaCha20-Poly1305 (libsodium). Packets that fail authentication are dropped, and NAT port remaps are only honored from packets that decrypt successfully.

## CLI Usage

```
./vswitch [OPTIONS]

  --local <name>            Add local TAP device
  --udp <local_ip:port:remote_ip:port>  Add UDP peer
  --stun <host:port>        STUN server for public address discovery
  --key <hex>               32-byte pre-shared key in hex (enables XChaCha20-Poly1305 encryption)
  --help                    Show help

Example (3+ players):
  sudo ./vswitch --local tap0 \
    --udp 0.0.0.0:5000:B_IP:5000 \
    --udp 0.0.0.0:5001:C_IP:5000
```

## License

This project is licensed under the MIT License - see LICENSE.txt for details.

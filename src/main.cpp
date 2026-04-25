#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include "network/tap_device.hpp"
#include "network/udp_port.hpp"
#include "network/stun_client.hpp"
#include "switch/virtual_switch.hpp"

struct Args {
    std::vector<std::string> taps;
    std::vector<std::string> udp_peers;
    std::string stun_server;
};

struct HostPort {
    std::string host;
    uint16_t port;
};

struct UdpPeerConfig {
    std::string local_ip;
    uint16_t    local_port;
    std::string remote_ip;
    uint16_t    remote_port;
};

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [OPTIONS]\n"
              << "  --local <tap_name>\n"
              << "  --udp   <local_ip:local_port:remote_ip:remote_port>\n"
              << "  --stun  <host:port>\n"
              << "Example:\n"
              << "  " << prog << " --local tap0 --stun stun.l.google.com:19302 --udp 0.0.0.0:5000:PEER_IP:5000\n";
}

Args parse_args(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--local" && i + 1 < argc) args.taps.push_back(argv[++i]);
        else if (arg == "--udp"   && i + 1 < argc) args.udp_peers.push_back(argv[++i]);
        else if (arg == "--stun"  && i + 1 < argc) args.stun_server = argv[++i];
        else if (arg == "--help"  || arg == "-h")  { print_usage(argv[0]); exit(0); }
        else { std::cerr << "Unknown option: " << arg << "\n"; print_usage(argv[0]); exit(1); }
    }
    return args;
}

HostPort parse_host_port(const std::string& s) {
    size_t colon = s.rfind(':');
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid host:port format: " + s);
    return { s.substr(0, colon), static_cast<uint16_t>(std::stoi(s.substr(colon + 1))) };
}

UdpPeerConfig parse_udp_peer(const std::string& s) {
    size_t c1 = s.find(':');
    size_t c2 = s.find(':', c1 + 1);
    size_t c3 = s.find(':', c2 + 1);
    if (c1 == std::string::npos || c2 == std::string::npos || c3 == std::string::npos)
        throw std::runtime_error("Invalid UDP peer format: " + s);
    return {
        s.substr(0, c1),
        static_cast<uint16_t>(std::stoi(s.substr(c1 + 1, c2 - c1 - 1))),
        s.substr(c2 + 1, c3 - c2 - 1),
        static_cast<uint16_t>(std::stoi(s.substr(c3 + 1)))
    };
}

void add_tap(vswitch::VirtualSwitch& sw, const std::string& name) {
    auto tap = std::make_unique<vswitch::TapDevice>(name);
    std::cout << "Created TAP device: " << tap->get_name() << "\n";
    sw.add_port(std::move(tap));
}

void add_udp_peer(vswitch::VirtualSwitch& sw, const std::string& peer_str,
                  const std::string& stun_host, uint16_t stun_port, bool& stun_done) {
    auto cfg = parse_udp_peer(peer_str);
    std::string name = "udp_" + cfg.remote_ip + ":" + std::to_string(cfg.remote_port);

    auto udp = std::make_unique<vswitch::UdpPort>(name, cfg.local_ip, cfg.local_port,
                                                        cfg.remote_ip, cfg.remote_port);
    std::cout << "Created UDP port: " << udp->get_name()
              << " (local=" << cfg.local_ip << ":" << cfg.local_port
              << ", remote=" << cfg.remote_ip << ":" << cfg.remote_port << ")\n";

    if (!stun_done && !stun_host.empty()) {
        try {
            vswitch::StunClient stun(stun_host, stun_port, udp->get_fd());
            auto addr = stun.get_public_address();
            std::cout << "\nYour public address: " << addr.ip << ":" << addr.port
                      << "  <-- share this with your peer\n\n";
        } catch (const std::exception& e) {
            std::cerr << "STUN failed: " << e.what() << "\n";
        }
        stun_done = true;
    }

    std::cout << "Punching NAT hole to " << cfg.remote_ip << ":" << cfg.remote_port << "...\n";
    udp->punch();

    sw.add_port(std::move(udp));
}

int main(int argc, char* argv[]) {
    try {
        auto args = parse_args(argc, argv);

        if (args.taps.empty() && args.udp_peers.empty()) {
            std::cerr << "No ports configured. Use --local or --udp to add ports.\n";
            print_usage(argv[0]);
            return 1;
        }

        vswitch::VirtualSwitch sw;

        for (const auto& name : args.taps)
            add_tap(sw, name);

        std::string stun_host;
        uint16_t stun_port = 0;
        if (!args.stun_server.empty()) {
            auto hp = parse_host_port(args.stun_server);
            stun_host = hp.host;
            stun_port = hp.port;
        }

        bool stun_done = false;
        for (const auto& peer : args.udp_peers)
            add_udp_peer(sw, peer, stun_host, stun_port, stun_done);

        std::cout << "\nSwitch running. Waiting for frames...\n\n";
        sw.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

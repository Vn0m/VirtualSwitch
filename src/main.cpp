#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include "network/tap_device.hpp"
#include "network/udp_port.hpp"
#include "network/stun_client.hpp"
#include "switch/virtual_switch.hpp"

void print_usage(const char* prog_name) {
	std::cerr << "Usage: " << prog_name << " [OPTIONS]\n"
	          << "Options:\n"
	          << "  --local <tap_name>                         Add local TAP device\n"
	          << "  --udp <local_ip:local_port:remote_ip:remote_port>  Add UDP peer\n"
	          << "  --stun <host:port>                         Discover public IP via STUN\n"
	          << "Example:\n"
	          << "  " << prog_name << " --local tap0 --stun stun.l.google.com:19302 --udp 0.0.0.0:5000:PEER_IP:5000\n";
}

int main(int argc, char* argv[]) {
	try {
		vswitch::VirtualSwitch vswitch;
		std::vector<std::string> local_taps;
		std::vector<std::string> udp_peers;
		std::string stun_server;

		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--local" && i + 1 < argc) {
				local_taps.push_back(argv[++i]);
			} else if (arg == "--udp" && i + 1 < argc) {
				udp_peers.push_back(argv[++i]);
			} else if (arg == "--stun" && i + 1 < argc) {
				stun_server = argv[++i];
			} else if (arg == "--help" || arg == "-h") {
				print_usage(argv[0]);
				return 0;
			} else {
				std::cerr << "Unknown option: " << arg << "\n";
				print_usage(argv[0]);
				return 1;
			}
		}

		for (const auto& tap_name : local_taps) {
			auto tap = std::make_unique<vswitch::TapDevice>(tap_name);
			std::cout << "Created TAP device: " << tap->get_name() << std::endl;
			vswitch.add_port(std::move(tap));
		}

		std::string stun_host;
		uint16_t stun_port = 0;
		if (!stun_server.empty()) {
			size_t colon = stun_server.rfind(':');
			if (colon == std::string::npos) {
				std::cerr << "Invalid STUN server format. Expected host:port\n";
				return 1;
			}
			stun_host = stun_server.substr(0, colon);
			stun_port = static_cast<uint16_t>(std::stoi(stun_server.substr(colon + 1)));
		}

		bool stun_done = false;
		for (const auto& peer : udp_peers) {
			size_t first_colon = peer.find(':');
			size_t second_colon = peer.find(':', first_colon + 1);
			size_t third_colon = peer.find(':', second_colon + 1);

			if (first_colon == std::string::npos ||
			    second_colon == std::string::npos ||
			    third_colon == std::string::npos) {
				std::cerr << "Invalid UDP peer format: " << peer << "\n";
				print_usage(argv[0]);
				return 1;
			}

			std::string local_ip = peer.substr(0, first_colon);
			std::string local_port_str = peer.substr(first_colon + 1, second_colon - first_colon - 1);
			std::string remote_ip = peer.substr(second_colon + 1, third_colon - second_colon - 1);
			std::string remote_port_str = peer.substr(third_colon + 1);

			uint16_t local_port = static_cast<uint16_t>(std::stoi(local_port_str));
			uint16_t remote_port = static_cast<uint16_t>(std::stoi(remote_port_str));

			std::string name = "udp_" + remote_ip + ":" + remote_port_str;
			auto udp = std::make_unique<vswitch::UdpPort>(name, local_ip, local_port, remote_ip, remote_port);
			std::cout << "Created UDP port: " << udp->get_name()
			          << " (local=" << local_ip << ":" << local_port
			          << ", remote=" << remote_ip << ":" << remote_port << ")" << std::endl;

			if (!stun_done && !stun_host.empty()) {
				try {
					vswitch::StunClient stun(stun_host, stun_port, udp->get_fd());
					auto addr = stun.get_public_address();
					std::cout << "\nYour public address: " << addr.ip << ":" << addr.port
					          << "  <-- share this with your peer\n" << std::endl;
				} catch (const std::exception& e) {
					std::cerr << "STUN failed: " << e.what() << std::endl;
				}
				stun_done = true;
			}

			std::cout << "Punching NAT hole to " << remote_ip << ":" << remote_port << "..." << std::endl;
			udp->punch();

			vswitch.add_port(std::move(udp));
		}

		if (local_taps.empty() && udp_peers.empty()) {
			std::cerr << "No ports configured. Use --local or --udp to add ports.\n";
			print_usage(argv[0]);
			return 1;
		}

		std::cout << "\nSwitch running. Waiting for frames...\n" << std::endl;
		vswitch.run();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

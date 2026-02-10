#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include "network/tap_device.hpp"
#include "network/udp_port.hpp"
#include "switch/virtual_switch.hpp"

void print_usage(const char* prog_name) {
	std::cerr << "Usage: " << prog_name << " [OPTIONS]\n"
	          << "Options:\n"
	          << "  --local <tap_name>           Add local TAP device\n"
	          << "  --udp <local_ip:local_port:remote_ip:remote_port>  Add UDP peer\n"
	          << "Example:\n"
	          << "  " << prog_name << " --local tap0 --udp 0.0.0.0:5000:192.168.1.5:5000\n";
}

int main(int argc, char* argv[]) {
	try {
		vswitch::VirtualSwitch vswitch;
		std::vector<std::string> local_taps;
		std::vector<std::string> udp_peers;

		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--local" && i + 1 < argc) {
				local_taps.push_back(argv[++i]);
			} else if (arg == "--udp" && i + 1 < argc) {
				udp_peers.push_back(argv[++i]);
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

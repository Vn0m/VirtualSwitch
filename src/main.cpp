#include <iostream>
#include <memory>
#include "network/tap_device.hpp"
#include "switch/virtual_switch.hpp"

int main(){
	try{
		vswitch::VirtualSwitch vswitch;
		auto tap0 = std::make_unique<vswitch::TapDevice>("tap0");
		auto tap1 = std::make_unique<vswitch::TapDevice>("tap1");
		auto tap2 = std::make_unique<vswitch::TapDevice>("tap2");
		auto tap3 = std::make_unique<vswitch::TapDevice>("tap3");
		std::cout << "created tap device: " << tap0->get_name() << std::endl;
		std::cout << "created tap device: " << tap1->get_name() << std::endl;
		std::cout << "created tap device: " << tap2->get_name() << std::endl;
		std::cout << "created tap device: " << tap3->get_name() << std::endl;

		vswitch.add_port(std::move(tap0));
		vswitch.add_port(std::move(tap1));
		vswitch.add_port(std::move(tap2));
		vswitch.add_port(std::move(tap3));
		std::cout << "Switch running. Waiting for frames..." << std::endl;
		vswitch.run();
	} catch(const std::exception& e){
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

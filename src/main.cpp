#include <iostream>
#include "network/tap_device.hpp"
#include "core/ethernet_frame.hpp"
#include "switch/virtual_switch.hpp"

int main(){
	try{
		vswitch::VirtualSwitch vswitch;
		vswitch::TapDevice* tap0 = new vswitch::TapDevice("tap0");
		vswitch::TapDevice* tap1 = new vswitch::TapDevice("tap1");
		std::cout << "created tap device: " << tap0->get_name() << std::endl;
		std::cout << "created tap device: " << tap1->get_name() << std::endl;

		vswitch.add_port(tap0);
		vswitch.add_port(tap1);

		std::cout << "Switch running and waiting for frames" << std::endl;
		vswitch.run();

	} catch(const std::exception& e){
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

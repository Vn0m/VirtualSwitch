#include <iostream>
#include "network/tap_device.hpp"
#include "core/ethernet_frame.hpp"

int main(){
    try{
        vswitch::TapDevice tap("tap0");
        std::cout << "created tap device: " << tap.get_name() << std::endl;

        std::cout << "waiting for a frame!" << std::endl;
        std::vector<uint8_t> raw_frame = tap.read_frame();
        std::cout << "got frame of size: " << raw_frame.size() << " bytes" << std::endl;

        vswitch::EthernetFrame frame(raw_frame);
        std::cout << "frame info: " << frame.to_string() << std::endl;

    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }


    return 0;
}

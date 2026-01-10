#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "network/tap_device.hpp"
#include "core/ethernet_frame.hpp"

namespace vswitch {
class VirtualSwitch {
public:
    // constructor
    VirtualSwitch();
    // deconstructor
    ~VirtualSwitch();
    VirtualSwitch(const VirtualSwitch&) = delete;
    VirtualSwitch& operator = (const VirtualSwitch&) = delete;

    // add a TAP device as a port to the switch
    void add_port(TapDevice* tap_device);

    // learn MAC address when frame is received
    void learn_mac_address(const std::array<uint8_t, 6>& mac_address, TapDevice* port);

    // forward frame to appropriate port or flood if unknown
    void forward_frame(const EthernetFrame& frame, TapDevice* source_port);

    // print MAC address table
    std::string mac_table_to_string() const;

    // main loop to process frames
    void run();

private:
    // mapping MAC addresses to ports 
    std::unordered_map<std::string, TapDevice*> mac_address_table_;
    // list of ports (TAP devices) connected to the switch
    std::vector<TapDevice*> ports_;
};

} // namespace vswitch
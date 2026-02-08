#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <chrono>
#include "network/tap_device.hpp"
#include "core/ethernet_frame.hpp"

namespace vswitch {

struct MacEntry {
    TapDevice* port;
    std::chrono::steady_clock::time_point learned_at;
};
class VirtualSwitch {
public:
    VirtualSwitch();
    ~VirtualSwitch();
    VirtualSwitch(const VirtualSwitch&) = delete;
    VirtualSwitch& operator = (const VirtualSwitch&) = delete;

    void add_port(std::unique_ptr<TapDevice> tap_device);
    void learn_mac_address(const std::array<uint8_t, 6>& mac_address, TapDevice* port);
    void forward_frame(const EthernetFrame& frame, TapDevice* source_port);
    std::string mac_table_to_string() const;
    void run();

private:
    static constexpr int FDB_AGING_TIME = 300;
    std::unordered_map<std::string, MacEntry> mac_address_table_;
    std::vector<std::unique_ptr<TapDevice>> ports_;
    
    void age_out_mac_entries();
};

} // namespace vswitch
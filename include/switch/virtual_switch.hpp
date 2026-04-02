#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <chrono>
#include "network/port.hpp"
#include "core/ethernet_frame.hpp"

namespace vswitch {

struct MacEntry {
    Port* port;
    std::chrono::steady_clock::time_point learned_at;
};

class VirtualSwitch {
public:
    VirtualSwitch() = default;
    ~VirtualSwitch() = default;
    VirtualSwitch(const VirtualSwitch&) = delete;
    VirtualSwitch& operator=(const VirtualSwitch&) = delete;

    void add_port(std::unique_ptr<Port> port);
    void learn_mac_address(const std::array<uint8_t, 6>& mac_address, Port* port);
    void forward_frame(const EthernetFrame& frame, const uint8_t* data, size_t len, Port* source_port);
    void run();

private:
    static constexpr int FDB_AGING_TIME    = 300;
    static constexpr int AGE_CHECK_INTERVAL = 5;

    std::unordered_map<std::string, MacEntry> mac_address_table_;
    std::vector<std::unique_ptr<Port>> ports_;
    std::chrono::steady_clock::time_point last_age_check_ = std::chrono::steady_clock::now();

    void age_out_mac_entries();
};

} // namespace vswitch

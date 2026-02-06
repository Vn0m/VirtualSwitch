#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace vswitch {

class EthernetFrame {
public:
    explicit EthernetFrame(const std::vector<uint8_t>& raw_frame);
    
    const std::vector<uint8_t>& get_raw_frame() const;
    const std::array<uint8_t, 6>& get_dst_mac() const;
    const std::array<uint8_t, 6>& get_src_mac() const;
    uint16_t get_ethertype() const;
    const std::vector<uint8_t>& get_payload() const;
    
    static std::string mac_to_string(const std::array<uint8_t, 6>& mac);
    std::string to_string() const;

private:
    std::vector<uint8_t> raw_frame_;
    std::array<uint8_t, 6> dst_mac_;
    std::array<uint8_t, 6> src_mac_;
    uint16_t ethertype_;
    std::vector<uint8_t> payload_;
};
} // namespace vswitch
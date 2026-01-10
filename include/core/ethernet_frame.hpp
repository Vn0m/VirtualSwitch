#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace vswitch {

class EthernetFrame {
public:
    // parse raw Ethernet frame bytes
    explicit EthernetFrame(const std::vector<uint8_t>& raw_frame);
    
    const std::vector<uint8_t>& get_raw_frame() const;
    const std::array<uint8_t, 6>& get_dst_mac() const;
    const std::array<uint8_t, 6>& get_src_mac() const;
    uint16_t get_ethertype() const;
    const std::vector<uint8_t>& get_payload() const;
    
    // format MAC address as string (aa:bb:cc:dd:ee:ff)
    static std::string mac_to_string(const std::array<uint8_t, 6>& mac);

    // print entire frame info
    std::string to_string() const;

private:
    std::vector<uint8_t> raw_frame_; // raw frame bytes
    std::array<uint8_t, 6> dst_mac_; // destination MAC 6 bytes
    std::array<uint8_t, 6> src_mac_; // source MAC 6 bytes
    uint16_t ethertype_; // EtherType 2 bytes 
    std::vector<uint8_t> payload_; // variable size payload
};
} // namespace vswitch
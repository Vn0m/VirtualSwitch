#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

namespace vswitch {

class EthernetFrame {
public:
    EthernetFrame(const uint8_t* data, size_t len);

    const std::array<uint8_t, 6>& get_dst_mac() const;
    const std::array<uint8_t, 6>& get_src_mac() const;

    static std::string mac_to_string(const std::array<uint8_t, 6>& mac);
    static bool is_multicast(const std::array<uint8_t, 6>& mac);

private:
    std::array<uint8_t, 6> dst_mac_;
    std::array<uint8_t, 6> src_mac_;
};

} // namespace vswitch

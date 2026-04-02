#include "core/ethernet_frame.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace vswitch {

EthernetFrame::EthernetFrame(const uint8_t* data, size_t len) {
    if (len < 14) {
        throw std::invalid_argument("Raw frame too short for an ethernet frame");
    }
    std::copy(data, data + 6, dst_mac_.begin());
    std::copy(data + 6, data + 12, src_mac_.begin());
}

const std::array<uint8_t, 6>& EthernetFrame::get_dst_mac() const {
    return dst_mac_;
}

const std::array<uint8_t, 6>& EthernetFrame::get_src_mac() const {
    return src_mac_;
}

std::string EthernetFrame::mac_to_string(const std::array<uint8_t, 6>& mac) {
    std::ostringstream oss;
    for (size_t i = 0; i < mac.size(); ++i) {
        if (i != 0) oss << ":";
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
    }
    return oss.str();
}

} // namespace vswitch

#include "core/ethernet_frame.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace vswitch {
    EthernetFrame::EthernetFrame(const std::vector<uint8_t>& raw_frame){
        if(raw_frame.size() < 14){
            throw std::invalid_argument("Raw frame too short for an ethernet frame");
        }
        
        std::copy(raw_frame.begin(), raw_frame.begin() + 6, dst_mac_.begin());
        std::copy(raw_frame.begin() + 6, raw_frame.begin() + 12, src_mac_.begin());
        ethertype_ = (static_cast<uint16_t>(raw_frame[12]) << 8) | 
                      static_cast<uint16_t>(raw_frame[13]);
        payload_.assign(raw_frame.begin() + 14, raw_frame.end());
    }

    const std::array<uint8_t, 6>& EthernetFrame::get_dst_mac() const {
        return dst_mac_;
    }
    const std::array<uint8_t, 6>& EthernetFrame::get_src_mac() const {
        return src_mac_;
    }
    uint16_t EthernetFrame::get_ethertype() const {
        return ethertype_;
    }
    const std::vector<uint8_t>& EthernetFrame::get_payload() const {
        return payload_;
    }

    std::string EthernetFrame::mac_to_string(const std::array<uint8_t, 6>& mac) {
        std::ostringstream oss;
        for(size_t i = 0; i < mac.size(); ++i){
            if(i != 0) oss << ":";
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
        }
        return oss.str();
    }

    std::string EthernetFrame::to_string() const {
        std::ostringstream oss;
        oss << "Dst MAC: " << mac_to_string(dst_mac_) << ", "
            << "Src MAC: " << mac_to_string(src_mac_) << ", "
            << "Ethertype: 0x" << std::hex << std::setw(4) << std::setfill('0') << ethertype_ << ", "
            << "Payload Size: " << std::dec << payload_.size() << " bytes";
        return oss.str();
    }
    
} // namespace vswitch
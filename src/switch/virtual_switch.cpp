#include "switch/virtual_switch.hpp"
#include <iostream>
#include <sstream>
#include <poll.h>

namespace vswitch {

    VirtualSwitch::VirtualSwitch() {
    }

    VirtualSwitch::~VirtualSwitch() {
    }

    void VirtualSwitch::add_port(std::unique_ptr<Port> port) {
        ports_.push_back(std::move(port));
    }

    void VirtualSwitch::learn_mac_address(const std::array<uint8_t, 6>& mac, Port* port) {
        std::string mac_str = EthernetFrame::mac_to_string(mac);
        mac_address_table_[mac_str] = MacEntry{port, std::chrono::steady_clock::now()};
    }

    void VirtualSwitch::forward_frame(const EthernetFrame& frame, Port* source_port) {
        std::string dst_mac = EthernetFrame::mac_to_string(frame.get_dst_mac());
        auto it = mac_address_table_.find(dst_mac);
        if (it != mac_address_table_.end()) {
            Port* dest_port = it->second.port;
            if (dest_port != source_port) {
                dest_port->write(frame.get_raw_frame().data(), frame.get_raw_frame().size());
            }
        } else {
            // Destination MAC unknown: flood to all ports except source
            for (const auto& port : ports_) {
                if (port.get() != source_port) {
                    port->write(frame.get_raw_frame().data(), frame.get_raw_frame().size());
                }
            }
        }
    }

    std::string VirtualSwitch::mac_table_to_string() const {
        std::ostringstream oss;
        oss << "MAC Address Table:\n";
        for (const auto& entry : mac_address_table_) {
            oss << entry.first << " -> " << entry.second.port->get_name() << "\n";
        }
        return oss.str();
    }

    void VirtualSwitch::age_out_mac_entries() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = mac_address_table_.begin(); it != mac_address_table_.end();) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.learned_at);
            if (elapsed.count() > FDB_AGING_TIME) {
                it = mac_address_table_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void VirtualSwitch::run() {
        const int MAX_FRAME_SIZE = 1600;
        std::vector<uint8_t> buffer(MAX_FRAME_SIZE);

        std::vector<pollfd> poll_fds;
        for (const auto& port : ports_) {
            pollfd pfd;
            pfd.fd = port->get_fd(); 
            pfd.events = POLLIN;
            pfd.revents = 0;
            poll_fds.push_back(pfd);
        }

        while (true) {
            int ret = poll(poll_fds.data(), poll_fds.size(), -1);
            if (ret < 0) {
                perror("poll");
                break;
            }

            for (size_t i = 0; i < poll_fds.size(); ++i) {
                if (poll_fds[i].revents & POLLIN) {
                    ssize_t len = ports_[i]->read(buffer.data(), buffer.size());
                    if (len > 0) {
                        buffer.resize(len);
                        EthernetFrame frame(buffer);
                        
                        std::cout << "[" << ports_[i]->get_name() << "] Received frame: "
                                  << EthernetFrame::mac_to_string(frame.get_src_mac()) 
                                  << " -> " 
                                  << EthernetFrame::mac_to_string(frame.get_dst_mac())
                                  << " (" << len << " bytes)" << std::endl;
                        
                        learn_mac_address(frame.get_src_mac(), ports_[i].get());
                        age_out_mac_entries();
                        std::cout << "\n" << mac_table_to_string() << std::endl;                        
                        forward_frame(frame, ports_[i].get());
                        buffer.resize(MAX_FRAME_SIZE);
                    }
                }
            }
        }
    }
    
} // namespace vswitch

#include "switch/virtual_switch.hpp"
#include <iostream>
#include <poll.h>

namespace vswitch {

void VirtualSwitch::add_port(std::unique_ptr<Port> port) {
    ports_.push_back(std::move(port));
}

void VirtualSwitch::learn_mac_address(const std::array<uint8_t, 6>& mac, Port* port) {
    std::string mac_str = EthernetFrame::mac_to_string(mac);
    mac_address_table_[mac_str] = MacEntry{port, std::chrono::steady_clock::now()};
}

void VirtualSwitch::forward_frame(const EthernetFrame& frame, const uint8_t* data, size_t len, Port* source_port) {
    if (EthernetFrame::is_multicast(frame.get_dst_mac())) {
        for (const auto& port : ports_){
            if (port.get() != source_port){
                port->write(data, len);
            }
        }
        return;
    }
    
    std::string dst_mac = EthernetFrame::mac_to_string(frame.get_dst_mac());
    auto it = mac_address_table_.find(dst_mac);
    if (it != mac_address_table_.end()) {
        Port* dest_port = it->second.port;
        if (dest_port != source_port) {
            dest_port->write(data, len);
        }
    } else {
        for (const auto& port : ports_) {
            if (port.get() != source_port) {
                port->write(data, len);
            }
        }
    }
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
    const size_t MAX_FRAME_SIZE = 1600;
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
                    EthernetFrame frame(buffer.data(), static_cast<size_t>(len));

                    std::cout << "[" << ports_[i]->get_name() << "] Received frame: "
                              << EthernetFrame::mac_to_string(frame.get_src_mac())
                              << " -> "
                              << EthernetFrame::mac_to_string(frame.get_dst_mac())
                              << " (" << len << " bytes)" << std::endl;

                    learn_mac_address(frame.get_src_mac(), ports_[i].get());

                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_age_check_).count() >= AGE_CHECK_INTERVAL) {
                        age_out_mac_entries();
                        last_age_check_ = now;
                    }

                    forward_frame(frame, buffer.data(), static_cast<size_t>(len), ports_[i].get());
                }
            }
        }
    }
}

} // namespace vswitch

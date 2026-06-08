#include "switch/virtual_switch.hpp"
#include <iostream>
#include <poll.h>
#include <arpa/inet.h>
#include <cstring>

namespace vswitch {

static constexpr uint16_t ETHERTYPE_ARP = 0x0806;
static constexpr uint16_t ARP_REQUEST   = 1;
static constexpr uint16_t ARP_REPLY     = 2;

uint16_t VirtualSwitch::get_ethertype(const uint8_t* data) {
    return (static_cast<uint16_t>(data[12]) << 8) | data[13];
}

std::string VirtualSwitch::ip_to_string(const uint8_t ip[4]) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return buf;
}

void VirtualSwitch::add_port(std::unique_ptr<Port> port) {
    ports_.push_back(std::move(port));
}

void VirtualSwitch::learn_mac_address(const std::array<uint8_t, 6>& mac, Port* port) {
    std::string mac_str = EthernetFrame::mac_to_string(mac);
    mac_address_table_[mac_str] = MacEntry{port, std::chrono::steady_clock::now()};
}

bool VirtualSwitch::handle_arp(const uint8_t* data, size_t len, Port* source_port) {
    if (len < 14 + 28) return false;

    const ArpPacket* arp = reinterpret_cast<const ArpPacket*>(data + 14);

    if (ntohs(arp->htype) != 1 || ntohs(arp->ptype) != 0x0800) return false;
    if (arp->hlen != 6 || arp->plen != 4) return false;

    uint32_t sender_ip, target_ip;
    memcpy(&sender_ip, arp->spa, 4);
    memcpy(&target_ip, arp->tpa, 4);

    std::array<uint8_t, 6> sender_mac;
    memcpy(sender_mac.data(), arp->sha, 6);

    if (sender_ip != 0) {
        bool is_new = ip_to_mac_.find(sender_ip) == ip_to_mac_.end();
        ip_to_mac_[sender_ip] = { sender_mac, std::chrono::steady_clock::now() };
        if (is_new)
            std::cout << "[ARP] Learned " << ip_to_string(arp->spa)
                      << " -> " << EthernetFrame::mac_to_string(sender_mac) << "\n";
    }

    if (ntohs(arp->oper) != ARP_REQUEST || target_ip == sender_ip) return false;

    auto it = ip_to_mac_.find(target_ip);
    if (it == ip_to_mac_.end()) return false;

    const std::array<uint8_t, 6>& target_mac = it->second.mac;

    uint8_t reply[42];

    memcpy(reply + 0, arp->sha, 6);
    memcpy(reply + 6, target_mac.data(), 6);
    reply[12] = 0x08; reply[13] = 0x06;

    ArpPacket* rep = reinterpret_cast<ArpPacket*>(reply + 14);
    rep->htype = htons(1);
    rep->ptype = htons(0x0800);
    rep->hlen  = 6;
    rep->plen  = 4;
    rep->oper  = htons(ARP_REPLY);
    memcpy(rep->sha, target_mac.data(), 6);
    memcpy(rep->spa, arp->tpa, 4);
    memcpy(rep->tha, arp->sha, 6);
    memcpy(rep->tpa, arp->spa, 4);

    source_port->write(reply, sizeof(reply));

    std::cout << "[ARP] Replied: " << ip_to_string(arp->tpa)
              << " is-at " << EthernetFrame::mac_to_string(target_mac)
              << " (on behalf of cached entry, to " << ip_to_string(arp->spa) << ")\n";

    return true;
}

void VirtualSwitch::forward_frame(const EthernetFrame& frame, const uint8_t* data, size_t len, Port* source_port) {
    if (EthernetFrame::is_multicast(frame.get_dst_mac())) {
        for (const auto& port : ports_) {
            if (port.get() != source_port)
                port->write(data, len);
        }
        return;
    }

    std::string dst_mac = EthernetFrame::mac_to_string(frame.get_dst_mac());
    auto it = mac_address_table_.find(dst_mac);
    if (it != mac_address_table_.end()) {
        Port* dest_port = it->second.port;
        if (dest_port != source_port)
            dest_port->write(data, len);
    } else {
        for (const auto& port : ports_) {
            if (port.get() != source_port)
                port->write(data, len);
        }
    }
}

void VirtualSwitch::age_out_mac_entries() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = mac_address_table_.begin(); it != mac_address_table_.end();) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.learned_at).count() > FDB_AGING_TIME)
            it = mac_address_table_.erase(it);
        else
            ++it;
    }
    for (auto it = ip_to_mac_.begin(); it != ip_to_mac_.end();) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.learned_at).count() > FDB_AGING_TIME)
            it = ip_to_mac_.erase(it);
        else
            ++it;
    }
}

void VirtualSwitch::run() {
    const size_t MAX_FRAME_SIZE = 1600;
    std::vector<uint8_t> buffer(MAX_FRAME_SIZE);

    std::vector<pollfd> poll_fds;
    for (const auto& port : ports_) {
        pollfd pfd;
        pfd.fd     = port->get_fd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        poll_fds.push_back(pfd);
    }

    while (true) {
        int ret = poll(poll_fds.data(), poll_fds.size(), 2500);
        if (ret < 0) { perror("poll"); break; }
        if (ret == 0) {
            for (const auto& port : ports_) port->keepalive();
            continue;
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (!(poll_fds[i].revents & POLLIN)) continue;

            ssize_t len = ports_[i]->read(buffer.data(), buffer.size());
            if (len <= 0) continue;

            EthernetFrame frame(buffer.data(), static_cast<size_t>(len));

            std::cout << "[" << ports_[i]->get_name() << "] "
                      << EthernetFrame::mac_to_string(frame.get_src_mac())
                      << " -> "
                      << EthernetFrame::mac_to_string(frame.get_dst_mac())
                      << " (" << len << " bytes)\n";

            learn_mac_address(frame.get_src_mac(), ports_[i].get());

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_age_check_).count() >= AGE_CHECK_INTERVAL) {
                age_out_mac_entries();
                last_age_check_ = now;
            }

            if (get_ethertype(buffer.data()) == ETHERTYPE_ARP &&
                handle_arp(buffer.data(), static_cast<size_t>(len), ports_[i].get())) {
                continue;
            }

            forward_frame(frame, buffer.data(), static_cast<size_t>(len), ports_[i].get());
        }
    }
}

} // namespace vswitch

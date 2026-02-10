#include "network/udp_port.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace vswitch {

    UdpPort::UdpPort(const std::string& name,
                     const std::string& local_ip,
                     uint16_t local_port,
                     const std::string& remote_ip,
                     uint16_t remote_port)
        : fd_(-1), name_(name) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) {
            throw std::runtime_error("Failed to create UDP socket");
        }

        sockaddr_in local_addr;
        std::memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(local_port);
        if (::inet_pton(AF_INET, local_ip.c_str(), &local_addr.sin_addr) != 1) {
            ::close(fd_);
            throw std::runtime_error("Invalid local IP: " + local_ip);
        }

        if (::bind(fd_, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
            ::close(fd_);
            throw std::runtime_error("Failed to bind UDP socket");
        }

        std::memset(&remote_addr_, 0, sizeof(remote_addr_));
        remote_addr_.sin_family = AF_INET;
        remote_addr_.sin_port = htons(remote_port);
        if (::inet_pton(AF_INET, remote_ip.c_str(), &remote_addr_.sin_addr) != 1) {
            ::close(fd_);
            throw std::runtime_error("Invalid remote IP: " + remote_ip);
        }
    }

    UdpPort::~UdpPort() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    ssize_t UdpPort::read(uint8_t* buffer, size_t size) {
        std::vector<uint8_t> packet(size + sizeof(uint32_t));
        sockaddr_in src_addr;
        socklen_t src_len = sizeof(src_addr);

        ssize_t bytes = ::recvfrom(fd_, packet.data(), packet.size(), 0,
                                   reinterpret_cast<sockaddr*>(&src_addr), &src_len);
        if (bytes <= 0) {
            return bytes;
        }

        if (bytes < static_cast<ssize_t>(sizeof(uint32_t))) {
            return 0;
        }

        uint32_t net_len = 0;
        std::memcpy(&net_len, packet.data(), sizeof(uint32_t));
        uint32_t frame_len = ntohl(net_len);

        if (frame_len == 0 || frame_len > size) {
            return 0;
        }

        if (bytes < static_cast<ssize_t>(sizeof(uint32_t) + frame_len)) {
            return 0;
        }

        std::memcpy(buffer, packet.data() + sizeof(uint32_t), frame_len);
        return static_cast<ssize_t>(frame_len);
    }

    ssize_t UdpPort::write(const uint8_t* buffer, size_t size) {
        uint32_t net_len = htonl(static_cast<uint32_t>(size));
        std::vector<uint8_t> packet(sizeof(uint32_t) + size);
        std::memcpy(packet.data(), &net_len, sizeof(uint32_t));
        std::memcpy(packet.data() + sizeof(uint32_t), buffer, size);

        return ::sendto(fd_, packet.data(), packet.size(), 0,
                        reinterpret_cast<const sockaddr*>(&remote_addr_), sizeof(remote_addr_));
    }

} // namespace vswitch

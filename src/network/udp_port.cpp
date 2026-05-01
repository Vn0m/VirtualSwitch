#include "network/udp_port.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace vswitch {

UdpPort::UdpPort(const std::string& name,
                 const std::string& local_ip,
                 uint16_t local_port,
                 const std::string& remote_ip,
                 uint16_t remote_port,
                 std::optional<std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>> key)
    : fd_(-1), name_(name), key_(key) {
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
    uint8_t packet[1604];
    sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);

    ssize_t bytes = ::recvfrom(fd_, packet, sizeof(packet), 0,
                               reinterpret_cast<sockaddr*>(&src_addr), &src_len);
    if (bytes <= 0) {
        return bytes;
    }

    if (src_addr.sin_addr.s_addr != remote_addr_.sin_addr.s_addr ||
        src_addr.sin_port != remote_addr_.sin_port) {
        return 0;
    }

    if (bytes < static_cast<ssize_t>(sizeof(uint32_t))) {
        return 0;
    }

    uint32_t net_len = 0;
    std::memcpy(&net_len, packet, sizeof(uint32_t));
    uint32_t frame_len = ntohl(net_len);

    if (frame_len == 0 || frame_len > size) {
        return 0;
    }

    if (bytes < static_cast<ssize_t>(sizeof(uint32_t) + frame_len)) {
        return 0;
    }

    std::memcpy(buffer, packet + sizeof(uint32_t), frame_len);
    return static_cast<ssize_t>(frame_len);
}

ssize_t UdpPort::write(const uint8_t* buffer, size_t size) {
    if (key_) {
        uint8_t nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
        randombytes_buf(nonce, sizeof(nonce));

        uint8_t ciphertext[1518 + crypto_aead_xchacha20poly1305_ietf_ABYTES];
        unsigned long long ciphertext_len = 0;
        crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext, &ciphertext_len,
            buffer, size,
            nullptr, 0,
            nullptr, nonce, key_->data());

        uint32_t net_len = htonl(static_cast<uint32_t>(sizeof(nonce) + ciphertext_len));

        iovec iov[3];
        iov[0].iov_base = &net_len;
        iov[0].iov_len  = sizeof(net_len);
        iov[1].iov_base = nonce;
        iov[1].iov_len  = sizeof(nonce);
        iov[2].iov_base = ciphertext;
        iov[2].iov_len  = ciphertext_len;

        msghdr msg{};
        msg.msg_name    = const_cast<sockaddr_in*>(&remote_addr_);
        msg.msg_namelen = sizeof(remote_addr_);
        msg.msg_iov     = iov;
        msg.msg_iovlen  = 3;

        return ::sendmsg(fd_, &msg, 0);
    }

    uint32_t net_len = htonl(static_cast<uint32_t>(size));
    iovec iov[2];
    iov[0].iov_base = &net_len;
    iov[0].iov_len  = sizeof(net_len);
    iov[1].iov_base = const_cast<uint8_t*>(buffer);
    iov[1].iov_len  = size;

    msghdr msg{};
    msg.msg_name    = const_cast<sockaddr_in*>(&remote_addr_);
    msg.msg_namelen = sizeof(remote_addr_);
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 2;

    return ::sendmsg(fd_, &msg, 0);
}

void UdpPort::punch(int attempts) {
    const uint8_t punch_packet[4] = {0, 0, 0, 0};
    for (int i = 0; i < attempts; ++i) {
        ::sendto(fd_, punch_packet, sizeof(punch_packet), 0,
                 reinterpret_cast<const sockaddr*>(&remote_addr_), sizeof(remote_addr_));
        usleep(50000);
    }
}

} // namespace vswitch

#include "network/stun_client.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <random>
#include <stdexcept>

namespace vswitch {

// STUN messages, attributes and magic cookie (RFC 8489).
static constexpr uint16_t STUN_BINDING_REQUEST  = 0x0001;
static constexpr uint16_t STUN_BINDING_RESPONSE = 0x0101;

static constexpr uint16_t STUN_ATTR_XOR_MAPPED_ADDRESS = 0x0020;
static constexpr uint16_t STUN_ATTR_MAPPED_ADDRESS     = 0x0001;

static constexpr uint32_t STUN_MAGIC_COOKIE = 0x2112A442;

// STUN message header:
//   2 bytes message type 
//   2 bytes message length
//   4 bytes magic cookie
//  12 bytes transaction ID
static constexpr size_t STUN_HEADER_SIZE = 20;
static constexpr size_t STUN_TXID_OFFSET = 8;
static constexpr size_t STUN_TXID_SIZE   = 12;

StunClient::StunClient(const std::string& stun_host, uint16_t stun_port, uint16_t local_port)
    : stun_host_(stun_host), stun_port_(stun_port), local_port_(local_port) {}

StunClient::~StunClient() {}

StunAddress StunClient::get_public_address() {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo* res = nullptr;
    std::string port_str = std::to_string(stun_port_);
    if (::getaddrinfo(stun_host_.c_str(), port_str.c_str(), &hints, &res) != 0 || res == nullptr) {
        throw std::runtime_error("Failed to resolve STUN server: " + stun_host_);
    }

    sockaddr_in stun_addr{};
    std::memcpy(&stun_addr, res->ai_addr, sizeof(stun_addr));
    ::freeaddrinfo(res);

    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Failed to create STUN socket");
    }

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(local_port_);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
        ::close(fd);
        throw std::runtime_error("Failed to bind STUN socket on port " + std::to_string(local_port_));
    }

    std::array<uint8_t, STUN_HEADER_SIZE> request{};
    request[0] = (STUN_BINDING_REQUEST >> 8) & 0xFF;
    request[1] =  STUN_BINDING_REQUEST       & 0xFF;
    request[2] = 0x00;
    request[3] = 0x00;
    uint32_t cookie_be = htonl(STUN_MAGIC_COOKIE);
    std::memcpy(&request[4], &cookie_be, sizeof(cookie_be));
    
    std::array<uint8_t, STUN_TXID_SIZE> txid{};
    {
        std::random_device rd;
        for (size_t i = 0; i < STUN_TXID_SIZE; ++i) {
            txid[i] = static_cast<uint8_t>(rd());
            request[STUN_TXID_OFFSET + i] = txid[i];
        }
    }

    ssize_t sent = ::sendto(fd, request.data(), request.size(), 0,
                            reinterpret_cast<sockaddr*>(&stun_addr), sizeof(stun_addr));
    if (sent < 0) {
        ::close(fd);
        throw std::runtime_error("Failed to send STUN request");
    }

    timeval tv{};
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::array<uint8_t, 512> response{};
    ssize_t received = ::recv(fd, response.data(), response.size(), 0);
    ::close(fd);

    if (received < static_cast<ssize_t>(STUN_HEADER_SIZE)) {
        throw std::runtime_error("STUN response too short or timed out");
    }

    uint16_t msg_type = (static_cast<uint16_t>(response[0]) << 8) | response[1];
    if (msg_type != STUN_BINDING_RESPONSE) {
        throw std::runtime_error("Unexpected STUN message type");
    }

    uint32_t magic_cookie = (static_cast<uint32_t>(response[4]) << 24) |
                            (static_cast<uint32_t>(response[5]) << 16) |
                            (static_cast<uint32_t>(response[6]) << 8)  |
                            static_cast<uint32_t>(response[7]);
    if (magic_cookie != STUN_MAGIC_COOKIE) {
        throw std::runtime_error("Invalid STUN magic cookie");
    }

    if (!std::equal(txid.begin(), txid.end(), response.begin() + STUN_TXID_OFFSET)) {
        throw std::runtime_error("STUN transaction ID mismatch");
    }

    uint16_t msg_len = (static_cast<uint16_t>(response[2]) << 8) | response[3];

    size_t response_size = static_cast<size_t>(received);
    size_t attrs_end = std::min(static_cast<size_t>(STUN_HEADER_SIZE + msg_len), response_size);

    size_t offset = STUN_HEADER_SIZE;
    while (offset + 4 <= attrs_end) {
        uint16_t attr_type   = (static_cast<uint16_t>(response[offset])     << 8) | response[offset + 1];
        uint16_t attr_length = (static_cast<uint16_t>(response[offset + 2]) << 8) | response[offset + 3];
        offset += 4;

        if (offset + attr_length > response_size) break;

        if (attr_type == STUN_ATTR_XOR_MAPPED_ADDRESS || attr_type == STUN_ATTR_MAPPED_ADDRESS) {
            if (attr_length < 8) break;

            uint8_t family = response[offset + 1];
            if (family != 0x01) {
                offset += (attr_length + 3) & ~3;
                continue;
            }

            uint16_t raw_port;
            uint32_t raw_ip;
            std::memcpy(&raw_port, &response[offset + 2], sizeof(raw_port));
            std::memcpy(&raw_ip,   &response[offset + 4], sizeof(raw_ip));

            uint32_t host_ip;
            StunAddress result;
            if (attr_type == STUN_ATTR_XOR_MAPPED_ADDRESS) {
                result.port = ntohs(raw_port) ^ (STUN_MAGIC_COOKIE >> 16);
                host_ip = raw_ip ^ htonl(STUN_MAGIC_COOKIE);
            } else {
                result.port = ntohs(raw_port);
                host_ip = raw_ip;
            }

            char ip_buf[INET_ADDRSTRLEN]{};
            if (::inet_ntop(AF_INET, &host_ip, ip_buf, sizeof(ip_buf)) == nullptr) {
                throw std::runtime_error("Failed to convert IP address");
            }
            result.ip = ip_buf;
            return result;
        }

        offset += (attr_length + 3) & ~3;
    }

    throw std::runtime_error("No mapped address in STUN response");
}

} // namespace vswitch

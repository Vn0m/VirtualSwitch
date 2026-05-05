#pragma once

#include <string>
#include <cstdint>

namespace vswitch {

struct StunAddress {
    std::string ip;
    uint16_t port;
};

class StunClient {
public:
    StunClient(const std::string& stun_host, uint16_t stun_port, int fd);
    ~StunClient() = default;

    StunClient(const StunClient&) = delete;
    StunClient& operator=(const StunClient&) = delete;

    StunAddress get_public_address();

    static uint16_t probe_stable_port(const std::string& stun_host, uint16_t stun_port, uint16_t start_port = 5000, size_t max_attempts = 20);

private:
    std::string stun_host_;
    uint16_t stun_port_;
    int fd_;
};

} // namespace vswitch

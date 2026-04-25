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

private:
    std::string stun_host_;
    uint16_t stun_port_;
    int fd_;
};

} // namespace vswitch

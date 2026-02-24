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
    StunClient(const std::string& stun_host, uint16_t stun_port, uint16_t local_port);
    ~StunClient();

    StunClient(const StunClient&) = delete;
    StunClient& operator=(const StunClient&) = delete;

    StunAddress get_public_address();

private:
    std::string stun_host_;
    uint16_t stun_port_;
    uint16_t local_port_;
};

} // namespace vswitch

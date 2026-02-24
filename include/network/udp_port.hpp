#pragma once

#include <string>
#include <cstdint>
#include <netinet/in.h>
#include "network/port.hpp"

namespace vswitch {

class UdpPort : public Port {
public:
    UdpPort(const std::string& name,
            const std::string& local_ip,
            uint16_t local_port,
            const std::string& remote_ip,
            uint16_t remote_port);

    ~UdpPort();

    UdpPort(const UdpPort&) = delete;
    UdpPort& operator=(const UdpPort&) = delete;

    const std::string& get_name() const override { return name_; }
    int get_fd() const override { return fd_; }
    ssize_t read(uint8_t* buffer, size_t size) override;
    ssize_t write(const uint8_t* buffer, size_t size) override;
    bool is_local() const override { return false; }

    void punch(int attempts = 10);

private:
    int fd_;
    std::string name_;
    struct sockaddr_in remote_addr_;
};

} // namespace vswitch

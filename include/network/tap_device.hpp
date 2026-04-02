#pragma once

#include <string>
#include <cstdint>
#include "network/port.hpp"

namespace vswitch {

class TapDevice : public Port {
public:
    explicit TapDevice(const std::string& device_name);
    ~TapDevice();

    TapDevice(const TapDevice&) = delete;
    TapDevice& operator=(const TapDevice&) = delete;

    const std::string& get_name() const override { return name_; }
    int get_fd() const override { return fd_; }

    ssize_t read(uint8_t* buffer, size_t size) override;
    ssize_t write(const uint8_t* buffer, size_t size) override;

private:
    int fd_;
    std::string name_;
};

} // namespace vswitch

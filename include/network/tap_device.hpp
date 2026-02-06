#pragma once

#include <string>
#include <cstdint>

namespace vswitch {
class TapDevice {
public:
    explicit TapDevice(const std::string& device_name);
    ~TapDevice();

    TapDevice(const TapDevice&) = delete;
    TapDevice& operator = (const TapDevice&) = delete;

    const std::string& get_name() const { return name_; }
    int get_fd() const { return fd_; }
    
    ssize_t read(uint8_t* buffer, size_t size);
    ssize_t write(const uint8_t* buffer, size_t size);

private:
    int fd_;
    std::string name_;
};

} // namespace vswitch


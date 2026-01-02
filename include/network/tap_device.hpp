#pragma once

#include <string>
#include <cstdint> 
#include <vector>

namespace vswitch {
class TapDevice {
public:
    // opens and configures the TAP device and we pass
    // the name of what the interface will be called 
    explicit TapDevice(const std::string& device_name);

    // deconstructor, close device
    ~TapDevice();

    TapDevice(const TapDevice&) = delete;
    TapDevice& operator = (const TapDevice&) = delete;
    
    // get an ethernet frame from the kernel and we return raw 
    // frame bytes 
    std::vector<uint8_t> read_frame();

    // send an ethernet frame to the kernel
    void write_frame(const std::vector<uint8_t>& frame);

    const std::string& get_name() const { return name_; }

private:
    int fd_; // file descriptor for tap device
    std::string name_; // device name
};

} // namespace vswitch


#include "network/tap_device.hpp"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>

namespace vswitch {
    TapDevice::TapDevice(const std::string& device_name){
        
        std::string path = "/dev/net/tun";
        fd_ = open(path.c_str(), O_RDWR);

        if(fd_ < 0){
            throw std::runtime_error("Failed to open " + path + ": " +
                                 std::string(strerror(errno)));
        }
        
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));

        // IFF_TAP: create TAP device
        // IFF_NO_PI: frames have no packet info header
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI; 

        if(!device_name.empty()){
            strncpy(ifr.ifr_name, device_name.c_str(), IFNAMSIZ);
        }
        
        if(ioctl(fd_, TUNSETIFF, &ifr) < 0){
            close(fd_);
            throw std::runtime_error("Failed to configure TAP device: " + 
                                 std::string(strerror(errno)));
        }

        name_ = ifr.ifr_name;

    }

    TapDevice::~TapDevice() {
        if(fd_ >= 0){
            close(fd_);
        }
    }

    ssize_t TapDevice::read(uint8_t* buffer, size_t size) {
        ssize_t bytes_read = ::read(fd_, buffer, size);
        return bytes_read;
    }

    ssize_t TapDevice::write(const uint8_t* buffer, size_t size) {
        ssize_t bytes_written = ::write(fd_, buffer, size);
        return bytes_written;
    }
    
} // namespace vswitch
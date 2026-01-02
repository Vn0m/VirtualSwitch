#include "network/tap_device.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <string>


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

    std::vector<uint8_t> TapDevice::read_frame(){
        std::vector<uint8_t> buffer(1518);

        ssize_t bytes_read = read(fd_, buffer.data(), buffer.size());

        if(bytes_read < 0){
            throw std::runtime_error("Failed to read from TAP: " + 
                                 std::string(strerror(errno)));
        }

        buffer.resize(bytes_read);

        return buffer;
    }

    void TapDevice::write_frame(const std::vector<uint8_t>& frame){
        ssize_t bytes_written = write(fd_, frame.data(), frame.size());

        if (bytes_written < 0){
            throw std::runtime_error("Failed to write to TAP: " + 
                                 std::string(strerror(errno)));
        }

        if(static_cast<size_t>(bytes_written) != frame.size()){
            throw std::runtime_error("Partial write to TAP device");
        }
    }
    
} // namespace vswitch
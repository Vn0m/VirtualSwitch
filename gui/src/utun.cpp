#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>

class UtunDevice {
public:
    UtunDevice() {
        fd_ = ::socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
        }

        struct ctl_info info;
        std::memset(&info, 0, sizeof(info));
        std::strncpy(info.ctl_name, UTUN_CONTROL_NAME, sizeof(info.ctl_name));
        if (::ioctl(fd_, CTLIOCGINFO, &info) < 0) {
            throw std::runtime_error(std::string("ioctl: ") + std::strerror(errno));
        }

        struct sockaddr_ctl addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sc_len = sizeof(addr);
        addr.sc_family = AF_SYSTEM;
        addr.ss_sysaddr = AF_SYS_CONTROL;
        addr.sc_id = info.ctl_id;
        addr.sc_unit = 0;
        if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(fd_);
            throw std::runtime_error(std::string("connect: ") + std::strerror(errno));
        }

        char ifname[32];
        socklen_t len = sizeof(ifname);
        if (::getsockopt(fd_, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &len) < 0) {
            ::close(fd_);
            throw std::runtime_error(std::string("getsockopt: ") + std::strerror(errno));
        }
        name_ = ifname;
    }

    ~UtunDevice() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    UtunDevice(const UtunDevice&) = delete;
    UtunDevice& operator=(const UtunDevice&) = delete;
    
    const std::string& name() const { return name_; }
    int fd() const { return fd_; }

private:
    int fd_;
    std::string name_;
};

int main() {
    try {
        UtunDevice tun;
        std::cout << "opened tunnel: " << tun.name() << "  (fd=" << tun.fd() << ")\n";

        for (;;) ::pause();
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

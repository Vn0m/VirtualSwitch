#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>
#include <arpa/inet.h>

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

uint16_t internet_checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (data[i] << 8) | data[i + 1];
    }
    if (len & 1) {
        sum += data[len - 1] << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

int main() {
    try {
        UtunDevice tun;
        std::cout << "opened tunnel: " << tun.name() << "  (fd=" << tun.fd() << ")\n";

        uint8_t buff[2048];
        for(;;) {
            ssize_t n = ::read(tun.fd(), buff, sizeof(buff));
            if (n < 0) {
                perror("read: ");
                break;
            }
            if (n < 4) {
                continue;
            }

            uint32_t family_be;
            std::memcpy(&family_be, buff, 4);
            uint32_t family = ntohl(family_be);

            const uint8_t* pkt = buff + 4;
            ssize_t pkt_len = n - 4;
            std::cout << "read " << n << " bytes " << "(family=" << family << ", packet=" << pkt_len << ")";

            if (family == AF_INET && pkt_len >= 20) {
                uint8_t version = pkt[0] >> 4;
                uint8_t protocol = pkt[9];
                const uint8_t* src = pkt + 12;
                const uint8_t* dst = pkt + 16;

                char s[16], d[16];
                std::snprintf(s, sizeof(s), "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
                std::snprintf(d, sizeof(d), "%u.%u.%u.%u", dst[0], dst[1], dst[2], dst[3]);

                std::cout << "  IPv" << int(version) << " " << s << " -> " << d << " proto=" << int(protocol);
            }
            std::cout << "\n";
        }
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

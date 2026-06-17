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
            ::close(fd_);
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

            uint8_t* pkt = buff + 4;
            ssize_t pkt_len = n - 4;
            std::cout << "read " << n << " bytes " << "(family=" << family << ", packet=" << pkt_len << ")";

            if (family == AF_INET && pkt_len >= 20) {
                //uint8_t version = pkt[0] >> 4;
                size_t ihl = (pkt[0] & 0x0F) * 4;
                uint8_t protocol = pkt[9];
                //const uint8_t* src = pkt + 12;
                //const uint8_t* dst = pkt + 16;
                uint8_t* icmp = pkt + ihl;

                if (protocol == 1 && pkt_len >= (ssize_t)ihl + 8 && icmp[0] == 8) {
                    uint8_t temp[4];
                    std::memcpy(temp, pkt + 12, 4);
                    std::memcpy(pkt + 12, pkt + 16, 4);
                    std::memcpy(pkt + 16, temp, 4);

                    icmp[0] = 0;

                    size_t icmp_len = pkt_len - ihl;
                    icmp[2] = 0; 
                    icmp[3] = 0;
                    uint16_t c = internet_checksum(icmp, icmp_len);
                    icmp[2] = c >> 8;
                    icmp[3] = c & 0xFF;

                    if (::write(tun.fd(), buff, n) < 0) { 
                        perror("write"); 
                    } else {
                        std::cout << "  -> replied\n";
                    }
                    continue;
                }
            }
        }
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <sys/ioctl.h>
#include <net/if_utun.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netinet/in.h>

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <virtual_ip> [docker_port]\n";
        return 1;
    }
    std::string vip = argv[1];
    uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::atoi(argv[2])) : 1194;
    try {
        UtunDevice tun;
        std::cout << "opened tunnel: " << tun.name() << "\n";

        std::string up = "ifconfig " + tun.name() + " " + vip + " 10.255.0.1 up";
        std::string rt = "route -n add -net 10.0.0.0/24 10.255.0.1";
        if (std::system(up.c_str()) != 0) {
            std::cerr << "failed to configure " << tun.name() << "\n";
            return 1;
        }
        std::system(rt.c_str());

        int udp = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (udp < 0) {
            perror("socket");
            return 1;
        }

        sockaddr_in peer{};
        peer.sin_family = AF_INET;
        peer.sin_port = htons(port);
        ::inet_pton(AF_INET, "127.0.0.1", &peer.sin_addr);

        struct pollfd fds[2];
        fds[0].fd = tun.fd();
        fds[0].events = POLLIN;
        fds[1].fd = udp;
        fds[1].events = POLLIN;

        uint8_t buff[2048];
        for (;;) {
            if (::poll(fds, 2, -1) < 0) {
                perror("poll");
                break;
            }

            if (fds[0].revents & POLLIN) {
                ssize_t n = ::read(tun.fd(), buff, sizeof(buff));
                if (n > 4) {
                    ::sendto(udp, buff + 4, n - 4, 0,
                             reinterpret_cast<sockaddr*>(&peer), sizeof(peer));
                }
            }

            if (fds[1].revents & POLLIN) {
                uint8_t out[2048];
                uint32_t fam = htonl(AF_INET);
                std::memcpy(out, &fam, 4);
                ssize_t n = ::recvfrom(udp, out + 4, sizeof(out) - 4, 0, nullptr, nullptr);
                if (n > 0) {
                    ::write(tun.fd(), out, n + 4);
                }
            }
        }

        ::close(udp);
    } catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

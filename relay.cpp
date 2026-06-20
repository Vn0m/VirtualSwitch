#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

static int open_tun(const std::string& name) {
    int fd = ::open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        throw std::runtime_error(std::string("open /dev/net/tun: ") + std::strerror(errno));
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    if (::ioctl(fd, TUNSETIFF, &ifr) < 0) {
        ::close(fd);
        throw std::runtime_error(std::string("TUNSETIFF: ") + std::strerror(errno));
    }
    return fd;
}

int main(int argc, char* argv[]) {
    uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::atoi(argv[1])) : 9000;
    try {
        int tun = open_tun("tun0");

        int udp = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (udp < 0) {
            throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
        }

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(port);
        if (::bind(udp, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
            throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
        }

        sockaddr_in mac{};
        socklen_t mac_len = sizeof(mac);
        bool have_mac = false;

        struct pollfd fds[2];
        fds[0].fd = tun;
        fds[0].events = POLLIN;
        fds[1].fd = udp;
        fds[1].events = POLLIN;

        std::cout << "relay up: tun0 <-> udp/" << port << "\n";

        uint8_t buff[2048];
        for (;;) {
            if (::poll(fds, 2, -1) < 0) {
                perror("poll");
                break;
            }

            if (fds[0].revents & POLLIN) {
                ssize_t n = ::read(tun, buff, sizeof(buff));
                if (n > 0 && have_mac) {
                    ::sendto(udp, buff, n, 0, reinterpret_cast<sockaddr*>(&mac), mac_len);
                }
            }

            if (fds[1].revents & POLLIN) {
                ssize_t n = ::recvfrom(udp, buff, sizeof(buff), 0,
                                       reinterpret_cast<sockaddr*>(&mac), &mac_len);
                if (n > 0) {
                    have_mac = true;
                    ::write(tun, buff, n);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

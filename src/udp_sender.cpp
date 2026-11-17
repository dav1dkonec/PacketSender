#include "udp_sender.h"

#include <arpa/inet.h>
#include <net/if.h>  // if_nametoindex
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

UdpSender::UdpSender(const std::string& dest, uint16_t port, const std::string& interface,
                     bool connected)
    : connected_(connected), dest_(dest + ':' + std::to_string(port))
{
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (::inet_pton(AF_INET, dest.c_str(), &addr_.sin_addr) != 1) {
        throw std::system_error(EINVAL, std::generic_category(), "address " + dest);
    }

    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(), "socket");
    }

    try {
        if (!interface.empty()) {
            bind_interface(interface);
        }

        // no packet is sent: only stores the peer and enables ICMP error reporting
        if (connected_ &&
            ::connect(fd_, reinterpret_cast<const sockaddr*>(&addr_), sizeof addr_) < 0) {
            fail(errno);
        }
    } catch (...) {
        ::close(fd_);  // our own constructor failed: the destructor will not run
        throw;
    }
}

void UdpSender::bind_interface(const std::string& interface)
{
    const unsigned index = ::if_nametoindex(interface.c_str());
    if (index == 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(), "interface '" + interface + "'");
    }
    // tested by feature, not by OS name
#if defined(IP_BOUND_IF)  // macOS, BSD: interface index
    if (::setsockopt(fd_, IPPROTO_IP, IP_BOUND_IF, &index, sizeof index) < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(), "setsockopt IP_BOUND_IF");
    }
#elif defined(SO_BINDTODEVICE)  // Linux: interface name
    if (::setsockopt(fd_, SOL_SOCKET, SO_BINDTODEVICE, interface.c_str(),
                     static_cast<socklen_t>(interface.size())) < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(),
                                err == EPERM ? "setsockopt SO_BINDTODEVICE (needs CAP_NET_RAW)"
                                             : "setsockopt SO_BINDTODEVICE");
    }
#else
    throw std::system_error(ENOTSUP, std::generic_category(), "--interface not supported here");
#endif
}

UdpSender::~UdpSender()
{
    ::close(fd_);
}

bool UdpSender::send(const void* data, std::size_t len)
{
    for (;;) {
        const ssize_t n = connected_
            ? ::send(fd_, data, len, 0)
            : ::sendto(fd_, data, len, 0, reinterpret_cast<const sockaddr*>(&addr_), sizeof addr_);
        if (n >= 0) {
            return true;
        }
        const int err = errno;
        if (err == EINTR) {
            continue;
        }
        if (err == ENOBUFS) {
            return false;
        }
        fail(err);
    }
}

void UdpSender::fail(int err) const
{
    switch (err) {
    case ECONNREFUSED:
        // reported asynchronously: the ICMP answer to an earlier packet
        throw std::system_error(err, std::generic_category(),
                                "receiver not listening on " + dest_ +
                                " (packet mode receiver: use --no-listener)");
    case EHOSTUNREACH:
    case EHOSTDOWN:  // macOS: no ARP reply
        throw std::system_error(err, std::generic_category(), "receiver " + dest_ + " unreachable");
    case ENETUNREACH:
        throw std::system_error(err, std::generic_category(), "no route to " + dest_);
    default:
        throw std::system_error(err, std::generic_category(), "send to " + dest_);
    }
}

#ifndef UDP_SENDER_UDP_SENDER_H
#define UDP_SENDER_UDP_SENDER_H

#include <netinet/in.h>

#include <cstddef>
#include <cstdint>
#include <string>

/// UDP socket towards one receiver.
/// Connected (default): the kernel reports ICMP errors of the receiver on the
/// next send(), so a receiver that is not listening stops the run.
/// Unconnected (@p connected = false, packet mode): sendto(), ICMP port
/// unreachable is ignored because the kernel cannot match it to the socket.
class UdpSender {
public:
    /// @p interface empty = routing decides, otherwise IP_BOUND_IF / SO_BINDTODEVICE.
    /// @throws std::system_error
    UdpSender(const std::string& dest, uint16_t port, const std::string& interface,
              bool connected);
    ~UdpSender();

    UdpSender(const UdpSender&) = delete;
    UdpSender& operator=(const UdpSender&) = delete;

    /// @return false if the interface queue was full (ENOBUFS): packet dropped
    /// @throws std::system_error receiver unreachable or another send error
    bool send(const void* data, std::size_t len);

private:
    void bind_interface(const std::string& interface);
    [[noreturn]] void fail(int err) const;

    int fd_ = -1;
    sockaddr_in addr_{};
    bool connected_;
    std::string dest_;  // "ip:port" for messages
};

#endif // UDP_SENDER_UDP_SENDER_H

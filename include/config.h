#ifndef UDP_SENDER_CONFIG_H
#define UDP_SENDER_CONFIG_H

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Sender configuration
// ---------------------------------------------------------------------------

struct Config {
    // network
    std::string dest = "192.168.50.2";  // receiver IPv4 address (direct cable)
    uint16_t port = 5000;           // receiver UDP port
    std::string interface;          // empty = routing decides
    bool no_listener = false;       // unconnected socket: ignore ICMP port unreachable

    // schedule
    uint32_t count = 10'000;        // packets including warm-up
    uint32_t interval_us = 1000;    // nominal gap between packets
    uint32_t jitter_us = 0;         // uniform random offset in [-jitter, +jitter]
    uint32_t warmup = 20;           // first seqs, discarded by the evaluation

    bool realtime = false;          // real-time scheduling of the send thread
};

/**
 * @brief Parses command-line arguments into a Config.
 *
 * Supported options:
 *   -d, --dest <ipv4>        receiver address
 *   -p, --port <n>           UDP port (1-65535)
 *   -I, --interface <name>   send through this interface, e.g. en7
 *   -L, --no-listener        receiver has no UDP socket (packet mode)
 *   -n, --count <n>          packets to send, warm-up included
 *   -i, --interval-us <n>    gap between packets in us
 *   -j, --jitter-us <n>      random offset of each send time, < interval / 2
 *   -w, --warmup <n>         warm-up packets, <= count
 *   -R, --realtime           real-time scheduling of the send thread
 *   -h, --help               print usage and exit
 *
 * @throws std::invalid_argument  unknown option, missing or invalid value
 * @note `--help` prints usage and exits with code 0.
 */
Config parse_args(int argc, char* argv[]);

/// Prints the options with their default values to stdout.
void print_usage(const char* prog);

#endif // UDP_SENDER_CONFIG_H

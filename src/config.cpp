#include "config.h"

#include <arpa/inet.h>  // inet_pton
#include <getopt.h>
#include <netinet/in.h>

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

// Parses the whole string as a number in [min, max].
template <typename T>
T parse_number(std::string_view text, std::string_view option, T min, T max)
{
    T value{};
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, value);

    if (ec != std::errc{} || ptr != end || value < min || value > max) {
        throw std::invalid_argument(
            std::string(option) + ": invalid value '" + std::string(text) +
            "' (expected " + std::to_string(min) + "-" + std::to_string(max) + ")");
    }
    return value;
}

std::string non_empty(const char* text, std::string_view option)
{
    if (*text == '\0') {
        throw std::invalid_argument(std::string(option) + ": empty value");
    }
    return text;
}

std::string ipv4(const char* text)
{
    in_addr addr{};
    if (::inet_pton(AF_INET, text, &addr) != 1) {
        throw std::invalid_argument("--dest: invalid IPv4 address '" + std::string(text) + "'");
    }
    return text;
}

} // namespace

void print_usage(const char* prog)
{
    const Config def;
    std::cout
        << "Usage: " << prog << " [options]\n"
        << "\nNetwork:\n"
        << "  -d, --dest <ipv4>       receiver address (default " << def.dest << ")\n"
        << "  -p, --port <n>          UDP port (default " << def.port << ")\n"
        << "  -I, --interface <name>  send through this interface (default: routing)\n"
        << "  -L, --no-listener       receiver in packet mode: ignore port unreachable\n"
        << "  -s, --size <bytes>      UDP payload 16-1472, rest is padding (default " << def.size << ")\n"
        << "\nSchedule:\n"
        << "  -n, --count <n>         packets, warm-up included (default " << def.count << ")\n"
        << "  -i, --interval-us <n>   gap between packets in us (default " << def.interval_us << ")\n"
        << "  -j, --jitter-us <n>     random offset +-n us, < interval/2 (default " << def.jitter_us << ")\n"
        << "  -w, --warmup <n>        warm-up packets (default " << def.warmup << ")\n"
        << "  -R, --realtime          real-time scheduling of the send thread\n"
        << "\n  -h, --help              print this help and exit\n";
}

Config parse_args(int argc, char* argv[])
{
    Config cfg;

    static const option long_options[] = {
        {"dest",        required_argument, nullptr, 'd'},
        {"port",        required_argument, nullptr, 'p'},
        {"interface",   required_argument, nullptr, 'I'},
        {"no-listener", no_argument,       nullptr, 'L'},
        {"size",        required_argument, nullptr, 's'},
        {"count",       required_argument, nullptr, 'n'},
        {"interval-us", required_argument, nullptr, 'i'},
        {"jitter-us",   required_argument, nullptr, 'j'},
        {"warmup",      required_argument, nullptr, 'w'},
        {"realtime",    no_argument,       nullptr, 'R'},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr,       0,                 nullptr, 0},
    };
    const char* short_options = ":d:p:I:Ls:n:i:j:w:Rh";

    // reset getopt's global state so parse_args can run repeatedly (tests)
#ifdef __GLIBC__
    optind = 0;
#else
    optreset = 1;
    optind = 1;
#endif
    opterr = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (opt) {
        case 'd':
            cfg.dest = ipv4(optarg);
            break;
        case 'p':
            cfg.port = parse_number<uint16_t>(optarg, "--port", 1, 65535);
            break;
        case 'I':
            cfg.interface = non_empty(optarg, "--interface");
            break;
        case 'L':
            cfg.no_listener = true;
            break;
        case 's':
            // 16 = test_packet header, 1472 = 1500 MTU - IPv4 20 - UDP 8
            cfg.size = parse_number<uint32_t>(optarg, "--size", 16, 1472);
            break;
        case 'n':
            cfg.count = parse_number<uint32_t>(optarg, "--count", 1, 100'000'000);
            break;
        case 'i':
            cfg.interval_us = parse_number<uint32_t>(optarg, "--interval-us", 1, 10'000'000);
            break;
        case 'j':
            cfg.jitter_us = parse_number<uint32_t>(optarg, "--jitter-us", 0, 10'000'000);
            break;
        case 'w':
            cfg.warmup = parse_number<uint32_t>(optarg, "--warmup", 0, 100'000'000);
            break;
        case 'R':
            cfg.realtime = true;
            break;
        case 'h':
            print_usage(argv[0]);
            std::exit(EXIT_SUCCESS);
        case ':':
            throw std::invalid_argument(std::string(argv[optind - 1]) + ": missing value");
        default: {  // '?'
            const std::string_view arg = argv[optind - 1];
            if (arg.starts_with("--")) {
                throw std::invalid_argument("unknown option: " + std::string(arg));
            }
            throw std::invalid_argument(std::string("unknown option: -") +
                                        static_cast<char>(optopt));
        }
        }
    }

    if (optind < argc) {
        throw std::invalid_argument(std::string("unexpected argument: ") + argv[optind]);
    }

    // cross-option checks: the order of the options must not matter
    if (uint64_t{cfg.jitter_us} * 2 >= cfg.interval_us) {
        // two neighbours shift by up to 2 * jitter: send times keep increasing
        throw std::invalid_argument("--jitter-us: must be less than half of --interval-us (" +
                                    std::to_string(cfg.interval_us) + ")");
    }
    if (cfg.warmup > cfg.count) {
        throw std::invalid_argument("--warmup: must not exceed --count (" +
                                    std::to_string(cfg.count) + ")");
    }
    return cfg;
}

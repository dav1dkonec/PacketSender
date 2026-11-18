// udp_sender: paced generator of LAT1 test packets for latency_rx

#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>

#include "config.h"
#include "lateness.h"
#include "pacer.h"
#include "packet.h"
#include "stop_signal.h"
#include "udp_sender.h"

namespace {

uint64_t realtime_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

} // namespace

int main(int argc, char* argv[])
{
    Config cfg;
    try {
        cfg = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n\n";
        print_usage(argv[0]);
        return 2;
    }

    std::cout << "dest " << cfg.dest << ':' << cfg.port
              << (cfg.interface.empty() ? "" : " via " + cfg.interface)
              << (cfg.no_listener ? ", no listener" : "") << ", payload " << cfg.size << " B\n"
              << cfg.count << " packets (" << cfg.warmup << " warm-up), interval "
              << cfg.interval_us << " us, jitter +-" << cfg.jitter_us << " us\n";

    uint32_t sent = 0;
    uint32_t dropped = 0;  // ENOBUFS
    Lateness lateness(cfg.count);
    int status = 0;
    const uint64_t t_begin = realtime_ns();

    try {
        UdpSender sender(cfg.dest, cfg.port, cfg.interface, !cfg.no_listener);
        Pacer pacer(cfg.interval_us, cfg.jitter_us);
        if (cfg.realtime) {
            pacer.make_realtime();
            std::cout << "RT: on\n";
        }
        install_stop_handler();
        std::cout << "sending (Ctrl+C to stop)" << std::endl;

        // header + zero padding, allocated once before the loop
        std::vector<unsigned char> buf(cfg.size, 0);
        test_packet pkt{};
        pkt.magic = tp_hton32(TEST_PACKET_MAGIC);

        pacer.start();
        for (uint32_t seq = 0; seq < cfg.count; ++seq) {
            const int64_t late = pacer.wait_next();
            if (late < 0) {
                break;  // g_stop
            }
            pkt.seq = tp_hton32(seq);
            pkt.send_ns = tp_hton64(realtime_ns());
            std::memcpy(buf.data(), &pkt, sizeof pkt);
            if (sender.send(buf.data(), buf.size())) {
                ++sent;
            } else {
                ++dropped;
            }
            lateness.add(late);
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        status = 1;
    }

    const double secs = static_cast<double>(realtime_ns() - t_begin) / 1e9;
    std::cout << (g_stop ? "stopped\n" : "") << "sent " << sent << " (warm-up "
              << std::min(sent, cfg.warmup) << "), ENOBUFS " << dropped << ", "
              << std::fixed << std::setprecision(2) << secs << " s\n";
    lateness.print(std::cout);
    return status;
}

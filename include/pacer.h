#ifndef UDP_SENDER_PACER_H
#define UDP_SENDER_PACER_H

#include <cstdint>
#include <random>

/// Absolute send schedule on a monotonic clock:
/// packet i is due at start + i * interval + jitter_i, jitter_i uniform in
/// [-jitter, +jitter]. Being late never shifts later packets.
/// Clock, sleep and real-time setup are per platform: pacer_mac.cpp, pacer_linux.cpp.
class Pacer {
public:
    /// @p jitter_us must be < interval_us / 2 so that send times keep increasing.
    Pacer(uint32_t interval_us, uint32_t jitter_us);

    /// Starts the schedule: packet 0 is due @p lead_us from now.
    void start(uint32_t lead_us = 10'000);

    /// Waits until the next packet is due: sleeps until kSpinNs before the
    /// deadline, then spins (wake-up from sleep alone is tens of us late).
    /// @return lateness in ns (>= 0), or -1 if g_stop was set meanwhile
    int64_t wait_next();

    /// Real-time scheduling of the calling thread
    /// (macOS: THREAD_TIME_CONSTRAINT_POLICY, Linux: SCHED_FIFO + mlockall).
    /// @throws std::runtime_error / std::system_error if refused
    void make_realtime() const;

    static constexpr uint64_t kSpinNs = 200'000;

private:
    static uint64_t now_ns();                // monotonic
    static void sleep_until_ns(uint64_t t);  // may return early on a signal

    uint64_t interval_ns_;
    uint64_t start_ns_ = 0;
    uint64_t next_ = 0;  // index of the next packet

    std::mt19937_64 rng_;
    std::uniform_int_distribution<int64_t> jitter_ns_;
};

#endif // UDP_SENDER_PACER_H

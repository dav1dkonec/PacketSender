#include "pacer.h"

#include <algorithm>

#include "stop_signal.h"

Pacer::Pacer(uint32_t interval_us, uint32_t jitter_us)
    : interval_ns_(uint64_t{interval_us} * 1000),
      rng_(std::random_device{}()),
      jitter_ns_(-int64_t{jitter_us} * 1000, int64_t{jitter_us} * 1000)
{
}

void Pacer::start(uint32_t lead_us)
{
    start_ns_ = now_ns() + uint64_t{lead_us} * 1000;
    next_ = 0;
}

int64_t Pacer::wait_next()
{
    // a negative offset (jitter > lead) is clamped to the start
    const int64_t offset = static_cast<int64_t>(next_ * interval_ns_) + jitter_ns_(rng_);
    const uint64_t due = start_ns_ + static_cast<uint64_t>(std::max<int64_t>(offset, 0));
    ++next_;

    for (;;) {
        if (g_stop) {
            return -1;
        }
        const uint64_t now = now_ns();
        if (now >= due) {
            return static_cast<int64_t>(now - due);
        }
        if (due - now > kSpinNs) {
            sleep_until_ns(due - kSpinNs);  // loop re-checks g_stop and the time
        }
    }
}

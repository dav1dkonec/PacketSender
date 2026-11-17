// Pacer platform part for macOS: mach absolute time

#include "pacer.h"

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <pthread.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {

// ns = ticks * numer / denom; Apple Silicon: 125/3 (24 MHz ticks), Intel: 1/1
const mach_timebase_info_data_t& timebase()
{
    static const mach_timebase_info_data_t tb = [] {
        mach_timebase_info_data_t t{};
        mach_timebase_info(&t);
        return t;
    }();
    return tb;
}

uint64_t to_ticks(uint64_t ns) { return ns * timebase().denom / timebase().numer; }

} // namespace

uint64_t Pacer::now_ns()
{
    return mach_absolute_time() * timebase().numer / timebase().denom;
}

void Pacer::sleep_until_ns(uint64_t t)
{
    mach_wait_until(to_ticks(t));  // returns early on a signal
}

void Pacer::make_realtime() const
{
    // computation: CPU time needed per period (spin + send), constraint: deadline
    // within the period; the kernel accepts roughly 50 us - 50 ms
    const uint64_t computation = kSpinNs + 100'000;
    const uint64_t constraint = std::clamp<uint64_t>(interval_ns_, computation, 50'000'000);

    thread_time_constraint_policy_data_t p{};
    p.period = static_cast<uint32_t>(to_ticks(std::min<uint64_t>(interval_ns_, 1'000'000'000)));
    p.computation = static_cast<uint32_t>(to_ticks(computation));
    p.constraint = static_cast<uint32_t>(to_ticks(constraint));
    p.preemptible = 1;

    const kern_return_t kr = thread_policy_set(
        pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY,
        reinterpret_cast<thread_policy_t>(&p), THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    if (kr != KERN_SUCCESS) {
        throw std::runtime_error(std::string("thread_policy_set: ") + mach_error_string(kr));
    }
}

// Pacer platform part for Linux: CLOCK_MONOTONIC, SCHED_FIFO

#include "pacer.h"

#include <sched.h>
#include <sys/mman.h>
#include <time.h>

#include <cerrno>
#include <system_error>

namespace {

constexpr int kFifoPriority = 80;  // below kernel threads at 99, as in latency_rx

} // namespace

uint64_t Pacer::now_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

void Pacer::sleep_until_ns(uint64_t t)
{
    const timespec ts{static_cast<time_t>(t / 1'000'000'000), static_cast<long>(t % 1'000'000'000)};
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);  // EINTR on a signal
}

void Pacer::make_realtime() const
{
    if (::mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(),
                                err == ENOMEM ? "mlockall (RLIMIT_MEMLOCK too low: ulimit -l)"
                                              : "mlockall (needs CAP_IPC_LOCK)");
    }
    const sched_param sp{kFifoPriority};
    if (::sched_setscheduler(0, SCHED_FIFO, &sp) < 0) {
        const int err = errno;
        throw std::system_error(err, std::generic_category(),
                                err == EPERM ? "SCHED_FIFO (needs CAP_SYS_NICE or ulimit -r)"
                                             : "SCHED_FIFO");
    }
}

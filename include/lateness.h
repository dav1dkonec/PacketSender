#ifndef UDP_SENDER_LATENESS_H
#define UDP_SENDER_LATENESS_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <vector>

/// How late each packet left against its schedule (Pacer::wait_next()).
class Lateness {
public:
    /// Preallocates, so add() never allocates inside the send loop.
    explicit Lateness(std::size_t capacity);

    /// Drops values beyond the capacity.
    void add(int64_t ns);

    /// min / avg / p99 / max in us; sorts the samples.
    void print(std::ostream& out);

    std::size_t size() const { return ns_.size(); }

private:
    std::vector<int64_t> ns_;
};

#endif // UDP_SENDER_LATENESS_H

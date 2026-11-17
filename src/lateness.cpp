#include "lateness.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <ostream>

Lateness::Lateness(std::size_t capacity)
{
    ns_.reserve(capacity);
}

void Lateness::add(int64_t ns)
{
    if (ns_.size() < ns_.capacity()) {
        ns_.push_back(ns);
    }
}

void Lateness::print(std::ostream& out)
{
    if (ns_.empty()) {
        out << "lateness: no samples\n";
        return;
    }
    std::sort(ns_.begin(), ns_.end());
    const double avg = std::accumulate(ns_.begin(), ns_.end(), 0.0) / static_cast<double>(ns_.size());
    // nearest rank: the smallest value with at least 99 % of samples <= it
    const int64_t p99 = ns_[(ns_.size() * 99 + 99) / 100 - 1];

    out << std::fixed << std::setprecision(1)
        << "lateness us: min " << static_cast<double>(ns_.front()) / 1000
        << " / avg " << avg / 1000
        << " / p99 " << static_cast<double>(p99) / 1000
        << " / max " << static_cast<double>(ns_.back()) / 1000 << '\n';
}

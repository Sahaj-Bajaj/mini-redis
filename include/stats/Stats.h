#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace miniredis::stats {

// Lock-free server statistics. Counters are updated on every command path.
// Intentionally a plain struct — no encapsulation needed for atomic counters.
struct Stats {
    std::atomic<uint64_t> totalCommands{0};
    std::atomic<uint64_t> hits{0};         // GET returning a value
    std::atomic<uint64_t> misses{0};       // GET returning NOT_FOUND
    std::atomic<uint64_t> expiredKeys{0};  // keys reaped by active sweep

    // Returns a human-readable INFO blob.
    [[nodiscard]] std::string report(std::size_t keyCount,
                                     std::size_t shardCount) const;
};

}  // namespace miniredis::stats
#include "stats/Stats.h"

#include <string>

namespace miniredis::stats {

std::string Stats::report(std::size_t keyCount, std::size_t shardCount) const {
    return
        "total_commands:" + std::to_string(totalCommands.load()) + "\r\n" +
        "hits:"           + std::to_string(hits.load())          + "\r\n" +
        "misses:"         + std::to_string(misses.load())        + "\r\n" +
        "expired_keys:"   + std::to_string(expiredKeys.load())   + "\r\n" +
        "total_keys:"     + std::to_string(keyCount)             + "\r\n" +
        "shards:"         + std::to_string(shardCount)           + "\r\n";
}

}  // namespace miniredis::stats
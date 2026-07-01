#pragma once

#include <array>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace miniredis::store {

// Sharded key-value store with per-shard LRU eviction and lock striping.
// Keys are distributed across shards by hash; each shard owns its mutex,
// so threads operating on different keys never contend.
class KvStore {
public:
    static constexpr std::size_t kShardCount = 16;

    // maxSize: total key cap across all shards (0 = unbounded).
    explicit KvStore(std::size_t maxSize = 0) noexcept;

    void set(const std::string& key, std::string value);
    [[nodiscard]] std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t maxSize() const noexcept;

private:
    struct Entry {
        std::string value;
        std::list<std::string>::iterator lruIt;
    };

    struct Shard {
        mutable std::mutex mutex;
        std::list<std::string> lruOrder;
        std::unordered_map<std::string, Entry> data;
    };

    std::array<Shard, kShardCount> shards_;
    std::size_t maxPerShard_;
    std::size_t maxSize_;

    [[nodiscard]] std::size_t shardIndex(const std::string& key) const noexcept;

    // Must be called with the shard's mutex held.
    void moveToFront(
        Shard& shard,
        std::unordered_map<std::string, Entry>::iterator it);

    void evictLru(Shard& shard);
};

} // namespace miniredis::store
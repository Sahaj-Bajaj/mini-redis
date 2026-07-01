#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace miniredis::store {

// Sharded key-value store with per-shard LRU eviction, TTL support,
// passive expiry on access, and a background sweep thread for active expiry.
class KvStore {
public:
    static constexpr std::size_t kShardCount = 16;
    static constexpr auto kSweepInterval = std::chrono::seconds(1);

    explicit KvStore(std::size_t maxSize = 0) noexcept;
    ~KvStore();

    void set(const std::string& key, std::string value);
    [[nodiscard]] std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);

    // Sets expiry on an existing key. Returns false if key not found.
    bool expire(const std::string& key, std::chrono::seconds ttl);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t maxSize() const noexcept;

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct Entry {
        std::string value;
        std::list<std::string>::iterator lruIt;
        std::optional<TimePoint> expiry;  // nullopt = no TTL
    };

    struct Shard {
        mutable std::mutex mutex;
        std::list<std::string> lruOrder;
        std::unordered_map<std::string, Entry> data;
    };

    std::array<Shard, kShardCount> shards_;
    std::size_t maxPerShard_;
    std::size_t maxSize_;

    // Active expiry background thread.
    std::thread sweepThread_;
    std::atomic<bool> stopSweep_{false};
    std::condition_variable sweepCv_;
    std::mutex sweepMutex_;

    [[nodiscard]] std::size_t shardIndex(const std::string& key) const noexcept;

    // Must be called with shard mutex held.
    void moveToFront(
        Shard& shard,
        std::unordered_map<std::string, Entry>::iterator it);

    void evictLru(Shard& shard);

    // Returns true and erases if entry is expired. Shard mutex must be held.
    bool isExpiredAndErase(
        Shard& shard,
        std::unordered_map<std::string, Entry>::iterator it);

    void sweepLoop();
};

}  // namespace miniredis::store
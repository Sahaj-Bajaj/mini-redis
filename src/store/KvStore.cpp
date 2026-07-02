#include "store/KvStore.h"

#include <functional>

namespace miniredis::store {

KvStore::KvStore(std::size_t maxSize) noexcept
    : maxPerShard_(maxSize == 0 ? 0 : (maxSize + kShardCount - 1) / kShardCount),
      maxSize_(maxSize) {
    sweepThread_ = std::thread(&KvStore::sweepLoop, this);
}

KvStore::~KvStore() {
    {
        std::scoped_lock lock(sweepMutex_);
        stopSweep_.store(true);
    }
    sweepCv_.notify_all();
    sweepThread_.join();
}

std::size_t KvStore::shardIndex(const std::string& key) const noexcept {
    return std::hash<std::string>{}(key) & (kShardCount - 1);
}

void KvStore::moveToFront(
    Shard& shard,
    std::unordered_map<std::string, Entry>::iterator it) {
    shard.lruOrder.splice(shard.lruOrder.begin(), shard.lruOrder, it->second.lruIt);
}

void KvStore::evictLru(Shard& shard) {
    shard.data.erase(shard.lruOrder.back());
    shard.lruOrder.pop_back();
}

bool KvStore::isExpiredAndErase(
    Shard& shard,
    std::unordered_map<std::string, Entry>::iterator it) {
    if (!it->second.expiry) {
        return false;
    }

    if (Clock::now() < *it->second.expiry) {
        return false;
    }

    shard.lruOrder.erase(it->second.lruIt);
    shard.data.erase(it);
    return true;
}

void KvStore::set(const std::string& key, std::string value) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it != shard.data.end()) {
        it->second.value = std::move(value);
        it->second.expiry = std::nullopt;  // SET clears any existing TTL.
        moveToFront(shard, it);
        return;
    }

    if (maxPerShard_ > 0 && shard.data.size() == maxPerShard_) {
        evictLru(shard);
    }

    shard.lruOrder.push_front(key);
    shard.data.emplace(
        key,
        Entry{
            std::move(value),
            shard.lruOrder.begin(),
            std::nullopt
        });
}

std::optional<std::string> KvStore::get(const std::string& key) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return std::nullopt;
    }

    if (isExpiredAndErase(shard, it)) {
        return std::nullopt;  // Passive expiry.
    }

    moveToFront(shard, it);
    return it->second.value;
}

bool KvStore::del(const std::string& key) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return false;
    }

    shard.lruOrder.erase(it->second.lruIt);
    shard.data.erase(it);
    return true;
}

bool KvStore::expire(const std::string& key, std::chrono::seconds ttl) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return false;
    }

    it->second.expiry = Clock::now() + ttl;
    return true;
}

std::size_t KvStore::size() const noexcept {
    std::size_t total = 0;

    for (const auto& shard : shards_) {
        std::scoped_lock lock(shard.mutex);
        total += shard.data.size();
    }

    return total;
}

std::size_t KvStore::maxSize() const noexcept {
    return maxSize_;
}

std::size_t KvStore::shardSize(std::size_t shardIdx) const noexcept {
    if (shardIdx >= kShardCount) {
        return 0;
    }

    std::scoped_lock lock(shards_[shardIdx].mutex);
    return shards_[shardIdx].data.size();
}

void KvStore::sweepLoop() {
    while (true) {
        {
            std::unique_lock lock(sweepMutex_);
            sweepCv_.wait_for(
                lock,
                kSweepInterval,
                [this] { return stopSweep_.load(); });
        }

        if (stopSweep_.load()) {
            return;
        }

        for (auto& shard : shards_) {
            std::scoped_lock lock(shard.mutex);

            for (auto it = shard.data.begin(); it != shard.data.end();) {
                if (it->second.expiry &&
    Clock::now() >= *it->second.expiry) {
    shard.lruOrder.erase(it->second.lruIt);
    it = shard.data.erase(it);
    expiredKeyCount.fetch_add(1, std::memory_order_relaxed);
} else {
    ++it;
}
            }
        }
    }
}

}  // namespace miniredis::store
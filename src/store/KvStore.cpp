#include "store/KvStore.h"

#include <functional>

namespace miniredis::store {

KvStore::KvStore(std::size_t maxSize) noexcept
    : maxPerShard_(maxSize == 0 ? 0 : (maxSize + kShardCount - 1) / kShardCount),
      maxSize_(maxSize) {}

std::size_t KvStore::shardIndex(const std::string& key) const noexcept {
    // Power-of-2 shard count lets us use bitmask instead of modulo.
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

void KvStore::set(const std::string& key, std::string value) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it != shard.data.end()) {
        it->second.value = std::move(value);
        moveToFront(shard, it);
        return;
    }

    if (maxPerShard_ > 0 && shard.data.size() == maxPerShard_) {
        evictLru(shard);
    }

    shard.lruOrder.push_front(key);
    shard.data.emplace(key, Entry{std::move(value), shard.lruOrder.begin()});
}

std::optional<std::string> KvStore::get(const std::string& key) {
    Shard& shard = shards_[shardIndex(key)];
    std::scoped_lock lock(shard.mutex);

    auto it = shard.data.find(key);
    if (it == shard.data.end()) {
        return std::nullopt;
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

} // namespace miniredis::store
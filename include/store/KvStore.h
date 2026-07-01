#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace miniredis::store {

// Thread-safe key-value store with LRU eviction.
// Uses a single mutex; Phase 5 replaces this with per-shard locking.
class KvStore {
public:
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

    std::list<std::string>                 lruOrder_;
    std::unordered_map<std::string, Entry> data_;
    std::size_t                            maxSize_;
    mutable std::mutex                     mutex_;

    // Must be called with mutex_ held.
    void moveToFront(std::unordered_map<std::string, Entry>::iterator it);
    void evictLru();
};

} // namespace miniredis::store
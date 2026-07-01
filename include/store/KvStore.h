#pragma once

#include <list>
#include <optional>
#include <string>
#include <unordered_map>

namespace miniredis::store {

// Key-value store with LRU eviction and a configurable max size.
// Uses the classic list + unordered_map pattern for O(1) get, set, and evict.
// Not thread-safe; synchronization is added in Phase 4.
class KvStore {
public:
    // maxSize = 0 means unbounded (no eviction).
    explicit KvStore(std::size_t maxSize = 0) noexcept;

    void set(const std::string& key, std::string value);

    [[nodiscard]] std::optional<std::string> get(const std::string& key);

    // Returns true if the key existed and was removed.
    bool del(const std::string& key);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t maxSize() const noexcept;

private:
    // List order:
    // front = most recently used
    // back  = least recently used
    std::list<std::string> lruOrder_;

    struct Entry {
        std::string value;
        std::list<std::string>::iterator lruIt;
    };

    std::unordered_map<std::string, Entry> data_;

    std::size_t maxSize_;

    void moveToFront(std::unordered_map<std::string, Entry>::iterator it);
    void evictLru();
};

}  // namespace miniredis::store
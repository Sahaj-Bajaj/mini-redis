#include "store/KvStore.h"

namespace miniredis::store {

KvStore::KvStore(std::size_t maxSize) noexcept : maxSize_(maxSize) {}

void KvStore::moveToFront(std::unordered_map<std::string, Entry>::iterator it) {
    lruOrder_.splice(lruOrder_.begin(), lruOrder_, it->second.lruIt);
}

void KvStore::evictLru() {
    const std::string& lruKey = lruOrder_.back();
    data_.erase(lruKey);
    lruOrder_.pop_back();
}

void KvStore::set(const std::string& key, std::string value) {
    auto it = data_.find(key);

    if (it != data_.end()) {
        it->second.value = std::move(value);
        moveToFront(it);
        return;
    }

    if (maxSize_ > 0 && data_.size() == maxSize_) {
        evictLru();
    }

    lruOrder_.push_front(key);
    data_.emplace(key, Entry{std::move(value), lruOrder_.begin()});
}

std::optional<std::string> KvStore::get(const std::string& key) {
    auto it = data_.find(key);

    if (it == data_.end()) {
        return std::nullopt;
    }

    moveToFront(it);
    return it->second.value;
}

bool KvStore::del(const std::string& key) {
    auto it = data_.find(key);

    if (it == data_.end()) {
        return false;
    }

    lruOrder_.erase(it->second.lruIt);
    data_.erase(it);
    return true;
}

std::size_t KvStore::size() const noexcept {
    return data_.size();
}

std::size_t KvStore::maxSize() const noexcept {
    return maxSize_;
}

}  // namespace miniredis::store
#include "store/KvStore.h"

#include <utility>

namespace miniredis::store {

void KvStore::set(const std::string& key, std::string value) {
    data_.insert_or_assign(key, std::move(value));
}

std::optional<std::string> KvStore::get(const std::string& key) const {
    const auto it = data_.find(key);

    if (it == data_.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool KvStore::del(const std::string& key) {
    return data_.erase(key) > 0;
}

std::size_t KvStore::size() const noexcept {
    return data_.size();
}

} // namespace miniredis::store
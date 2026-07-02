#pragma once

#include "store/KvStore.h"
#include <cstddef>
#include <string>

namespace miniredis::persistence {

// Replays an AOF file against a KvStore on startup to restore state.
class AofReader {
public:
    // Returns commands replayed. Returns 0 if file does not exist (fresh start).
    [[nodiscard]] static std::size_t replay(const std::string& path,
                                             store::KvStore&    store);
};

}  // namespace miniredis::persistence
#pragma once

#include <cstdint>
#include <string>

namespace miniredis::protocol {

// Formats server responses in RESP2, compatible with redis-cli and any RESP2 client.
class RespFormatter {
public:
    [[nodiscard]] static std::string ok();
    [[nodiscard]] static std::string pong();
    [[nodiscard]] static std::string nullBulk();
    [[nodiscard]] static std::string error(const std::string& msg);
    [[nodiscard]] static std::string integer(int64_t value);
    [[nodiscard]] static std::string bulkString(const std::string& value);
};

}  // namespace miniredis::protocol
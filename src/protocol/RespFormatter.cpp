#include "protocol/RespFormatter.h"

namespace miniredis::protocol {

std::string RespFormatter::ok()                        { return "+OK\r\n"; }
std::string RespFormatter::pong()                      { return "+PONG\r\n"; }
std::string RespFormatter::nullBulk()                  { return "$-1\r\n"; }
std::string RespFormatter::error(const std::string& m) { return "-ERR " + m + "\r\n"; }
std::string RespFormatter::integer(int64_t v)          { return ":" + std::to_string(v) + "\r\n"; }
std::string RespFormatter::bulkString(const std::string& v) {
    return "$" + std::to_string(v.size()) + "\r\n" + v + "\r\n";
}

}  // namespace miniredis::protocol
#pragma once

#include <string>
#include <vector>

namespace miniredis::protocol {

enum class CommandType {
    Set,
    Get,
    Del,
    Expire,
    Unknown
};

struct Command {
    CommandType type = CommandType::Unknown;
    std::vector<std::string> args;
};

}  // namespace miniredis::protocol
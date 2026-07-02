#pragma once

#include "protocol/Command.h"
#include <string>
#include <vector>

namespace miniredis::protocol {

// Parses RESP2 array commands from a byte buffer.
// Handles pipelining: returns all complete commands from one buffer read.
// Incomplete commands are left in the buffer for the next recv().
class RespParser {
public:
    [[nodiscard]] static std::vector<Command> parse(std::string& buffer);
};

}  // namespace miniredis::protocol
#pragma once

#include "protocol/Command.h"

#include <string_view>

namespace miniredis::protocol {

class CommandParser {
public:
    [[nodiscard]]
    static Command parse(std::string_view line);
};

} // namespace miniredis::protocol
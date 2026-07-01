#include "protocol/CommandParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace miniredis::protocol {

Command CommandParser::parse(std::string_view line) {
    std::vector<std::string> tokens;

    {
        std::istringstream stream{std::string(line)};
        std::string token;

        while (stream >> token) {
            tokens.push_back(std::move(token));
        }
    }

    if (tokens.empty()) {
        return {};
    }

    std::string verb = tokens[0];

    std::transform(
        verb.begin(),
        verb.end(),
        verb.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

    if (verb == "SET" && tokens.size() >= 3) {

        std::string value;

        for (std::size_t i = 2; i < tokens.size(); ++i) {
            if (i > 2) {
                value += ' ';
            }

            value += tokens[i];
        }

        return {
            CommandType::Set,
            {tokens[1], std::move(value)}
        };
    }

    if (verb == "GET" && tokens.size() == 2) {
        return {
            CommandType::Get,
            {tokens[1]}
        };
    }

    if (verb == "DEL" && tokens.size() == 2) {
        return {
            CommandType::Del,
            {tokens[1]}
        };
    }

    return {};
}

} // namespace miniredis::protocol
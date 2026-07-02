#include "protocol/RespParser.h"
#include "protocol/CommandParser.h"

#include <charconv>

namespace miniredis::protocol {

namespace {

bool readLine(const std::string& buf, std::size_t& pos, std::string& out) {
    const auto cr = buf.find('\r', pos);
    if (cr == std::string::npos || cr + 1 >= buf.size() || buf[cr + 1] != '\n')
        return false;
    out = buf.substr(pos, cr - pos);
    pos = cr + 2;
    return true;
}

}  // namespace

std::vector<Command> RespParser::parse(std::string& buffer) {
    std::vector<Command> commands;
    std::size_t pos = 0;

    while (pos < buffer.size()) {
        if (buffer[pos] != '*') break;

        const std::size_t cmdStart = pos++;

        std::string countStr;
        if (!readLine(buffer, pos, countStr)) { pos = cmdStart; break; }

        int count = 0;
        if (std::from_chars(countStr.data(),
                            countStr.data() + countStr.size(),
                            count).ec != std::errc{} || count <= 0) {
            pos = cmdStart; break;
        }

        std::vector<std::string> tokens;
        tokens.reserve(static_cast<std::size_t>(count));
        bool incomplete = false;

        for (int i = 0; i < count; ++i) {
            if (pos >= buffer.size() || buffer[pos] != '$') { incomplete = true; break; }
            ++pos;

            std::string lenStr;
            if (!readLine(buffer, pos, lenStr)) { incomplete = true; break; }

            std::size_t len = 0;
            if (std::from_chars(lenStr.data(),
                                lenStr.data() + lenStr.size(),
                                len).ec != std::errc{}) { incomplete = true; break; }

            if (pos + len + 2 > buffer.size()) { incomplete = true; break; }
            tokens.push_back(buffer.substr(pos, len));
            pos += len + 2;
        }

        if (incomplete) { pos = cmdStart; break; }

        // Reconstruct as space-separated line; CommandParser handles dispatch.
        std::string line;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) line += ' ';
            line += tokens[i];
        }
        commands.push_back(CommandParser::parse(line));
    }

    buffer.erase(0, pos);
    return commands;
}

}  // namespace miniredis::protocol
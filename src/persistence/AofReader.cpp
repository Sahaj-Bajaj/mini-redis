#include "persistence/AofReader.h"
#include "protocol/Command.h"
#include "protocol/CommandParser.h"

#include <fstream>
#include <string>

namespace miniredis::persistence {

std::size_t AofReader::replay(const std::string& path, store::KvStore& store) {
    std::ifstream file(path);
    if (!file.is_open()) return 0;

    std::size_t count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        const auto cmd = protocol::CommandParser::parse(line);
        using protocol::CommandType;
        switch (cmd.type) {
            case CommandType::Set: store.set(cmd.args[0], cmd.args[1]); ++count; break;
            case CommandType::Del: store.del(cmd.args[0]);               ++count; break;
            default: break;
        }
    }
    return count;
}

}  // namespace miniredis::persistence
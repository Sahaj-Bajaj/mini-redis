#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace miniredis::persistence {

// Appends SET and DEL commands to a file for durability.
// Thread-safe; multiple worker threads share one writer.
// EXPIRE is not persisted — keys lose TTL across restarts.
class AofWriter {
public:
    explicit AofWriter(const std::string& path);
    ~AofWriter();
    AofWriter(const AofWriter&)            = delete;
    AofWriter& operator=(const AofWriter&) = delete;

    void writeSet(const std::string& key, const std::string& value);
    void writeDel(const std::string& key);

private:
    std::ofstream file_;
    std::mutex    mutex_;
    void writeLine(const std::string& line);
};

}  // namespace miniredis::persistence
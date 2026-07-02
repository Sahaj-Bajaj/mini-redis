#include "persistence/AofWriter.h"
#include <stdexcept>

namespace miniredis::persistence {

AofWriter::AofWriter(const std::string& path)
    : file_(path, std::ios::app) {
    if (!file_.is_open())
        throw std::runtime_error("Cannot open AOF: " + path);
}

AofWriter::~AofWriter() {
    if (file_.is_open()) file_.flush();
}

void AofWriter::writeLine(const std::string& line) {
    std::scoped_lock lock(mutex_);
    file_ << line << '\n';
    file_.flush();
    // No fsync: relies on OS page cache. Configurable policy is a polishing item.
}

void AofWriter::writeSet(const std::string& key, const std::string& value) {
    writeLine("SET " + key + " " + value);
}

void AofWriter::writeDel(const std::string& key) {
    writeLine("DEL " + key);
}

}  // namespace miniredis::persistence
#include "net/Socket.h"
#include "net/TcpListener.h"
#include "net/ThreadPool.h"
#include "persistence/AofReader.h"
#include "persistence/AofWriter.h"
#include "protocol/Command.h"
#include "protocol/CommandParser.h"
#include "protocol/RespFormatter.h"
#include "protocol/RespParser.h"
#include "stats/Stats.h"
#include "store/KvStore.h"

#include <sys/socket.h>

#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

constexpr uint16_t    kPort        = 6380;
constexpr std::size_t kBufferSize  = 4096;
constexpr std::size_t kThreadCount = 4;
constexpr std::size_t kMaxKeys     = 1024;
constexpr const char* kAofPath     = "miniredis.aof";

enum class Protocol { Text, Resp };

void sendAll(miniredis::net::Socket& socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = ::send(socket.fd(),
                           data.data() + sent,
                           data.size() - sent,
                           MSG_NOSIGNAL);
        if (n < 0) { std::cerr << "send() failed: " << std::strerror(errno) << '\n'; return; }
        sent += static_cast<std::size_t>(n);
    }
}

std::string execute(const miniredis::protocol::Command&   cmd,
                    miniredis::store::KvStore&             store,
                    miniredis::stats::Stats&               stats,
                    miniredis::persistence::AofWriter&     aof,
                    Protocol                               proto) {
    using CT = miniredis::protocol::CommandType;
    using RF = miniredis::protocol::RespFormatter;

    const bool resp = (proto == Protocol::Resp);
    stats.totalCommands.fetch_add(1, std::memory_order_relaxed);

    switch (cmd.type) {
        case CT::Ping:
            return resp ? RF::pong() : "PONG\r\n";

        case CT::Set:
            store.set(cmd.args[0], cmd.args[1]);
            aof.writeSet(cmd.args[0], cmd.args[1]);
            return resp ? RF::ok() : "OK\r\n";

        case CT::Get: {
            auto val = store.get(cmd.args[0]);
            if (val) {
                stats.hits.fetch_add(1, std::memory_order_relaxed);
                return resp ? RF::bulkString(*val) : "VALUE " + *val + "\r\n";
            }
            stats.misses.fetch_add(1, std::memory_order_relaxed);
            return resp ? RF::nullBulk() : "NOT_FOUND\r\n";
        }

        case CT::Del: {
            const bool deleted = store.del(cmd.args[0]);
            if (deleted) aof.writeDel(cmd.args[0]);
            return resp ? RF::integer(deleted ? 1 : 0)
                        : (deleted ? "DELETED\r\n" : "NOT_FOUND\r\n");
        }

        case CT::Expire: {
            int secs = 0;
            auto [ptr, ec] = std::from_chars(cmd.args[1].data(),
                                              cmd.args[1].data() + cmd.args[1].size(), secs);
            if (ec != std::errc{} || secs < 0)
                return resp ? RF::error("invalid TTL") : "ERROR invalid TTL\r\n";
            const bool ok = store.expire(cmd.args[0], std::chrono::seconds(secs));
            return resp ? RF::integer(ok ? 1 : 0)
                        : (ok ? "OK\r\n" : "NOT_FOUND\r\n");
        }

        case CT::Size:
            return resp ? RF::integer(static_cast<int64_t>(store.size()))
                        : "SIZE " + std::to_string(store.size()) + "\r\n";

        case CT::Shards: {
            std::string out;
            for (std::size_t i = 0; i < miniredis::store::KvStore::kShardCount; ++i)
                out += "shard[" + std::to_string(i) + "]:"
                     + std::to_string(store.shardSize(i)) + "\r\n";
            return resp ? RF::bulkString(out) : out;
        }

        case CT::Info: {
            stats.expiredKeys.store(
                store.expiredKeyCount.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            const auto report = stats.report(store.size(),
                                              miniredis::store::KvStore::kShardCount);
            return resp ? RF::bulkString(report) : report;
        }

        case CT::Unknown:
        default:
            return resp ? RF::error("unknown command") : "ERROR unknown command\r\n";
    }
}

void serveClient(miniredis::net::Socket            client,
                 miniredis::store::KvStore&         store,
                 miniredis::stats::Stats&           stats,
                 miniredis::persistence::AofWriter& aof) {
    std::string buffer;
    std::array<char, kBufferSize> chunk{};
    Protocol proto         = Protocol::Text;
    bool     protoDetected = false;

    while (true) {
        ssize_t n = ::recv(client.fd(), chunk.data(), chunk.size(), 0);
        if (n <= 0) return;
        buffer.append(chunk.data(), static_cast<std::size_t>(n));

        if (!protoDetected && !buffer.empty()) {
            proto         = (buffer[0] == '*') ? Protocol::Resp : Protocol::Text;
            protoDetected = true;
        }

        if (proto == Protocol::Resp) {
            const auto commands = miniredis::protocol::RespParser::parse(buffer);
            for (const auto& cmd : commands)
                sendAll(client, execute(cmd, store, stats, aof, Protocol::Resp));
        } else {
            std::size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                buffer.erase(0, pos + 1);
                auto cmd = miniredis::protocol::CommandParser::parse(line);
                sendAll(client, execute(cmd, store, stats, aof, Protocol::Text));
            }
        }
    }
}

}  // namespace

int main() {
    try {
        miniredis::store::KvStore store(kMaxKeys);

        const std::size_t replayed =
            miniredis::persistence::AofReader::replay(kAofPath, store);
        if (replayed > 0)
            std::cout << "Replayed " << replayed << " commands from " << kAofPath << '\n';

        miniredis::net::TcpListener            listener(kPort);
        miniredis::net::ThreadPool             pool(kThreadCount);
        miniredis::stats::Stats                stats;
        miniredis::persistence::AofWriter      aof(kAofPath);

        std::cout << "miniredis listening on port " << kPort
                  << " (" << kThreadCount << " threads)\n";

        while (true) {
            auto client = std::make_shared<miniredis::net::Socket>(listener.accept());

pool.enqueue([&store, &stats, &aof, client]() {
    serveClient(std::move(*client), store, stats, aof);
});
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
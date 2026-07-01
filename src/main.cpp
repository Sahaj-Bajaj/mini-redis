#include "net/Socket.h"
#include "net/TcpListener.h"
#include "net/ThreadPool.h"
#include "protocol/Command.h"
#include "protocol/CommandParser.h"
#include "store/KvStore.h"

#include <memory>

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

constexpr uint16_t kPort = 6380;
constexpr std::size_t kBufferSize = 4096;
constexpr std::size_t kThreadCount = 4;
constexpr std::size_t kMaxKeys = 1024;

void sendAll(miniredis::net::Socket& socket, const std::string& data) {
    std::size_t sent = 0;

    while (sent < data.size()) {
        ssize_t n = ::send(
            socket.fd(),
            data.data() + sent,
            data.size() - sent,
            MSG_NOSIGNAL);

        if (n < 0) {
            std::cerr << "send() failed: " << std::strerror(errno) << '\n';
            return;
        }

        sent += static_cast<std::size_t>(n);
    }
}

std::string execute(
    const miniredis::protocol::Command& cmd,
    miniredis::store::KvStore& store) {

    using miniredis::protocol::CommandType;

    switch (cmd.type) {
        case CommandType::Set:
            store.set(cmd.args[0], cmd.args[1]);
            return "OK\r\n";

        case CommandType::Get: {
            auto value = store.get(cmd.args[0]);
            return value ? ("VALUE " + *value + "\r\n") : "NOT_FOUND\r\n";
        }

        case CommandType::Del:
            return store.del(cmd.args[0])
                ? "DELETED\r\n"
                : "NOT_FOUND\r\n";

        case CommandType::Expire: {
            int seconds = 0;

            auto [ptr, ec] = std::from_chars(
                cmd.args[1].data(),
                cmd.args[1].data() + cmd.args[1].size(),
                seconds);

            if (ec != std::errc{} || seconds < 0) {
                return "ERROR invalid TTL\r\n";
            }

            return store.expire(cmd.args[0], std::chrono::seconds(seconds))
                ? "OK\r\n"
                : "NOT_FOUND\r\n";
        }

        case CommandType::Unknown:
        default:
            return "ERROR unknown command\r\n";
    }
}

void serveClient(
    miniredis::net::Socket client,
    miniredis::store::KvStore& store) {

    std::string buffer;
    std::array<char, kBufferSize> chunk{};

    while (true) {
        ssize_t n = ::recv(client.fd(), chunk.data(), chunk.size(), 0);

        if (n <= 0) {
            return;
        }

        buffer.append(chunk.data(), static_cast<std::size_t>(n));

        std::size_t pos;

        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            buffer.erase(0, pos + 1);

            auto cmd = miniredis::protocol::CommandParser::parse(line);
            sendAll(client, execute(cmd, store));
        }
    }
}

}  // namespace

int main() {
    try {
        miniredis::net::TcpListener listener(kPort);
        miniredis::net::ThreadPool pool(kThreadCount);
        miniredis::store::KvStore store(kMaxKeys);

        std::cout << "miniredis listening on port "
                  << kPort
                  << " ("
                  << kThreadCount
                  << " worker threads)\n";

        while (true) {
            auto client =
                std::make_shared<miniredis::net::Socket>(listener.accept());

            pool.enqueue([&store, client]() {
                serveClient(std::move(*client), store);
            });
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
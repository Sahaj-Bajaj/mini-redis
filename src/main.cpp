#include "net/Socket.h"
#include "net/TcpListener.h"
#include "protocol/Command.h"
#include "protocol/CommandParser.h"
#include "store/KvStore.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

constexpr uint16_t kPort = 6380;  // 6380, not 6379, to avoid colliding with a real Redis instance.
constexpr std::size_t kBufferSize = 4096;

// Sends every byte in the response.
void sendAll(miniredis::net::Socket& client, const std::string& response) {
    std::size_t totalSent = 0;

    while (totalSent < response.size()) {
        const ssize_t bytesSent = ::send(
            client.fd(),
            response.data() + totalSent,
            response.size() - totalSent,
            MSG_NOSIGNAL);

        if (bytesSent < 0) {
            std::cerr << "send() failed: "
                      << std::strerror(errno)
                      << '\n';
            return;
        }

        totalSent += static_cast<std::size_t>(bytesSent);
    }
}

// Reads commands from the client, executes them against the key-value store,
// and sends back a text response.
void runCommandLoop(miniredis::net::Socket& client,
                    miniredis::store::KvStore& store) {
    std::array<char, kBufferSize> chunk{};
    std::string buffer;

    while (true) {
        const ssize_t bytesRead =
            ::recv(client.fd(), chunk.data(), chunk.size(), 0);

        if (bytesRead < 0) {
            std::cerr << "recv() failed: "
                      << std::strerror(errno)
                      << '\n';
            return;
        }

        if (bytesRead == 0) {
            return;
        }

        buffer.append(chunk.data(),
                      static_cast<std::size_t>(bytesRead));

        std::size_t newlinePos;

        while ((newlinePos = buffer.find('\n')) != std::string::npos) {

            std::string line = buffer.substr(0, newlinePos);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            buffer.erase(0, newlinePos + 1);

            const auto command =
                miniredis::protocol::CommandParser::parse(line);

            std::string response;

            using miniredis::protocol::CommandType;

            switch (command.type) {
                case CommandType::Set:
                    store.set(command.args[0], command.args[1]);
                    response = "OK\n";
                    break;

                case CommandType::Get: {
                    const auto value = store.get(command.args[0]);

                    if (value) {
                        response = *value + "\n";
                    } else {
                        response = "NULL\n";
                    }

                    break;
                }

                case CommandType::Del:
                    response = store.del(command.args[0]) ? "1\n" : "0\n";
                    break;

                case CommandType::Unknown:
                default:
                    response = "ERR unknown command\n";
                    break;
            }

            sendAll(client, response);
        }
    }
}

}  // namespace

int main() {
    try {
        miniredis::net::TcpListener listener(kPort);
        miniredis::store::KvStore store;

        std::cout << "MiniRedis listening on port " << kPort << '\n';

        while (true) {
            std::cout << "Waiting for a client...\n";

            miniredis::net::Socket client = listener.accept();

            std::cout << "Client connected.\n";

            runCommandLoop(client, store);

            std::cout << "Client disconnected.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
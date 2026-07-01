#include "net/Socket.h"
#include "net/TcpListener.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

constexpr uint16_t kPort = 6380;  // 6380, not 6379, to avoid colliding with a real Redis instance.
constexpr std::size_t kBufferSize = 4096;

// Reads from the client and writes back exactly what was read, until the
// client closes the connection or an error occurs.
void runEchoLoop(cache::net::Socket& client) {
    std::array<char, kBufferSize> buffer{};

    while (true) {
        ssize_t bytesRead = ::recv(client.fd(), buffer.data(), buffer.size(), 0);
        if (bytesRead < 0) {
            std::cerr << "recv() failed: " << std::strerror(errno) << '\n';
            return;
        }
        if (bytesRead == 0) {
            return;  // Client closed its side of the connection.
        }

        // send() is not guaranteed to write every byte in one call, so we
        // loop until the whole chunk that was just read is flushed out.
        std::size_t totalSent = 0;
        while (totalSent < static_cast<std::size_t>(bytesRead)) {
            ssize_t bytesSent = ::send(client.fd(),
                                        buffer.data() + totalSent,
                                        static_cast<std::size_t>(bytesRead) - totalSent,
                                        MSG_NOSIGNAL);
            if (bytesSent < 0) {
                std::cerr << "send() failed: " << std::strerror(errno) << '\n';
                return;
            }
            totalSent += static_cast<std::size_t>(bytesSent);
        }
    }
}

}  // namespace

int main() {
    try {
        cache::net::TcpListener listener(kPort);
        std::cout << "cache_server listening on port " << kPort << '\n';

        while (true) {
            std::cout << "Waiting for a client...\n";
            cache::net::Socket client = listener.accept();
            std::cout << "Client connected.\n";

            runEchoLoop(client);

            std::cout << "Client disconnected.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
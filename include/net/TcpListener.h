#pragma once

#include "net/Socket.h"

#include <cstdint>

namespace miniredis::net {

// Owns a listening TCP socket bound to a port on all local interfaces.
// Responsible only for accepting connections -- it has no knowledge of
// what the accepted connections are used for.
class TcpListener {
public:
    explicit TcpListener(uint16_t port, int backlog = 16);

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    TcpListener(TcpListener&&) noexcept = default;
    TcpListener& operator=(TcpListener&&) noexcept = default;

    // Blocks until a client connects. Throws std::runtime_error on failure.
    [[nodiscard]] Socket accept();

private:
    Socket listenSocket_;
};

}  // namespace miniredis::net
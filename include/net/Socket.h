#pragma once

namespace miniredis::net {

// Owns a single POSIX file descriptor for a socket. Guarantees the fd is
// closed exactly once, regardless of how the owning scope is exited.
class Socket {
public:
    Socket() noexcept = default;
    explicit Socket(int fd) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    [[nodiscard]] int fd() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;

    void close() noexcept;

private:
    int fd_ = -1;
};

}  // namespace miniredis::net
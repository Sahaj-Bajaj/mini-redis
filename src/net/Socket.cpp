#include "net/Socket.h"

#include <unistd.h>

#include <utility>

namespace miniredis::net {

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

int Socket::fd() const noexcept {
    return fd_;
}

bool Socket::isValid() const noexcept {
    return fd_ >= 0;
}

void Socket::close() noexcept {
    // Close once, don't retry on failure: on Linux, retrying close() after
    // EINTR can close a different fd that the OS already reused.
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

}  // namespace miniredis::net
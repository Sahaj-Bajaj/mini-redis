#include "net/TcpListener.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace cache::net {

namespace {

[[noreturn]] void throwErrno(const char* what) {
    throw std::runtime_error(std::string(what) + ": " + std::strerror(errno));
}

Socket createListenSocket(uint16_t port, int backlog) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throwErrno("socket() failed");
    }
    Socket socket(fd);

    // Without SO_REUSEADDR, restarting the server quickly fails with
    // "Address already in use": the OS holds the port in TIME_WAIT for a
    // while after the last connection on it closes.
    int reuse = 1;
    if (::setsockopt(socket.fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        throwErrno("setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (::bind(socket.fd(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throwErrno("bind() failed");
    }

    if (::listen(socket.fd(), backlog) < 0) {
        throwErrno("listen() failed");
    }

    return socket;
}

}  // namespace

TcpListener::TcpListener(uint16_t port, int backlog)
    : listenSocket_(createListenSocket(port, backlog)) {}

Socket TcpListener::accept() {
    sockaddr_in clientAddress{};
    socklen_t clientAddressLen = sizeof(clientAddress);

    int clientFd = ::accept(listenSocket_.fd(),
                             reinterpret_cast<sockaddr*>(&clientAddress),
                             &clientAddressLen);
    if (clientFd < 0) {
        throwErrno("accept() failed");
    }

    return Socket(clientFd);
}

}  // namespace cache::net
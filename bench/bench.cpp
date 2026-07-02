// Benchmark client for miniredis.
// Measures throughput (ops/sec) and latency percentiles (p50, p99)
// for SET and GET commands over a persistent TCP connection.
//
// Usage:
//   ./build/miniredis_bench [host] [port] [requests] [threads]
// Defaults: localhost 6380 100000 4

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock    = std::chrono::steady_clock;
using Nanos    = std::chrono::nanoseconds;
using Micros   = std::chrono::microseconds;

// Opens a blocking TCP connection to host:port.
// Returns fd on success, throws on failure.
int connectTo(const std::string& host, uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket() failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        throw std::runtime_error("inet_pton() failed: bad address");
    }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        throw std::runtime_error(std::string("connect() failed: ") + std::strerror(errno));
    }
    return fd;
}

// Sends all bytes in buf over fd.
void sendAll(int fd, const std::string& buf) {
    std::size_t sent = 0;
    while (sent < buf.size()) {
        ssize_t n = ::send(fd, buf.data() + sent, buf.size() - sent, MSG_NOSIGNAL);
        if (n < 0) throw std::runtime_error("send() failed");
        sent += static_cast<std::size_t>(n);
    }
}

// Reads until '\n' is found. Returns the complete line including '\n'.
std::string readLine(int fd) {
    std::string line;
    char c = 0;
    while (true) {
        ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0) throw std::runtime_error("connection closed unexpectedly");
        line += c;
        if (c == '\n') return line;
    }
}

struct Result {
    std::size_t        ops;
    double             elapsedSec;
    std::vector<Nanos> latencies;
};

// Worker: issues `opsPerThread` SET+GET pairs, records per-op latency.
Result runWorker(const std::string& host, uint16_t port, std::size_t opsPerThread) {
    int fd = connectTo(host, port);

    std::vector<Nanos> latencies;
    latencies.reserve(opsPerThread * 2);

    const auto start = Clock::now();

    for (std::size_t i = 0; i < opsPerThread; ++i) {
        const std::string key = "bench:" + std::to_string(i);

        // SET
        {
            auto t0  = Clock::now();
            sendAll(fd, "SET " + key + " value\r\n");
            readLine(fd);
            latencies.push_back(Clock::now() - t0);
        }

        // GET
        {
            auto t0  = Clock::now();
            sendAll(fd, "GET " + key + "\r\n");
            readLine(fd);
            latencies.push_back(Clock::now() - t0);
        }
    }

    const auto elapsed = Clock::now() - start;
    ::close(fd);

    return {
        opsPerThread * 2,
        std::chrono::duration<double>(elapsed).count(),
        std::move(latencies)
    };
}

double percentile(std::vector<Nanos>& v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const std::size_t idx = static_cast<std::size_t>(p / 100.0 * static_cast<double>(v.size() - 1));
    return static_cast<double>(v[idx].count()) / 1000.0;  // ns → µs
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string host        = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t    port        = argc > 2 ? static_cast<uint16_t>(std::stoi(argv[2])) : 6380;
    const std::size_t totalOps    = argc > 3 ? std::stoul(argv[3]) : 100'000;
    const std::size_t threadCount = argc > 4 ? std::stoul(argv[4]) : 4;

    const std::size_t opsPerThread = totalOps / threadCount;

    std::cout << "miniredis benchmark\n"
              << "  host=" << host << " port=" << port
              << " ops=" << totalOps << " threads=" << threadCount << "\n\n";

    std::vector<std::thread> threads;
    std::vector<Result>      results(threadCount);

    const auto wallStart = Clock::now();

    for (std::size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t] {
            results[t] = runWorker(host, port, opsPerThread);
        });
    }
    for (auto& th : threads) th.join();

    const double wallSec =
        std::chrono::duration<double>(Clock::now() - wallStart).count();

    // Aggregate latencies across all threads.
    std::vector<Nanos> allLatencies;
    std::size_t totalCompleted = 0;
    for (auto& r : results) {
        totalCompleted += r.ops;
        allLatencies.insert(allLatencies.end(),
                            r.latencies.begin(), r.latencies.end());
    }

    const double throughput = static_cast<double>(totalCompleted) / wallSec;
    const double p50        = percentile(allLatencies, 50.0);
    const double p99        = percentile(allLatencies, 99.0);
    const double p999       = percentile(allLatencies, 99.9);

    std::cout << "Results\n"
              << "  completed ops : " << totalCompleted << "\n"
              << "  wall time     : " << wallSec        << " s\n"
              << "  throughput    : " << static_cast<std::size_t>(throughput) << " ops/s\n"
              << "  latency p50   : " << p50  << " µs\n"
              << "  latency p99   : " << p99  << " µs\n"
              << "  latency p99.9 : " << p999 << " µs\n";
}
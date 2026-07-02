# MiniRedis

A Redis-inspired in-memory key-value cache server built incrementally in modern **C++20** to learn systems programming, networking, concurrency, and backend engineering concepts.

**Current Status: Phase 8 Complete**

## Features

Implemented so far:

* TCP listener using POSIX sockets
* Persistent client connections
* Fixed-size thread pool
* Concurrent client handling
* **Sharded** thread-safe in-memory key-value store
* Lock striping with per-shard mutexes
* Text-based command protocol
* O(1) per-shard LRU cache using `std::list` + `std::unordered_map`
* Configurable cache capacity
* Automatic Least Recently Used (LRU) eviction
* TTL (Time-To-Live) support
* Passive expiration on key access
* Background active expiration thread
* Lock-free server statistics
* Benchmark client for throughput and latency measurement
* Stress-testing and latency measurement scripts
* Commands:

  * `PING`
  * `SET`
  * `GET`
  * `DEL`
  * `EXPIRE`
  * `INFO`
  * `SIZE`
  * `SHARDS`

* GoogleTest unit tests

---

## Build

```bash
cmake -S . -B build
cmake --build build
```

---

## Run

```bash
./build/miniredis
```

The server listens on port **6380**.

Connect using:

```bash
nc localhost 6380
```

Multiple clients can connect simultaneously. Requests are handled by a fixed-size worker thread pool while keys are distributed across multiple shards using lock striping to reduce contention.

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## Benchmark

Run the server:

```bash
./build/miniredis
```

In another terminal:

```bash
./build/miniredis_bench 127.0.0.1 6380 100000 4
```

Example output:

```text
completed ops : 200000
throughput    : 33955 ops/s
latency p50   : 112 µs
latency p99   : 268 µs
latency p99.9 : 790 µs
```

Additional helper scripts:

```bash
chmod +x scripts/stress.sh scripts/latency.sh

./scripts/stress.sh
./scripts/latency.sh
```

> Note: The benchmark client (`miniredis_bench`) provides the primary throughput and latency measurements. The shell scripts are intended as lightweight helpers and may exhibit platform-dependent behavior on WSL due to `nc`/shell process overhead.

---

## Supported Commands

| Command | Response |
|----------|----------|
| `PING` | `PONG` |
| `SET key value` | `OK` |
| `GET key` | `VALUE <value>` or `NOT_FOUND` |
| `DEL key` | `DELETED` or `NOT_FOUND` |
| `EXPIRE key seconds` | `OK` or `NOT_FOUND` |
| `SIZE` | Number of keys |
| `INFO` | Server statistics |
| `SHARDS` | Per-shard statistics |
| Unknown command | `ERROR unknown command` |

---

## Example Session

```text
PING
PONG

SET session abc
OK

GET session
VALUE abc

EXPIRE session 5
OK

INFO

SIZE

SHARDS
```

---

## Current Architecture

```text
                     +----------------+
                     |  TcpListener   |
                     +-------+--------+
                             |
                        accept()
                             |
                             v
                     +----------------+
                     |  ThreadPool    |
                     | (4 workers)    |
                     +-------+--------+
                             |
                      worker thread
                             |
                             v
                     +----------------+
                     | serveClient()  |
                     +-------+--------+
                             |
                      CommandParser
                             |
                             v
                +-----------------------------+
                |        Sharded KvStore      |
                |     (16 independent shards) |
                +---------------+-------------+
                                |
          +---------------------+---------------------+
          |                     |                     |
          v                     v                     v
     +----------+          +----------+         +----------+
     | Shard 0  |   ...    | Shard 7  |   ...   | Shard15  |
     | Map      |          | Map      |         | Map      |
     | LRU      |          | LRU      |         | LRU      |
     | TTL      |          | TTL      |         | TTL      |
     | Mutex    |          | Mutex    |         | Mutex    |
     +----------+          +----------+         +----------+
                                ^
                                |
                    Background TTL Sweep Thread
                                |
                                v
                     Lock-free Statistics Module
```

---

## Repository Structure

```text
.
├── bench/
│   └── bench.cpp
├── include/
├── scripts/
│   ├── latency.sh
│   └── stress.sh
├── src/
├── tests/
├── CMakeLists.txt
└── README.md
```

---

## Roadmap

* [x] Phase 1 — TCP listener and persistent client connections
* [x] Phase 2 — Text protocol and key-value store (`SET`, `GET`, `DEL`)
* [x] Phase 3 — O(1) LRU cache and bounded capacity
* [x] Phase 4 — Thread pool and concurrent client handling
* [x] Phase 5 — Sharded key-value store and lock striping
* [x] Phase 6 — TTL expiration and background cleanup
* [x] Phase 7 — INFO and administrative commands
* [x] Phase 8 — Benchmarking and performance analysis
* [ ] Phase 9 — Append-only persistence (optional)
* [ ] Phase 10 — RESP protocol and request pipelining (optional)
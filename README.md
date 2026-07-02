# MiniRedis

A Redis-inspired in-memory key-value cache server built incrementally in modern **C++20** to learn systems programming, networking, concurrency, and backend engineering concepts.

**Current Status: Phase 6 Complete**

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
* Commands:

  * `SET`
  * `GET`
  * `DEL`
  * `EXPIRE`
* GoogleTest unit tests

## Build

```bash
cmake -S . -B build
cmake --build build
```

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

## Running Tests

```bash
cd build
ctest --output-on-failure
```

## Supported Commands

| Command | Response |
|----------|----------|
| `SET key value` | `OK` |
| `GET key` | `VALUE <value>` or `NOT_FOUND` |
| `DEL key` | `DELETED` or `NOT_FOUND` |
| `EXPIRE key seconds` | `OK` or `NOT_FOUND` |
| Unknown command | `ERROR unknown command` |

## Example Session

```text
SET session abc
OK

GET session
VALUE abc

EXPIRE session 5
OK

GET session
VALUE abc

# wait 5+ seconds

GET session
NOT_FOUND
```

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
```

## Roadmap

* [x] Phase 1 — TCP listener and persistent client connections
* [x] Phase 2 — Text protocol and key-value store (`SET`, `GET`, `DEL`)
* [x] Phase 3 — O(1) LRU cache and bounded capacity
* [x] Phase 4 — Thread pool and concurrent client handling
* [x] Phase 5 — Sharded key-value store and lock striping
* [x] Phase 6 — TTL expiration and background cleanup
* [ ] Phase 7 — INFO and administrative commands
* [ ] Phase 8 — Benchmarking and performance analysis
* [ ] Phase 9 — Append-only persistence (optional)
* [ ] Phase 10 — RESP protocol and request pipelining (optional)
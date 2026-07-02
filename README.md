# MiniRedis

A Redis-inspired in-memory key-value cache server built incrementally in modern **C++20** to learn systems programming, networking, concurrency, persistence, and backend engineering concepts.

**Current Status: Phase 10 Complete**

## Features

Implemented so far:

* TCP listener using POSIX sockets
* Persistent client connections
* Fixed-size thread pool
* Concurrent client handling
* **Sharded** thread-safe in-memory key-value store
* Lock striping with per-shard mutexes
* Dual protocol support:
  * Text protocol
  * RESP2 protocol (compatible with `redis-cli`)
* Automatic protocol detection per connection
* Request pipelining for RESP clients
* O(1) per-shard LRU cache using `std::list` + `std::unordered_map`
* Configurable cache capacity
* Automatic Least Recently Used (LRU) eviction
* TTL (Time-To-Live) support
* Passive expiration on key access
* Background active expiration thread
* Append-Only File (AOF) persistence
* Automatic replay of persisted commands on startup
* Lock-free server statistics
* Benchmark client for throughput and latency measurement
* Stress-testing and latency measurement scripts
* GoogleTest unit tests

### Supported Commands

* `PING`
* `SET`
* `GET`
* `DEL`
* `EXPIRE`
* `INFO`
* `SIZE`
* `SHARDS`

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

You can connect using either protocol.

### Text protocol

```bash
nc localhost 6380
```

### RESP2

```bash
redis-cli -p 6380
```

Multiple clients can connect simultaneously. Requests are processed by a fixed-size worker thread pool while keys are distributed across multiple shards using lock striping to reduce contention.

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

Helper scripts:

```bash
chmod +x scripts/stress.sh scripts/latency.sh

./scripts/stress.sh
./scripts/latency.sh
```

> Note: The benchmark client (`miniredis_bench`) provides the primary throughput and latency measurements. The helper scripts are intended for lightweight testing and may show platform-dependent behavior under WSL because of shell and `nc` overhead.

---

## Persistence (AOF)

MiniRedis implements a simple Append-Only File (AOF) persistence mechanism.

Every successful:

* `SET`
* `DEL`

operation is appended to `miniredis.aof`.

When the server starts, the AOF is replayed automatically to restore the previous in-memory state.

Current limitations:

* TTLs are **not persisted**.
* `EXPIRE` commands are intentionally ignored during replay.
* No AOF compaction (`BGREWRITEAOF`) yet.
* No configurable fsync policy.

---

## RESP2 Support

MiniRedis supports the Redis Serialization Protocol (RESP2).

Features:

* Compatible with `redis-cli`
* Automatic protocol detection
* Request pipelining
* Bulk strings
* Integer replies
* Error replies
* Null bulk replies
* Simple string replies

The original text protocol remains fully supported for manual testing using `nc`.

---

## Supported Commands

| Command | Text Response | RESP2 Response |
|----------|---------------|----------------|
| `PING` | `PONG` | `+PONG` |
| `SET key value` | `OK` | `+OK` |
| `GET key` | `VALUE <value>` | Bulk String |
| `DEL key` | `DELETED` / `NOT_FOUND` | Integer |
| `EXPIRE key seconds` | `OK` / `NOT_FOUND` | Integer |
| `SIZE` | Number of keys | Integer |
| `INFO` | Statistics | Bulk String |
| `SHARDS` | Per-shard statistics | Bulk String |
| Unknown command | Error | `-ERR` |

---

## Example Session (Text Protocol)

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

## Example Session (RESP2)

```bash
redis-cli -p 6380

127.0.0.1:6380> PING
PONG

127.0.0.1:6380> SET foo bar
OK

127.0.0.1:6380> GET foo
"bar"

127.0.0.1:6380> INFO
...
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
               +-------------+-------------+
               |                           |
        Text Protocol                 RESP2 Parser
        CommandParser                 (Pipelining)
               |                           |
               +-------------+-------------+
                             |
                             v
                +-----------------------------+
                |        Sharded KvStore      |
                |     (16 independent shards) |
                +---------------+-------------+
                                |
          +---------------------+----------------------+
          |                     |                      |
          v                     v                      v
     +----------+         +----------+          +----------+
     | Shard 0  |   ...   | Shard 7  |   ...    | Shard15  |
     | Map      |         | Map      |          | Map      |
     | LRU      |         | LRU      |          | LRU      |
     | TTL      |         | TTL      |          | TTL      |
     | Mutex    |         | Mutex    |          | Mutex    |
     +----------+         +----------+          +----------+
                                ^
                                |
                    Background TTL Sweep Thread
                                |
                                v
                     Lock-free Statistics Module
                                |
                                v
                     Append-Only File (AOF)
```

---

## Repository Structure

```text
.
├── bench/
│   └── bench.cpp
├── include/
│   ├── net/
│   ├── persistence/
│   ├── protocol/
│   ├── stats/
│   └── store/
├── scripts/
│   ├── latency.sh
│   └── stress.sh
├── src/
│   ├── net/
│   ├── persistence/
│   ├── protocol/
│   ├── stats/
│   └── store/
├── tests/
├── CMakeLists.txt
└── README.md
```

---

## Roadmap

- [x] Phase 1 — TCP listener and persistent client connections
- [x] Phase 2 — Text protocol and key-value store (`SET`, `GET`, `DEL`)
- [x] Phase 3 — O(1) LRU cache and bounded capacity
- [x] Phase 4 — Thread pool and concurrent client handling
- [x] Phase 5 — Sharded key-value store and lock striping
- [x] Phase 6 — TTL expiration and background cleanup
- [x] Phase 7 — INFO and administrative commands
- [x] Phase 8 — Benchmarking and performance analysis
- [x] Phase 9 — Append-only persistence
- [x] Phase 10 — RESP2 protocol and request pipelining

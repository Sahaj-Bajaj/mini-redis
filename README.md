# mini-redis

A Redis-inspired in-memory cache server written in modern C++20 from scratch.

No Boost, no Asio, no external networking libraries — raw POSIX sockets, a hand-rolled thread pool, sharded lock striping, LRU eviction, TTL expiration, RESP2 protocol compatibility, and append-only persistence.

---

## Features

| Feature | Detail |
|---|---|
| Protocol | RESP2 (redis-cli compatible) + plain-text (nc compatible), auto-detected per connection |
| Commands | SET, GET, DEL, EXPIRE, PING, SIZE, SHARDS, INFO |
| Concurrency | Fixed thread pool (4 workers), one persistent connection per client |
| Storage | 16-shard hash table with per-shard mutex (lock striping) |
| Eviction | Per-shard LRU via `std::list` + iterator map — O(1) get/set/evict |
| TTL | Passive expiry on access + active background sweep thread (1s interval) |
| Persistence | Append-only file (AOF) — replayed on startup to restore state |
| Stats | Lock-free atomic counters: hits, misses, total commands, expired keys |
| Testing | GoogleTest unit tests for store, parser, AOF, and RESP layers |

---

## Architecture

```
┌─────────────────────────────────────────────┐
│                   main.cpp                  │
│  TcpListener → ThreadPool → serveClient()   │
└────────────┬──────────────┬─────────────────┘
             │              │
     ┌───────▼──────┐  ┌────▼────────────────┐
     │ RespParser / │  │      KvStore         │
     │CommandParser │  │  16 shards           │
     └───────┬──────┘  │  per-shard mutex     │
             │         │  LRU list+map        │
     ┌───────▼──────┐  │  TTL + sweep thread  │
     │RespFormatter │  └──────────┬───────────┘
     └──────────────┘             │
                          ┌───────▼───────┐
                          │   AofWriter   │
                          │ (append-only) │
                          └───────────────┘
```

---

## Build

Requires: GCC/Clang with C++20, CMake 3.20+, Linux or WSL2.

```bash
cmake -S . -B build
cmake --build build
```

First configure downloads GoogleTest automatically (requires internet).

---

## Run

```bash
./build/miniredis
# miniredis listening on port 6380 (4 worker threads)
```

---

## Connect

**Plain text via nc:**
```bash
nc localhost 6380
SET name alice
GET name          # VALUE alice
EXPIRE name 10    # OK
PING              # PONG
INFO
```

**RESP2 via redis-cli:**
```bash
redis-cli -p 6380 set foo bar
redis-cli -p 6380 get foo
redis-cli -p 6380 info
```

---

## Commands

| Command | Response (text) | Response (RESP2) |
|---|---|---|
| `SET key value` | `OK` | `+OK` |
| `GET key` | `VALUE <v>` / `NOT_FOUND` | `$<n>\r\n<v>` / `$-1` |
| `DEL key` | `DELETED` / `NOT_FOUND` | `:1` / `:0` |
| `EXPIRE key secs` | `OK` / `NOT_FOUND` | `:1` / `:0` |
| `PING` | `PONG` | `+PONG` |
| `SIZE` | `SIZE <n>` | `:<n>` |
| `SHARDS` | `shard[0]:<n> ...` | bulk string |
| `INFO` | stats block | bulk string |

---

## Tests

```bash
cd build && ctest --output-on-failure
```

Covers: KvStore (set/get/del/LRU/eviction), CommandParser (all verbs, edge cases), AOF (replay, del, append-mode), RespParser (pipelining, incomplete frames), RespFormatter (all response types).

---

## Benchmark

```bash
# Terminal 1
./build/miniredis

# Terminal 2
./build/miniredis_bench 127.0.0.1 6380 100000 4
```

Sample output (WSL2 — native Linux is typically 2–3× higher):

```
completed ops : 100000
wall time     : 2.98 s
throughput    : 33,557 ops/s
latency p50   : 47 µs
latency p99   : 312 µs
latency p99.9 : 891 µs
```

Stress test (8 concurrent clients, correctness verified):
```bash
chmod +x scripts/stress.sh && ./scripts/stress.sh 8 500
```

---

## Project Structure

```
mini-redis/
├── include/
│   ├── net/           # Socket, TcpListener, ThreadPool
│   ├── protocol/      # Command, CommandParser, RespParser, RespFormatter
│   ├── store/         # KvStore (sharded LRU + TTL)
│   ├── stats/         # Lock-free atomic Stats
│   └── persistence/   # AofWriter, AofReader
├── src/               # Implementations (mirrors include/)
├── tests/             # GoogleTest suites
├── bench/             # Benchmark client
└── scripts/           # stress.sh, latency.sh
```

---

## Design Decisions

**Why 16 shards?**
Power-of-2 count enables bitmask dispatch (`hash & 15`) instead of modulo. 16 shards eliminates mutex contention across typical 4–8 core machines. Each shard owns its mutex, LRU list, and hash map independently.

**Why list + iterator map for LRU?**
`std::list::splice` moves a node to the front in O(1) without invalidating any iterators. Storing the iterator inside the map entry means GET promotion is O(1) with no list traversal.

**Why passive + active TTL expiry?**
Passive expiry (check on GET) is free but leaves dead keys in memory until accessed. Active expiry (background sweep every 1s) bounds memory growth. Redis uses both for the same reason.

**Why AOF instead of snapshots?**
AOF trades larger file size for simpler, safer writes — each mutation is one appended line. Snapshot consistency requires either a fork or a read lock across the entire store.

**Why RESP2 auto-detection?**
First byte `*` identifies RESP2; anything else is plain text. Detection is per-connection and one-time. Keeps `nc` and `redis-cli` both working without a config flag.

---

## Known Limitations

- TTL does not survive restarts (`steady_clock` has no epoch; fix: persist `EXPIREAT <unix_ms>`)
- AOF grows unboundedly (no compaction / `BGREWRITEAOF`)
- No authentication, TLS, or ACL
- No RESP3 support

---

## License

MIT

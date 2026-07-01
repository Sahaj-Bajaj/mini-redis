# MiniRedis

A Redis-inspired in-memory key-value cache server, built incrementally in modern C++20.

**Phase 3 Complete**

Implemented:

- TCP listener
- Persistent client connections
- Text-based command protocol
- Single-threaded in-memory key-value store
- O(1) LRU cache using `std::list` + `std::unordered_map`
- Configurable bounded cache capacity
- Automatic least recently used (LRU) eviction
- `SET`
- `GET`
- `DEL`
- GoogleTest unit tests

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

You can connect using:

```bash
nc localhost 6380
```

## Tests

```bash
cd build
ctest --output-on-failure
```

## Protocol

| Command | Response |
|---------|----------|
| `SET key value` | `OK` |
| `GET key` | `<value>` or `NULL` |
| `DEL key` | `1` (deleted) or `0` (not found) |
| Unknown command | `ERR unknown command` |

### Example Session

```text
SET name Sahaj
OK

GET name
Sahaj

DEL name
1

GET name
NULL
```

## Roadmap

- [x] Phase 1 -- TCP listener and single-client echo server
- [x] Phase 2 -- Text protocol, `SET`/`GET`/`DEL`, single-threaded key-value store
- [x] Phase 3 -- O(1) LRU eviction and bounded cache
- [ ] Phase 4 -- Thread pool and concurrent client handling
- [ ] Phase 5 -- Sharded key-value store and lock striping
- [ ] Phase 6 -- TTL expiration and background cleanup
- [ ] Phase 7 -- INFO and administrative commands
- [ ] Phase 8 -- Benchmarking and performance analysis
- [ ] Phase 9 -- Append-only persistence (optional)
- [ ] Phase 10 -- RESP protocol and request pipelining (optional)
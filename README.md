# MiniRedis

A Redis-inspired in-memory key-value cache server, built incrementally in modern C++20.

## Status

**Phase 1 complete:** TCP listener + single-client echo server.

## Build

    cmake -S . -B build
    cmake --build build

## Run

    ./build/miniredis

Listens on port 6380. Connect with `nc localhost 6380` — anything typed is echoed back.

## Roadmap

- [x] Phase 1 -- TCP listener, single-client echo
- [ ] Phase 2 -- text protocol, SET/GET/DEL, single-threaded KV store
- [ ] Phase 3 -- LRU eviction, bounded cache
- [ ] Phase 4 -- thread pool, concurrent clients
- [ ] Phase 5 -- sharded store, lock striping
- [ ] Phase 6 -- TTL, active/passive expiration
- [ ] Phase 7 -- INFO/admin commands
- [ ] Phase 8 -- benchmarking
- [ ] Phase 9 -- append-only persistence (optional)
- [ ] Phase 10 -- RESP protocol, pipelining (optional)
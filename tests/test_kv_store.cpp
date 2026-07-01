#include "store/KvStore.h"
#include <gtest/gtest.h>

using miniredis::store::KvStore;

TEST(KvStoreTest, SetAndGet) {
    KvStore store;
    store.set("name", "alice");
    EXPECT_EQ(store.get("name"), "alice");
}

TEST(KvStoreTest, GetMissingReturnsNullopt) {
    KvStore store;
    EXPECT_EQ(store.get("missing"), std::nullopt);
}

TEST(KvStoreTest, SetOverwritesExistingValue) {
    KvStore store;
    store.set("key", "first");
    store.set("key", "second");
    EXPECT_EQ(store.get("key"), "second");
}

TEST(KvStoreTest, DelExistingReturnsTrue) {
    KvStore store;
    store.set("key", "val");
    EXPECT_TRUE(store.del("key"));
    EXPECT_EQ(store.get("key"), std::nullopt);
}

TEST(KvStoreTest, DelMissingReturnsFalse) {
    KvStore store;
    EXPECT_FALSE(store.del("missing"));
}

TEST(KvStoreTest, SizeTracksEntries) {
    KvStore store;
    EXPECT_EQ(store.size(), 0u);
    store.set("a", "1");
    store.set("b", "2");
    EXPECT_EQ(store.size(), 2u);
    store.del("a");
    EXPECT_EQ(store.size(), 1u);
}

// ── LRU (per-shard) ───────────────────────────────────────────────────────────

TEST(KvStoreTest, EvictsWhenShardFull) {
    // 16-shard store, 16 total slots → 1 per shard.
    KvStore store(KvStore::kShardCount);
    store.set("a", "1");
    store.set("a2", "x");
    store.set("a", "2");
    EXPECT_EQ(store.get("a"), "2");
}

TEST(KvStoreTest, UnboundedNeverEvicts) {
    KvStore store(0);
    for (int i = 0; i < 1000; ++i) {
        store.set(std::to_string(i), "v");
    }
    EXPECT_EQ(store.size(), 1000u);
}

TEST(KvStoreTest, SizeNeverExceedsMax) {
    KvStore store(160);
    for (int i = 0; i < 500; ++i) {
        store.set(std::to_string(i), "v");
    }
    EXPECT_LE(store.size(), 160u);
}
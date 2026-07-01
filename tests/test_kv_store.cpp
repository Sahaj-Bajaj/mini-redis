#include "store/KvStore.h"

#include <gtest/gtest.h>

#include <string>

using miniredis::store::KvStore;

// ── Phase 2 tests ─────────────────────────────────────────────────────────────

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

// ── Phase 3 LRU tests ─────────────────────────────────────────────────────────

TEST(KvStoreTest, EvictsLruWhenFull) {
    KvStore store(2);

    store.set("a", "1");  // [a]
    store.set("b", "2");  // [b, a]
    store.set("c", "3");  // evicts a -> [c, b]

    EXPECT_EQ(store.get("a"), std::nullopt);
    EXPECT_EQ(store.get("b"), "2");
    EXPECT_EQ(store.get("c"), "3");
}

TEST(KvStoreTest, GetPromotesKeyToMru) {
    KvStore store(2);

    store.set("a", "1");  // [a]
    store.set("b", "2");  // [b, a]

    EXPECT_EQ(store.get("a"), "1");  // promotes a -> [a, b]

    store.set("c", "3");  // evicts b -> [c, a]

    EXPECT_EQ(store.get("b"), std::nullopt);
    EXPECT_EQ(store.get("a"), "1");
    EXPECT_EQ(store.get("c"), "3");
}

TEST(KvStoreTest, UpdateExistingKeyPromotesToMru) {
    KvStore store(2);

    store.set("a", "1");      // [a]
    store.set("b", "2");      // [b, a]
    store.set("a", "new");    // [a, b]
    store.set("c", "3");      // evicts b

    EXPECT_EQ(store.get("b"), std::nullopt);
    EXPECT_EQ(store.get("a"), "new");
    EXPECT_EQ(store.get("c"), "3");
}

TEST(KvStoreTest, UnboundedStoreNeverEvicts) {
    KvStore store(0);

    for (int i = 0; i < 1000; ++i) {
        store.set(std::to_string(i), "v");
    }

    EXPECT_EQ(store.size(), 1000u);
}

TEST(KvStoreTest, SizeNeverExceedsMax) {
    KvStore store(5);

    for (int i = 0; i < 20; ++i) {
        store.set(std::to_string(i), "v");
    }

    EXPECT_EQ(store.size(), 5u);
}
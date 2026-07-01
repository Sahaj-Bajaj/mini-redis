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

    store.set("key", "value");

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

    EXPECT_TRUE(store.del("a"));

    EXPECT_EQ(store.size(), 1u);
}
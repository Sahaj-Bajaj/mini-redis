#include "store/KvStore.h"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using miniredis::store::KvStore;

TEST(KvStoreTest, SetAndGet) {
    KvStore s;
    s.set("k", "v");
    EXPECT_EQ(s.get("k"), "v");
}

TEST(KvStoreTest, GetMissing) {
    KvStore s;
    EXPECT_EQ(s.get("x"), std::nullopt);
}

TEST(KvStoreTest, SetOverwrites) {
    KvStore s;
    s.set("k", "a");
    s.set("k", "b");
    EXPECT_EQ(s.get("k"), "b");
}

TEST(KvStoreTest, DelExisting) {
    KvStore s;
    s.set("k", "v");
    EXPECT_TRUE(s.del("k"));
    EXPECT_EQ(s.get("k"), std::nullopt);
}

TEST(KvStoreTest, DelMissing) {
    KvStore s;
    EXPECT_FALSE(s.del("x"));
}

TEST(KvStoreTest, SizeTracks) {
    KvStore s;
    s.set("a", "1");
    s.set("b", "2");
    EXPECT_EQ(s.size(), 2u);

    s.del("a");
    EXPECT_EQ(s.size(), 1u);
}

TEST(KvStoreTest, UnboundedNeverEvicts) {
    KvStore s(0);

    for (int i = 0; i < 1000; ++i) {
        s.set(std::to_string(i), "v");
    }

    EXPECT_EQ(s.size(), 1000u);
}

TEST(KvStoreTest, SizeNeverExceedsMax) {
    KvStore s(160);

    for (int i = 0; i < 500; ++i) {
        s.set(std::to_string(i), "v");
    }

    EXPECT_LE(s.size(), 160u);
}

// TTL ------------------------------------------------------------------------

TEST(KvStoreTest, ExpireOnMissingReturnsFalse) {
    KvStore s;
    EXPECT_FALSE(s.expire("missing", std::chrono::seconds(10)));
}

TEST(KvStoreTest, PassiveExpiryOnGet) {
    KvStore s;

    s.set("k", "v");
    s.expire("k", std::chrono::seconds(0));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(s.get("k"), std::nullopt);
}

TEST(KvStoreTest, ActiveSweepRemovesExpired) {
    KvStore s;

    s.set("k", "v");
    s.expire("k", std::chrono::seconds(0));

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    EXPECT_EQ(s.size(), 0u);
}

TEST(KvStoreTest, SetClearsTtl) {
    KvStore s;

    s.set("k", "v");
    s.expire("k", std::chrono::seconds(0));

    s.set("k", "v2");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(s.get("k"), "v2");
}
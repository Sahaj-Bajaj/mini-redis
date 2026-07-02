#include "persistence/AofReader.h"
#include "persistence/AofWriter.h"
#include "store/KvStore.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace miniredis;
static const std::string kPath = "/tmp/miniredis_test.aof";

class AofTest : public ::testing::Test {
    void TearDown() override { std::filesystem::remove(kPath); }
};

TEST_F(AofTest, ReplayRestoresSets) {
    { persistence::AofWriter w(kPath); w.writeSet("a","1"); w.writeSet("b","2"); }
    store::KvStore store;
    EXPECT_EQ(persistence::AofReader::replay(kPath, store), 2u);
    EXPECT_EQ(store.get("a"), "1");
    EXPECT_EQ(store.get("b"), "2");
}

TEST_F(AofTest, ReplayAppliesDel) {
    { persistence::AofWriter w(kPath); w.writeSet("a","1"); w.writeDel("a"); }
    store::KvStore store;
    (void)persistence::AofReader::replay(kPath, store);
    EXPECT_EQ(store.get("a"), std::nullopt);
}

TEST_F(AofTest, MissingFileReturnsZero) {
    store::KvStore store;
    EXPECT_EQ(persistence::AofReader::replay("/tmp/no_such.aof", store), 0u);
}

TEST_F(AofTest, AppendModePreservesData) {
    { persistence::AofWriter w(kPath); w.writeSet("a","1"); }
    { persistence::AofWriter w(kPath); w.writeSet("b","2"); }
    store::KvStore store;
    EXPECT_EQ(persistence::AofReader::replay(kPath, store), 2u);
}
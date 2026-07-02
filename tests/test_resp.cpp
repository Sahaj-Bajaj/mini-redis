#include "protocol/RespFormatter.h"
#include "protocol/RespParser.h"
#include "protocol/Command.h"
#include <gtest/gtest.h>

using miniredis::protocol::RespFormatter;
using miniredis::protocol::RespParser;
using miniredis::protocol::CommandType;

TEST(RespParserTest, ParsesSet) {
    std::string buf = "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    auto cmds = RespParser::parse(buf);
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].type, CommandType::Set);
    EXPECT_EQ(cmds[0].args[0], "foo");
    EXPECT_EQ(cmds[0].args[1], "bar");
    EXPECT_TRUE(buf.empty());
}

TEST(RespParserTest, ParsesGet) {
    std::string buf = "*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n";
    auto cmds = RespParser::parse(buf);
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].type, CommandType::Get);
}

TEST(RespParserTest, ParsesPing) {
    std::string buf = "*1\r\n$4\r\nPING\r\n";
    auto cmds = RespParser::parse(buf);
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].type, CommandType::Ping);
}

TEST(RespParserTest, PipeliningTwoCommands) {
    std::string buf =
        "*3\r\n$3\r\nSET\r\n$1\r\na\r\n$1\r\n1\r\n"
        "*2\r\n$3\r\nGET\r\n$1\r\na\r\n";
    auto cmds = RespParser::parse(buf);
    EXPECT_EQ(cmds.size(), 2u);
    EXPECT_TRUE(buf.empty());
}

TEST(RespParserTest, IncompleteCommandLeftInBuffer) {
    std::string buf = "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n";
    auto cmds = RespParser::parse(buf);
    EXPECT_TRUE(cmds.empty());
    EXPECT_FALSE(buf.empty());
}

TEST(RespFormatterTest, Ok)         { EXPECT_EQ(RespFormatter::ok(),                "+OK\r\n"); }
TEST(RespFormatterTest, Pong)       { EXPECT_EQ(RespFormatter::pong(),              "+PONG\r\n"); }
TEST(RespFormatterTest, NullBulk)   { EXPECT_EQ(RespFormatter::nullBulk(),          "$-1\r\n"); }
TEST(RespFormatterTest, Integer)    { EXPECT_EQ(RespFormatter::integer(7),          ":7\r\n"); }
TEST(RespFormatterTest, BulkString) { EXPECT_EQ(RespFormatter::bulkString("hello"), "$5\r\nhello\r\n"); }
TEST(RespFormatterTest, Error)      { EXPECT_EQ(RespFormatter::error("oops"),       "-ERR oops\r\n"); }
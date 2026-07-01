#include "protocol/Command.h"
#include "protocol/CommandParser.h"

#include <gtest/gtest.h>

using miniredis::protocol::CommandParser;
using miniredis::protocol::CommandType;

TEST(CommandParserTest, ParseSet) {
    auto c = CommandParser::parse("SET k v");
    EXPECT_EQ(c.type, CommandType::Set);
    EXPECT_EQ(c.args[1], "v");
}

TEST(CommandParserTest, SetMultiWordValue) {
    auto c = CommandParser::parse("SET k hi bob");
    EXPECT_EQ(c.args[1], "hi bob");
}

TEST(CommandParserTest, ParseGet) {
    auto c = CommandParser::parse("GET k");
    EXPECT_EQ(c.type, CommandType::Get);
}

TEST(CommandParserTest, ParseDel) {
    auto c = CommandParser::parse("DEL k");
    EXPECT_EQ(c.type, CommandType::Del);
}

TEST(CommandParserTest, ParseExpire) {
    auto c = CommandParser::parse("EXPIRE k 10");
    EXPECT_EQ(c.type, CommandType::Expire);
    EXPECT_EQ(c.args[1], "10");
}

TEST(CommandParserTest, CaseInsensitive) {
    EXPECT_EQ(
        CommandParser::parse("expire k 5").type,
        CommandType::Expire);
}

TEST(CommandParserTest, UnknownVerb) {
    EXPECT_EQ(
        CommandParser::parse("PING").type,
        CommandType::Unknown);
}

TEST(CommandParserTest, EmptyLine) {
    EXPECT_EQ(
        CommandParser::parse("").type,
        CommandType::Unknown);
}

TEST(CommandParserTest, ExpireWrongArgCount) {
    EXPECT_EQ(
        CommandParser::parse("EXPIRE k").type,
        CommandType::Unknown);
}
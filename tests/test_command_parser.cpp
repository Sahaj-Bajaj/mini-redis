#include "protocol/Command.h"
#include "protocol/CommandParser.h"

#include <gtest/gtest.h>

using miniredis::protocol::CommandParser;
using miniredis::protocol::CommandType;

TEST(CommandParserTest, ParseSet) {
    EXPECT_EQ(CommandParser::parse("SET k v").type, CommandType::Set);
}

TEST(CommandParserTest, SetMultiWordValue) {
    EXPECT_EQ(CommandParser::parse("SET k hi bob").args[1], "hi bob");
}

TEST(CommandParserTest, ParseGet) {
    EXPECT_EQ(CommandParser::parse("GET k").type, CommandType::Get);
}

TEST(CommandParserTest, ParseDel) {
    EXPECT_EQ(CommandParser::parse("DEL k").type, CommandType::Del);
}

TEST(CommandParserTest, ParseExpire) {
    EXPECT_EQ(CommandParser::parse("EXPIRE k 10").type, CommandType::Expire);
}

TEST(CommandParserTest, ParsePing) {
    EXPECT_EQ(CommandParser::parse("PING").type, CommandType::Ping);
}

TEST(CommandParserTest, ParseSize) {
    EXPECT_EQ(CommandParser::parse("SIZE").type, CommandType::Size);
}

TEST(CommandParserTest, ParseShards) {
    EXPECT_EQ(CommandParser::parse("SHARDS").type, CommandType::Shards);
}

TEST(CommandParserTest, ParseInfo) {
    EXPECT_EQ(CommandParser::parse("INFO").type, CommandType::Info);
}

TEST(CommandParserTest, CaseInsensitive) {
    EXPECT_EQ(CommandParser::parse("ping").type, CommandType::Ping);
}

TEST(CommandParserTest, UnknownVerb) {
    EXPECT_EQ(CommandParser::parse("BADCMD").type, CommandType::Unknown);
}

TEST(CommandParserTest, EmptyLine) {
    EXPECT_EQ(CommandParser::parse("").type, CommandType::Unknown);
}

TEST(CommandParserTest, ExpireWrongArgCount) {
    EXPECT_EQ(CommandParser::parse("EXPIRE k").type, CommandType::Unknown);
}
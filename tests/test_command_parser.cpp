#include "protocol/Command.h"
#include "protocol/CommandParser.h"

#include <gtest/gtest.h>

using miniredis::protocol::CommandParser;
using miniredis::protocol::CommandType;

TEST(CommandParserTest, ParseSet) {
    auto command = CommandParser::parse("SET name alice");

    EXPECT_EQ(command.type, CommandType::Set);

    ASSERT_EQ(command.args.size(), 2u);

    EXPECT_EQ(command.args[0], "name");
    EXPECT_EQ(command.args[1], "alice");
}

TEST(CommandParserTest, ParseGet) {
    auto command = CommandParser::parse("GET name");

    EXPECT_EQ(command.type, CommandType::Get);

    ASSERT_EQ(command.args.size(), 1u);

    EXPECT_EQ(command.args[0], "name");
}

TEST(CommandParserTest, ParseDel) {
    auto command = CommandParser::parse("DEL name");

    EXPECT_EQ(command.type, CommandType::Del);

    ASSERT_EQ(command.args.size(), 1u);

    EXPECT_EQ(command.args[0], "name");
}

TEST(CommandParserTest, LowercaseCommands) {
    EXPECT_EQ(CommandParser::parse("set a b").type,
              CommandType::Set);

    EXPECT_EQ(CommandParser::parse("get a").type,
              CommandType::Get);

    EXPECT_EQ(CommandParser::parse("del a").type,
              CommandType::Del);
}

TEST(CommandParserTest, MultiWordValue) {
    auto command =
        CommandParser::parse("SET city New Delhi");

    EXPECT_EQ(command.type, CommandType::Set);

    ASSERT_EQ(command.args.size(), 2u);

    EXPECT_EQ(command.args[1], "New Delhi");
}

TEST(CommandParserTest, UnknownCommand) {
    EXPECT_EQ(CommandParser::parse("PING").type,
              CommandType::Unknown);
}

TEST(CommandParserTest, EmptyCommand) {
    EXPECT_EQ(CommandParser::parse("").type,
              CommandType::Unknown);
}

TEST(CommandParserTest, InvalidArgumentCount) {
    EXPECT_EQ(CommandParser::parse("GET").type,
              CommandType::Unknown);

    EXPECT_EQ(CommandParser::parse("SET key").type,
              CommandType::Unknown);

    EXPECT_EQ(CommandParser::parse("DEL").type,
              CommandType::Unknown);

    EXPECT_EQ(CommandParser::parse("GET a b").type,
              CommandType::Unknown);
}
#include <gtest/gtest.h>
#include "detail/sql_parser.hpp"

TEST(ParserTest, SimpleParse) {
    auto result = SqlParser::Parse("SELECT * FROM t", {});
    ASSERT_TRUE(result.ok());
}

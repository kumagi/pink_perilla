#include <gtest/gtest.h>

#include "pink_perilla.hpp"
#include "table_definition.hpp"

TEST(TableDefinition, SelectFromDefinedTable) {
    std::vector<pink_perilla::TableDefinition> table_defs = {
        {"users",
         {{"id", pink_perilla::DataType::kI64, false},
          {"name", pink_perilla::DataType::kString, true}}}};

    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT id, name FROM users", table_defs);

    ASSERT_TRUE(plan.ok());
}

TEST(TableDefinition, SelectFromUndefinedTable) {
    std::vector<pink_perilla::TableDefinition> table_defs = {
        {"users",
         {{"id", pink_perilla::DataType::kI64, false},
          {"name", pink_perilla::DataType::kString, true}}}};

    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT id, name FROM undefined_table", table_defs);

    ASSERT_FALSE(plan.ok());
    EXPECT_EQ(plan.status().code(), absl::StatusCode::kNotFound);
    EXPECT_EQ(plan.status().message(), "Table not found: undefined_table");
}

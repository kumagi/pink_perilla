#include <gtest/gtest.h>
#include <google/protobuf/text_format.h>
#include "pink_perilla.hpp"
#include "substrait/plan.pb.h"

namespace {
void AssertPlanEqualsPrototext(const substrait::Plan& plan, const std::string& expected_prototext) {
    std::string actual_prototext;
    google::protobuf::TextFormat::PrintToString(plan, &actual_prototext);
    ASSERT_EQ(actual_prototext, expected_prototext);
}
} // namespace

TEST(Select, SimplestQuery) {
    std::string sql = "SELECT name FROM employees";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      project {
        input {
          read {
            named_table {
              names: "employees"
            }
          }
        }
        expressions {
          literal {
            string: "name"
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Select, DISABLED_WindowFunction) {
    std::string sql =
        "SELECT row_number() OVER (PARTITION BY col1, col2) FROM my_table";
    substrait::Plan plan = pink_perilla::Parse(sql);

    ASSERT_EQ(plan.relations_size(), 1);
    const substrait::RelRoot &root_rel = plan.relations(0).root();

    ASSERT_TRUE(root_rel.input().has_window());
    const substrait::ConsistentPartitionWindowRel &window_rel =
        root_rel.input().window();

    ASSERT_EQ(window_rel.partition_expressions_size(), 2);

    const substrait::Expression &part_expr_1 =
        window_rel.partition_expressions(0);
    ASSERT_TRUE(part_expr_1.has_literal());
    EXPECT_EQ(part_expr_1.literal().string(), "col1");

    const substrait::Expression &part_expr_2 =
        window_rel.partition_expressions(1);
    ASSERT_TRUE(part_expr_2.has_literal());
    EXPECT_EQ(part_expr_2.literal().string(), "col2");

    ASSERT_EQ(window_rel.window_functions_size(), 1);
    const substrait::ConsistentPartitionWindowRel::WindowRelFunction &win_func =
        window_rel.window_functions(0);
    EXPECT_EQ(win_func.function_reference(), 0);
}

TEST(Select, NestedQuery) {
    std::string sql =
        "SELECT my_col FROM (SELECT col1 AS my_col FROM my_table)";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      project {
        input {
          project {
            input {
              read {
                named_table {
                  names: "my_table"
                }
              }
            }
            expressions {
              literal {
                string: "col1 AS my_col"
              }
            }
          }
        }
        expressions {
          literal {
            string: "my_col"
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Select, DISABLED_CorrelatedSubquery) {
    std::string sql =
        "SELECT name FROM employees e1 WHERE e1.salary > (SELECT AVG(salary) "
        "FROM employees e2 WHERE e2.department = e1.department)";
    substrait::Plan plan = pink_perilla::Parse(sql);
    // This test is disabled as it requires a much more advanced parser.
    FAIL();
}

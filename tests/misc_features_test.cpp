#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Misc, OrderBy) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT * FROM my_table ORDER BY col1 ASC, col2 DESC");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          sort {
                            input { read { named_table { names: "my_table" } } }
                            sorts {
                              expr { literal { string: "col1" } }
                              direction: SORT_DIRECTION_ASC_NULLS_FIRST
                            }
                            sorts {
                              expr { literal { string: "col2" } }
                              direction: SORT_DIRECTION_DESC_NULLS_FIRST
                            }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Misc, Limit) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT * FROM my_table LIMIT 10");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          fetch {
                            input { read { named_table { names: "my_table" } } }
                            count_expr { literal { i64: 10 } }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(MiscFeatures, GroupBy) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT department, SUM(salary) FROM employees GROUP BY department");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       aggregate {
                         input { read { named_table { names: "employees" } } }
                         groupings { expression_references: 0 }
                         measures { measure {} }
                         grouping_expressions {
                           literal { string: "department" }
                         }
                       }
                     }
                     expressions { literal { string: "department" } }
                     expressions { literal { string: "SUM(salary)" } }
                   }
                 }
               }
             }
        )pb");
}

TEST(MiscFeatures, AggregateFunctions) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT department, COUNT(id), SUM(salary), MAX(salary), MIN(salary) "
        "FROM employees GROUP BY department");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       aggregate {
                         input { read { named_table { names: "employees" } } }
                         groupings { expression_references: 0 }
                         measures { measure {} }
                         measures { measure {} }
                         measures { measure {} }
                         measures { measure {} }
                         grouping_expressions {
                           literal { string: "department" }
                         }
                       }
                     }
                     expressions { literal { string: "department" } }
                     expressions { literal { string: "COUNT(id)" } }
                     expressions { literal { string: "SUM(salary)" } }
                     expressions { literal { string: "MAX(salary)" } }
                     expressions { literal { string: "MIN(salary)" } }
                   }
                 }
               }
             }
        )pb");
}

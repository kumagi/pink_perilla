#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "google/protobuf/util/message_differencer.h"
#include "pink_perilla.hpp"
#include "substrait/plan.pb.h"
#include "proto_tools.h"

TEST(Select, SimplestQuery) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse("SELECT name FROM employees");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input { read { named_table { names: "employees" } } }
                     expressions { literal { string: "name" } }
                   }
                 }
               }
             }
        )pb");
}

TEST(Select, NestedQuery) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT my_col FROM (SELECT col1 AS my_col FROM my_table)");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       project {
                         input { read { named_table { names: "my_table" } } }
                         expressions { literal { string: "col1 AS my_col" } }
                       }
                     }
                     expressions { literal { string: "my_col" } }
                   }
                 }
               }
             }
        )pb");
}

TEST(Select, WhereCondition) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT name FROM employees WHERE salary > 500;");

    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       filter {
                         input { read { named_table { names: "employees" } } }
                         condition {
                           scalar_function {
                             arguments {
                               value { literal { string: "salary > 500" } }
                             }
                           }
                         }
                       }
                     }
                     expressions { literal { string: "name" } }
                   }
                 }
               }
             }
        )pb");
}

TEST(Select, DISABLED_WindowFunction) {
    const absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse("SELECT row_number() OVER (PARTITION BY col1, col2) FROM my_table");

    ASSERT_TRUE(plan.ok());
    FAIL();
}

TEST(Select, DISABLED_CorrelatedSubquery) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT name FROM employees e1 WHERE e1.salary > (SELECT AVG(salary) "
        "FROM employees e2 WHERE e2.department = e1.department)");
    ;
    // This test is disabled as it requires a much more advanced parser.
    FAIL();
}

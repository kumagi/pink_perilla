#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Robustness, ExtraWhitespace) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT\t*\tFROM  my_table ");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root { input { read { named_table { names: "my_table" } } } }
             }
        )pb");
}

TEST(Robustness, SingleLineComment) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT * -- select all columns\nFROM my_table;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root { input { read { named_table { names: "my_table" } } } }
             }
        )pb");
}

TEST(Robustness, MultiLineComment) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT * FROM /* this is a multi-line comment */ my_table");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,

        R"pb(relations {
               root { input { read { named_table { names: "my_table" } } } }
             }
        )pb");
}

TEST(Robustness, Mixed) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT col1, -- select column 1\n col2 /* another comment */ "
        "FROM\nmy_table;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          project {
                            input { read { named_table { names: "my_table" } } }
                            expressions { literal { string: "col1" } }
                            expressions { literal { string: "col2" } }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Robustness, MultiLines) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(R"SQL(
        SELECT
            c1
        FROM
            my_table
        WHERE
            c2 > 10;
        )SQL");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       filter {
                         input {
                           read { named_table { names: "my_table" } }
                         }
                         condition {
                           scalar_function {
                             arguments {
                               value { literal { string: "c2 > 10" } }
                             }
                           }
                         }
                       }
                     }
                     expressions { literal { string: "c1" } }
                   }
                 }
               }
             }
        )pb");
}

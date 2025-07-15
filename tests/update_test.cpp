#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Update, BasicUpdate) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "UPDATE users SET name = 'Bob' WHERE id = 101;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   update {
                     named_table { names: "users" }
                     condition {
                       scalar_function {
                         arguments {
                           value { literal { string: "id = 101" } }
                         }
                       }
                     }
                     transformations {
                       transformation { literal { string: "Bob" } }
                     }
                   }
                 }
               }
             }
        )pb");
}

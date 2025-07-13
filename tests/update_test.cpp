#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Update, BasicUpdate) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "UPDATE my_table SET col1 = 456 WHERE col2 = 'hello';");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   update {
                     named_table { names: "my_table" }
                     condition {
                       scalar_function {
                         arguments {
                           value { literal { string: "col2 = \'hello\'" } }
                         }
                       }
                     }
                     transformations { transformation { literal { i32: 456 } } }
                   }
                 }
               }
             }
        )pb");
}

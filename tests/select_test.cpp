#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "google/protobuf/util/message_differencer.h"
#include "pink_perilla.hpp"
#include "substrait/plan.pb.h"
#include "proto_tools.h"

TEST(Select, SelectWithWhere) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT a, b FROM table WHERE x > 0;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       filter {
                         input {
                           read { named_table { names: "table" } }
                         }
                         condition {
                           scalar_function {
                             arguments {
                               value { literal { string: "x > 0" } }
                             }
                           }
                         }
                       }
                     }
                     expressions { literal { string: "a" } }
                     expressions { literal { string: "b" } }
                   }
                 }
               }
             }
        )pb");
}

#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"
#include "proto_tools.h"

TEST(Delete, BasicDelete) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("DELETE FROM users WHERE id = 101;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   write {
                     named_table { names: "users" }
                     op: WRITE_OP_DELETE
                     input {
                       filter {
                         input {
                           read { named_table { names: "users" } }
                         }
                         condition {
                           scalar_function {
                             arguments {
                               value { literal { string: "id = 101" } }
                             }
                           }
                         }
                       }
                     }
                   }
                 }
               }
             }
        )pb");
}

#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Insert, BasicInsert) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "INSERT INTO users (id, name) VALUES (101, 'Alice');");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
        R"pb(relations {
               root {
                 input {
                   write {
                     named_table { names: "users" }
                     input {
                       read {
                         base_schema {
                           names: "id"
                           names: "name"
                         }
                         virtual_table {
                           expressions {
                             fields {
                               literal { i32: 101 }
                             }
                             fields {
                               literal { string: "Alice" }
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

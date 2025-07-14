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
                 project {
                   common { emit: [ 0, 1 ] }
                   input {
                     filter {
                       condition {
                         scalar_function {
                           function_reference: 1
                           output_type { bool { nullability: NULLABILITY_NULLABLE } }
                           arguments {
                             value {
                               selection {
                                 direct_reference { struct_field { field: 2 } }
                                 root_reference {}
                               }
                             }
                           }
                           arguments {
                             value { literal { i32: 0 } }
                           }
                         }
                       }
                       input {
                         read {
                           base_schema {
                             names: "a"
                             names: "b"
                             names: "x"
                             struct_ {
                               types { i32 {} }
                               types { i32 {} }
                               types { i32 {} }
                             }
                           }
                           named_table { names: "table" }
                         }
                       }
                     }
                   }
                   expressions {
                     selection {
                       direct_reference { struct_field { field: 0 } }
                       root_reference {}
                     }
                   }
                   expressions {
                     selection {
                       direct_reference { struct_field { field: 1 } }
                       root_reference {}
                     }
                   }
                 }
               }
             }
             extension_uris {
               extension_uri_anchor: 1
               uri: "https://github.com/substrait-io/substrait/blob/main/"
                    "extensions/functions_arithmetic.yaml"
             }
             extensions {
               extension_function {
                 extension_uri_reference: 1
                 function_anchor: 1
                 name: "gt:i32_i32"
               }
             })pb");
}

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
                 write {
                   named_table { names: "users" }
                   input {
                     project {
                       common { emit: [ 0, 2 ] }
                       input {
                         filter {
                           condition {
                             scalar_function {
                               function_reference: 1
                               output_type {
                                 bool { nullability: NULLABILITY_NULLABLE }
                               }
                               arguments {
                                 value {
                                   selection {
                                     direct_reference {
                                       struct_field { field: 0 }
                                     }
                                     root_reference {}
                                   }
                                 }
                               }
                               arguments {
                                 value { literal { i32: 101 } }
                               }
                             }
                           }
                           input {
                             read {
                               base_schema {
                                 names: "id"
                                 names: "name"
                                 struct_ {
                                   types { i32 {} }
                                   types { string {} }
                                 }
                               }
                               named_table { names: "users" }
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
                       expressions { literal { string: "Bob" } }
                     }
                   }
                 }
               }
             }
             extension_uris {
               extension_uri_anchor: 1
               uri: "https://github.com/substrait-io/substrait/blob/main/"
                    "extensions/functions_comparison.yaml"
             }
             extensions {
               extension_function {
                 extension_uri_reference: 1
                 function_anchor: 1
                 name: "equal:i32_i32"
               }
             })pb");
}

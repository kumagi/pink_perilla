#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(DDL, CreateTable) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("CREATE TABLE users (id INT, name VARCHAR(255));");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   ddl {
                     named_object { names: "users" }
                     table_schema {
                       names: "id"
                       names: "name"
                       struct {
                         types { string { nullability: NULLABILITY_NULLABLE } }
                         types { string { nullability: NULLABILITY_NULLABLE } }
                       }
                     }
                     object: DDL_OBJECT_TABLE
                     op: DDL_OP_CREATE
                   }
                 }
               }
             }
        )pb");
}

TEST(DDL, DropTable) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse("DROP TABLE users;");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
        R"pb(relations {
               root {
                 input {
                   ddl {
                     named_object { names: "users" }
                     object: DDL_OBJECT_TABLE
                     op: DDL_OP_DROP
                   }
                 }
               }
             }
        )pb");
}

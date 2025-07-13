#include <gtest/gtest.h>
#include <google/protobuf/text_format.h>
#include "pink_perilla.hpp"
#include "substrait/plan.pb.h"

namespace {
void AssertPlanEqualsPrototext(const substrait::Plan& plan, const std::string& expected_prototext) {
    std::string actual_prototext;
    google::protobuf::TextFormat::PrintToString(plan, &actual_prototext);
    ASSERT_EQ(actual_prototext, expected_prototext);
}
} // namespace

TEST(DDL, CreateTable) {
    std::string sql = "CREATE TABLE users (id INT, name VARCHAR(255));";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      ddl {
        named_object {
          names: "users"
        }
        table_schema {
          names: "id"
          names: "name"
          struct {
            types {
              string {
                nullability: NULLABILITY_NULLABLE
              }
            }
            types {
              string {
                nullability: NULLABILITY_NULLABLE
              }
            }
          }
        }
        object: DDL_OBJECT_TABLE
        op: DDL_OP_CREATE
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(DDL, DropTable) {
    std::string sql = "DROP TABLE users;";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      ddl {
        named_object {
          names: "users"
        }
        object: DDL_OBJECT_TABLE
        op: DDL_OP_DROP
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}


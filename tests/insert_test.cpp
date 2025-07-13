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

TEST(Insert, BasicInsert) {
    std::string sql =
        "INSERT INTO my_table (col1, col2) VALUES (123, 'hello');";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      write {
        named_table {
          names: "my_table"
        }
        input {
          read {
            base_schema {
              names: "col1"
              names: "col2"
            }
            virtual_table {
              expressions {
                fields {
                  literal {
                    i32: 123
                  }
                }
                fields {
                  literal {
                    string: "hello"
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

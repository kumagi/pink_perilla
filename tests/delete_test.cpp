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

TEST(Delete, BasicDelete) {
    std::string sql = "DELETE FROM my_table WHERE col1 = 123;";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      write {
        named_table {
          names: "my_table"
        }
        op: WRITE_OP_DELETE
        input {
          filter {
            input {
              read {
                named_table {
                  names: "my_table"
                }
              }
            }
            condition {
              scalar_function {
                arguments {
                  value {
                    literal {
                      string: "col1 = 123"
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
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

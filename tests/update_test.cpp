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

TEST(Update, BasicUpdate) {
    std::string sql = "UPDATE my_table SET col1 = 456 WHERE col2 = 'hello';";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      update {
        named_table {
          names: "my_table"
        }
        condition {
          scalar_function {
            arguments {
              value {
                literal {
                  string: "col2 = \'hello\'"
                }
              }
            }
          }
        }
        transformations {
          transformation {
            literal {
              i32: 456
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

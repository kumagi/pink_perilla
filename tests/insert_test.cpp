#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Insert, BasicInsert) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "INSERT INTO my_table (col1, col2) VALUES (123, 'hello');");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          write {
                            named_table { names: "my_table" }
                            input {
                              read {
                                base_schema { names: "col1" names: "col2" }
                                virtual_table {
                                  expressions {
                                    fields { literal { i32: 123 } }
                                    fields { literal { string: "hello" } }
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

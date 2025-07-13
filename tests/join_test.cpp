#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/debugging/stacktrace.h"
#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Join, CrossJoin) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT * FROM t1, t2");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          cross {
                            left { read { named_table { names: "t1" } } }
                            right { read { named_table { names: "t2" } } }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Join, InnerJoin) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          join {
                            left { read { named_table { names: "t1" } } }
                            right { read { named_table { names: "t2" } } }
                            expression {
                              scalar_function {
                                arguments {
                                  value { literal { string: "t1.id = t2.id" } }
                                }
                              }
                            }
                            type: JOIN_TYPE_INNER
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Join, LeftOuterJoin) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT * FROM t1 LEFT OUTER JOIN t2 ON t1.id = t2.id");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          join {
                            left { read { named_table { names: "t1" } } }
                            right { read { named_table { names: "t2" } } }
                            expression {
                              scalar_function {
                                arguments {
                                  value { literal { string: "t1.id = t2.id" } }
                                }
                              }
                            }
                            type: JOIN_TYPE_LEFT
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Join, FourTableJoin) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id INNER JOIN t3 ON "
        "t2.id = t3.id INNER JOIN t4 ON t3.id = t4.id");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   join {
                     left {
                       join {
                         left {
                           join {
                             left { read { named_table { names: "t1" } } }
                             right { read { named_table { names: "t2" } } }
                             expression {
                               scalar_function {
                                 arguments {
                                   value { literal { string: "t1.id = t2.id" } }
                                 }
                               }
                             }
                             type: JOIN_TYPE_INNER
                           }
                         }
                         right { read { named_table { names: "t3" } } }
                         expression {
                           scalar_function {
                             arguments {
                               value { literal { string: "t2.id = t3.id" } }
                             }
                           }
                         }
                         type: JOIN_TYPE_INNER
                       }
                     }
                     right { read { named_table { names: "t4" } } }
                     expression {
                       scalar_function {
                         arguments {
                           value { literal { string: "t3.id = t4.id" } }
                         }
                       }
                     }
                     type: JOIN_TYPE_INNER
                   }
                 }
               }
             }
        )pb");
}

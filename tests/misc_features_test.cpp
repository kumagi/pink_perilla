#include <google/protobuf/text_format.h>
#include <gtest/gtest.h>

#include "absl/status/statusor.h"
#include "pink_perilla.hpp"
#include "proto_tools.h"
#include "substrait/plan.pb.h"

TEST(Misc, OrderBy) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT * FROM my_table ORDER BY col1 ASC, col2 DESC");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          sort {
                            input { read { named_table { names: "my_table" } } }
                            sorts {
                              expr { literal { string: "col1" } }
                              direction: SORT_DIRECTION_ASC_NULLS_FIRST
                            }
                            sorts {
                              expr { literal { string: "col2" } }
                              direction: SORT_DIRECTION_DESC_NULLS_FIRST
                            }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(Misc, Limit) {
    absl::StatusOr<substrait::Plan> plan =
        pink_perilla::Parse("SELECT * FROM users LIMIT 10");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(*plan,
               R"pb(relations {
                      root {
                        input {
                          fetch {
                            input {
                              read {
                                named_table {
                                  names: "users"
                                }
                              }
                            }
                            count_expr {
                              literal {
                                i64: 10
                              }
                            }
                          }
                        }
                      }
                    }
               )pb");
}

TEST(MiscFeatures, GroupByWithCount) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT department, COUNT(employee_id) FROM employees GROUP BY department");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       aggregate {
                         input {
                           read {
                             named_table {
                               names: "employees"
                             }
                           }
                         }
                         groupings {
                           expression_references: 0
                         }
                         measures {
                           measure {
                           }
                         }
                         grouping_expressions {
                           literal {
                             string: "department"
                           }
                         }
                       }
                     }
                     expressions {
                       literal {
                         string: "department"
                       }
                     }
                     expressions {
                       literal {
                         string: "COUNT(employee_id)"
                       }
                     }
                   }
                 }
               }
             }
        )pb");
}

TEST(MiscFeatures, GroupByWithAvg) {
    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(
        "SELECT department, AVG(salary) FROM employees GROUP BY department");

    ASSERT_TRUE(plan.ok());
    ProtoEqual(
        *plan,
        R"pb(relations {
               root {
                 input {
                   project {
                     input {
                       aggregate {
                         input {
                           read {
                             named_table {
                               names: "employees"
                             }
                           }
                         }
                         groupings {
                           expression_references: 0
                         }
                         measures {
                           measure {
                           }
                         }
                         grouping_expressions {
                           literal {
                             string: "department"
                           }
                         }
                       }
                     }
                     expressions {
                       literal {
                         string: "department"
                       }
                     }
                     expressions {
                       literal {
                         string: "AVG(salary)"
                       }
                     }
                   }
                 }
               }
             }
        )pb");
}
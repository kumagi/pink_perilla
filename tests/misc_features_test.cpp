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
               R"pb(plan {
                      relations {
                        root {
                          fetch {
                            count: 10
                            input {
                              read {
                                base_schema {
                                  names: ["id", "name"]
                                  struct_ {
                                    types { i32 {} }
                                    types { string {} }
                                  }
                                }
                                named_table {
                                  names: "users"
                                }
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
        R"pb(plan {
               relations {
                 root {
                   aggregate {
                     groupings {
                       grouping_expressions {
                         selection {
                           direct_reference {
                             struct_field {
                               field: 0
                             }
                           }
                           root_reference {}
                         }
                       }
                     }
                     measures {
                       measure {
                         function_reference: 1
                         arguments {
                           value {
                             selection {
                               direct_reference {
                                 struct_field {
                                   field: 1
                                 }
                               }
                               root_reference {}
                             }
                           }
                         }
                         output_type {
                           i64 {}
                         }
                       }
                     }
                     input {
                       read {
                         base_schema {
                           names: ["department", "employee_id"]
                           struct_ {
                             types { string {} }
                             types { i32 {} }
                           }
                         }
                         named_table {
                           names: "employees"
                         }
                       }
                     }
                   }
                 }
               }
               extension_uris {
                 extension_uri_anchor: 1
                 uri: "https://github.com/substrait-io/substrait/blob/main/extensions/functions_aggregate.yaml"
               }
               extensions {
                 extension_function {
                   extension_uri_reference: 1
                   function_anchor: 1
                   name: "count:any"
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
        R"pb(plan {
               relations {
                 root {
                   aggregate {
                     groupings {
                       grouping_expressions {
                         selection {
                           direct_reference {
                             struct_field {
                               field: 0
                             }
                           }
                           root_reference {}
                         }
                       }
                     }
                     measures {
                       measure {
                         function_reference: 1
                         arguments {
                           value {
                             selection {
                               direct_reference {
                                 struct_field {
                                   field: 2
                                 }
                               }
                               root_reference {}
                             }
                           }
                         }
                         output_type {
                           fp64 {}
                         }
                       }
                     }
                     input {
                       read {
                         base_schema {
                           names: ["department", "employee_id", "salary"]
                           struct_ {
                             types { string {} }
                             types { i32 {} }
                             types { fp64 {} }
                           }
                         }
                         named_table {
                           names: "employees"
                         }
                       }
                     }
                   }
                 }
               }
               extension_uris {
                 extension_uri_anchor: 1
                 uri: "https://github.com/substrait-io/substrait/blob/main/extensions/functions_aggregate.yaml"
               }
               extensions {
                 extension_function {
                   extension_uri_reference: 1
                   function_anchor: 1
                   name: "avg:fp64"
                 }
               }
             }
        )pb");
}
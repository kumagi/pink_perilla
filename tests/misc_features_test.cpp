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

TEST(Misc, OrderBy) {
    std::string sql = "SELECT * FROM my_table ORDER BY col1 ASC, col2 DESC";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      sort {
        input {
          read {
            named_table {
              names: "my_table"
            }
          }
        }
        sorts {
          expr {
            literal {
              string: "col1"
            }
          }
          direction: SORT_DIRECTION_ASC_NULLS_FIRST
        }
        sorts {
          expr {
            literal {
              string: "col2"
            }
          }
          direction: SORT_DIRECTION_DESC_NULLS_FIRST
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Misc, Limit) {
    std::string sql = "SELECT * FROM my_table LIMIT 10";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      fetch {
        input {
          read {
            named_table {
              names: "my_table"
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
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}


TEST(MiscFeatures, GroupBy) {
    std::string sql =
        "SELECT department, SUM(salary) FROM employees GROUP BY department";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
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
            string: "SUM(salary)"
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(MiscFeatures, AggregateFunctions) {
    std::string sql =
        "SELECT department, COUNT(id), SUM(salary), MAX(salary), MIN(salary) "
        "FROM employees GROUP BY department";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
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
            measures {
              measure {
              }
            }
            measures {
              measure {
              }
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
            string: "COUNT(id)"
          }
        }
        expressions {
          literal {
            string: "SUM(salary)"
          }
        }
        expressions {
          literal {
            string: "MAX(salary)"
          }
        }
        expressions {
          literal {
            string: "MIN(salary)"
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

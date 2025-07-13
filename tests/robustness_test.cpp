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

TEST(Robustness, ExtraWhitespace) {
    std::string sql = "SELECT\t*\tFROM  my_table ";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      read {
        named_table {
          names: "my_table"
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Robustness, SingleLineComment) {
    std::string sql = "SELECT * -- select all columns\nFROM my_table;";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      read {
        named_table {
          names: "my_table"
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Robustness, MultiLineComment) {
    std::string sql =
        "SELECT * FROM /* this is a multi-line comment */ my_table";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      read {
        named_table {
          names: "my_table"
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Robustness, Mixed) {
    std::string sql =
        "SELECT col1, -- select column 1\n col2 /* another comment */ "
        "FROM\nmy_table;";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      project {
        input {
          read {
            named_table {
              names: "my_table"
            }
          }
        }
        expressions {
          literal {
            string: "col1"
          }
        }
        expressions {
          literal {
            string: "col2"
          }
        }
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Robustness, MultiLines) {
    std::string sql = R"SQL(
        SELECT
            c1
        FROM
            my_table
        WHERE
            c2 > 10;
        )SQL";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
        R"pb(relations {
  root {
    input {
      project {
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
              literal {
                string: "c2 > 10"
              }
            }
          }
        }
        expressions {
          literal {
            string: "c1"
          }
        }
      }
    }
    names: "c1"
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

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

TEST(Join, CrossJoin) {
    std::string sql = "SELECT * FROM t1, t2";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      cross {
        left {
          read {
            named_table {
              names: "t1"
            }
          }
        }
        right {
          read {
            named_table {
              names: "t2"
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

TEST(Join, InnerJoin) {
    std::string sql = "SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      join {
        left {
          read {
            named_table {
              names: "t1"
            }
          }
        }
        right {
          read {
            named_table {
              names: "t2"
            }
          }
        }
        expression {
          scalar_function {
            arguments {
              value {
                literal {
                  string: "t1.id = t2.id"
                }
              }
            }
          }
        }
        type: JOIN_TYPE_INNER
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Join, LeftOuterJoin) {
    std::string sql = "SELECT * FROM t1 LEFT OUTER JOIN t2 ON t1.id = t2.id";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      join {
        left {
          read {
            named_table {
              names: "t1"
            }
          }
        }
        right {
          read {
            named_table {
              names: "t2"
            }
          }
        }
        expression {
          scalar_function {
            arguments {
              value {
                literal {
                  string: "t1.id = t2.id"
                }
              }
            }
          }
        }
        type: JOIN_TYPE_LEFT
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

TEST(Join, FourTableJoin) {
    std::string sql =
        "SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id INNER JOIN t3 ON "
        "t2.id = t3.id INNER JOIN t4 ON t3.id = t4.id";
    substrait::Plan plan = pink_perilla::Parse(sql);
    const char* expected_prototext =
R"pb(relations {
  root {
    input {
      join {
        left {
          join {
            left {
              join {
                left {
                  read {
                    named_table {
                      names: "t1"
                    }
                  }
                }
                right {
                  read {
                    named_table {
                      names: "t2"
                    }
                  }
                }
                expression {
                  scalar_function {
                    arguments {
                      value {
                        literal {
                          string: "t1.id = t2.id"
                        }
                      }
                    }
                  }
                }
                type: JOIN_TYPE_INNER
              }
            }
            right {
              read {
                named_table {
                  names: "t3"
                }
              }
            }
            expression {
              scalar_function {
                arguments {
                  value {
                    literal {
                      string: "t2.id = t3.id"
                    }
                  }
                }
              }
            }
            type: JOIN_TYPE_INNER
          }
        }
        right {
          read {
            named_table {
              names: "t4"
            }
          }
        }
        expression {
          scalar_function {
            arguments {
              value {
                literal {
                  string: "t3.id = t4.id"
                }
              }
            }
          }
        }
        type: JOIN_TYPE_INNER
      }
    }
  }
}
)pb";
    AssertPlanEqualsPrototext(plan, expected_prototext);
}

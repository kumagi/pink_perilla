This file provides an example of a SQL query and its corresponding representation in the Substrait prototext format. This helps in understanding the expected output structure for testing purposes.

## Example SQL to Substrait Conversion

### Input SQL

```sql
SELECT a, b FROM table WHERE x > 0;
```

### Output Substrait Plan (prototext)

The following is the Substrait plan that represents the SQL query above. The plan defines the series of operations—reading the table, filtering the rows, and projecting the final columns.

```prototext
plan {
  relations {
    root {
      project {
        common {
          emit: [0, 1]
        }
        input {
          filter {
            condition {
              scalar_function {
                function_reference: 1
                output_type {
                  bool {
                    nullability: NULLABILITY_NULLABLE
                  }
                }
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
                arguments {
                  value {
                    literal {
                      i32: 0
                    }
                  }
                }
              }
            }
            input {
              read {
                base_schema {
                  names: ["a", "b", "x"]
                  struct_ {
                    types {
                      i32 {}
                    }
                    types {
                      i32 {}
                    }
                    types {
                      i32 {}
                    }
                  }
                }
                named_table {
                  names: "table"
                }
              }
            }
          }
        }
        expressions {
          selection {
            direct_reference {
              struct_field {
                field: 0
              }
            }
            root_reference {}
          }
        }
        expressions {
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
    }
  }
  extension_uris {
    extension_uri_anchor: 1
    uri: "https://github.com/substrait-io/substrait/blob/main/extensions/functions_arithmetic.yaml"
  }
  extensions {
    extension_function {
      extension_uri_reference: 1
      function_anchor: 1
      name: "gt:i32_i32"
    }
  }
}
```

---

## DML Examples

### INSERT Statement

**Input SQL:**
```sql
INSERT INTO users (id, name) VALUES (101, 'Alice');
```

**Output Substrait Plan (prototext):**
```prototext
plan {
  relations {
    root {
      write {
        named_table {
          names: "users"
        }
        input {
          virtual_table {
            values {
              struct_ {
                fields {
                  i32: 101
                }
                fields {
                  string: "Alice"
                }
              }
            }
          }
        }
      }
    }
  }
}
```

### UPDATE Statement

**Input SQL:**
```sql
UPDATE users SET name = 'Bob' WHERE id = 101;
```

**Output Substrait Plan (prototext):**
```prototext
plan {
  relations {
    root {
      write {
        named_table {
          names: "users"
        }
        input {
          project {
            common {
              emit: [0, 2] # Project original id and new name
            }
            input {
              filter {
                condition {
                  scalar_function {
                    function_reference: 1
                    output_type { bool { nullability: NULLABILITY_NULLABLE } }
                    arguments {
                      value {
                        selection {
                          direct_reference { struct_field { field: 0 } }
                          root_reference {}
                        }
                      }
                    }
                    arguments {
                      value { literal { i32: 101 } }
                    }
                  }
                }
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
            expressions {
              selection {
                direct_reference { struct_field { field: 0 } }
                root_reference {}
              }
            }
            expressions {
              literal { string: "Bob" }
            }
          }
        }
      }
    }
  }
  extension_uris {
    extension_uri_anchor: 1
    uri: "https://github.com/substrait-io/substrait/blob/main/extensions/functions_comparison.yaml"
  }
  extensions {
    extension_function {
      extension_uri_reference: 1
      function_anchor: 1
      name: "equal:i32_i32"
    }
  }
}
```

### DELETE Statement

**Input SQL:**
```sql
DELETE FROM users WHERE id = 101;
```

**Output Substrait Plan (prototext):**
```prototext
plan {
  relations {
    root {
      write {
        type: DML_OPERATION_TYPE_DELETE
        named_table {
          names: "users"
        }
        input {
          filter {
            condition {
              scalar_function {
                function_reference: 1
                output_type { bool { nullability: NULLABILITY_NULLABLE } }
                arguments {
                  value {
                    selection {
                      direct_reference { struct_field { field: 0 } }
                      root_reference {}
                    }
                  }
                }
                arguments {
                  value { literal { i32: 101 } }
                }
              }
            }
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
  }
  extension_uris {
    extension_uri_anchor: 1
    uri: "https://github.com/substrait-io/substrait/blob/main/extensions/functions_comparison.yaml"
  }
  extensions {
    extension_function {
      extension_uri_reference: 1
      function_anchor: 1
      name: "equal:i32_i32"
    }
  }
}
```
---

## Aggregate and Row Limit Examples

### LIMIT Clause

**Input SQL:**
```sql
SELECT * FROM users LIMIT 10;
```

**Output Substrait Plan (prototext):**
```prototext
plan {
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
```

### GROUP BY with COUNT

**Input SQL:**
```sql
SELECT department, COUNT(employee_id) FROM employees GROUP BY department;
```

**Output Substrait Plan (prototext):**
```prototext
plan {
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
```

### GROUP BY with AVG

**Input SQL:**
```sql
SELECT department, AVG(salary) FROM employees GROUP BY department;
```

**Output Substrait Plan (prototext):**
```prototext
plan {
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
```
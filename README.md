# PinkPerilla

PinkPerilla is a SQL frontend that parses SQL queries and converts them into [Substrait](https://substrait.io/) execution plans. This allows for a standardized representation of data transformation pipelines that can be processed by various backend engines.

This version is a work-in-progress and provides a command-line interface (CLI) for parsing SQL and observing the generated Substrait plan.

## Features

PinkPerilla supports a range of SQL statements, including:

*   **Data Query Language (DQL):** `SELECT` (with projections, filters, and joins)
*   **Data Manipulation Language (DML):** `INSERT`, `UPDATE`, `DELETE`
*   **Data Definition Language (DDL):** `CREATE TABLE`, `DROP TABLE`

## Dependencies

The project manages most of its dependencies using CMake's `FetchContent`.

### Build-time Dependencies
*   A C++17 compliant compiler (e.g., Clang, GCC)
*   CMake (version 3.10 or later)
*   Ninja (or another build system)
*   Readline (must be installed separately, e.g., `brew install readline` on macOS)

### Fetched Dependencies
*   [Protocol Buffers](https://github.com/protocolbuffers/protobuf) (for Substrait message handling)
*   [GoogleTest](https://github.com/google/googletest) (for testing)
*   [Abseil](https://github.com/abseil/abseil-cpp) (as a dependency of Protobuf)

## Building the Project

Follow these steps to build the project from the root directory:

1.  **Configure the project:**
    ```sh
    cmake -B build -S . -G "Ninja"
    ```
    *Note: If `readline` is not found, this step will fail. Please ensure it is installed and accessible.*

2.  **Build the project:**
    ```sh
    cmake --build build
    ```

The main executable will be located at `build/pink_perilla`.

## Usage

The `pink_perilla` executable takes a SQL query and outputs the corresponding Substrait plan in JSON format.

You can provide the SQL query via standard input or as a command-line argument.

**Example using standard input:**
```sh
echo "SELECT * FROM foo WHERE x = 1;" | ./build/pink_perilla
```

**Example using command-line argument:**
```sh
./build/pink_perilla --sql "SELECT * FROM foo WHERE x = 1;"
```

### Example SQL and Output Plan

Below are some examples of SQL queries and a simplified, human-readable representation of the resulting execution plan. **Note: The actual output of the tool is a detailed Substrait plan in JSON format.**

#### Simple Filter
**SQL Input:**
```sql
SELECT * FROM foo WHERE x = 1;
```
**Execution Plan Idea:**
```
filter {
  condition: "x = 1"
  input {
    read {
      table: "foo"
    }
  }
}
```

#### Projection with Filter
**SQL Input:**
```sql
SELECT bar, baz FROM foo WHERE bar > 10;
```
**Execution Plan Idea:**
```
project {
  expressions: ["bar", "baz"]
  input {
    filter {
      condition: "bar > 10"
      input {
        read {
          table: "foo"
        }
      }
    }
  }
}
```

#### INSERT
**SQL Input:**
```sql
INSERT INTO my_table (col1, col2) VALUES (123, 'hello');
```

#### UPDATE
**SQL Input:**
```sql
UPDATE my_table SET col1 = 456 WHERE col2 = 'hello';
```

#### DELETE
**SQL Input:**
```sql
DELETE FROM my_table WHERE col1 = 123;
```

#### CREATE TABLE
**SQL Input:**
```sql
CREATE TABLE new_table (id INT, name VARCHAR(255));
```

#### DROP TABLE
**SQL Input:**
```sql
DROP TABLE old_table;
```

## Running Tests

This project uses CTest and GoogleTest for unit testing. To run the tests, execute the following command from the build directory:

```sh
# Navigate to the build directory
cd build

# Run all tests
ctest --output-on-failure
```

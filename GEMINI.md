# Project Summary: PinkPerilla

This document provides a summary of the PinkPerilla project based on the `README.md` file.

## Core Functionality

PinkPerilla is a C++-based SQL frontend. Its primary purpose is to parse SQL queries and convert them into [Substrait](https://substrait.io/) execution plans. This allows a standardized representation of data transformation pipelines that can be processed by various backend engines.

## Key Features

*   **SQL Dialect Support:**
    *   **DQL:** `SELECT` (with projections, filters, and joins)
    *   **DML:** `INSERT`, `UPDATE`, `DELETE`
    *   **DDL:** `CREATE TABLE`, `DROP TABLE`
*   **Output:** The tool outputs the generated Substrait plan in JSON format.
*   **Interface:** It provides a command-line interface (CLI) for parsing SQL.

## Dependencies

*   **Build-time:**
    *   C++17 compliant compiler (Clang, GCC)
    *   CMake (3.10+)
    *   Ninja (or other build system)
    *   Readline (must be installed on the system)
*   **Fetched via CMake `FetchContent`:**
    *   Protocol Buffers
    *   GoogleTest
    *   Abseil

## Development Workflow

### Building

1.  **Configure:** `cmake -B build -S . -G "Ninja"`
2.  **Build:** `cmake --build build`
3.  The executable is located at `build/pink_perilla`.

### Usage

The CLI can take SQL from standard input or a command-line argument.

*   **Stdin:** `echo "SELECT * FROM foo;" | ./build/pink_perilla`
*   **Argument:** `./build/pink_perilla --sql "SELECT * FROM foo;"`

### Testing

The project uses CTest and GoogleTest.

*   **Run tests:** From the `build` directory, run `ctest --output-on-failure`.

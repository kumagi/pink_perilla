#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include <substrait/plan.pb.h>

#include "absl/status/statusor.h"

struct InsertInfo {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::string> values;

    friend std::ostream &operator<<(std::ostream &ost, const InsertInfo &info) {
        ost << "InsertInfo{table_name: " << info.table_name << ", columns: [";
        for (size_t i = 0; i < info.columns.size(); ++i) {
            ost << info.columns[i] << (i < info.columns.size() - 1 ? ", " : "");
        }
        ost << "], values: [";
        for (size_t i = 0; i < info.values.size(); ++i) {
            ost << info.values[i] << (i < info.values.size() - 1 ? ", " : "");
        }
        return ost << "]}";
    }
};

struct ColumnDef {
    std::string name;
    std::string type;

    friend std::ostream &operator<<(std::ostream &ost, const ColumnDef &info) {
        return ost << "ColumnDef{name: " << info.name << ", type: " << info.type
               << "}";
    }
};

struct CreateTableInfo {
    std::string table_name;
    std::vector<ColumnDef> columns;

    friend std::ostream &operator<<(std::ostream &ost,
                                    const CreateTableInfo &info) {
        ost << "CreateTableInfo{table_name: " << info.table_name
                << ", columns: [";
        for (size_t i = 0; i < info.columns.size(); ++i) {
            ost << info.columns[i] << (i < info.columns.size() - 1 ? ", " : "");
        }
        return ost << "]}";
    }
};

struct DropTableInfo {
    std::string table_name;

    friend std::ostream &operator<<(std::ostream &ost,
                                    const DropTableInfo &info) {
        return ost << "DropTableInfo{table_name: " << info.table_name << "}";
    }
};

struct DeleteInfo {
    std::string table_name;
    std::optional<std::string> where_clause;

    friend std::ostream &operator<<(std::ostream &ost, const DeleteInfo &info) {
        ost << "DeleteInfo{table_name: " << info.table_name;
        if (info.where_clause) {
            ost << ", where_clause: " << *info.where_clause;
        }
        return ost << "}";
    }
};

struct SetClause {
    std::string column_name;
    std::string
    value; // For simplicity, we'll treat values as raw strings for now

    friend std::ostream &operator<<(std::ostream &ost, const SetClause &info) {
        return ost << "SetClause{column_name: " << info.column_name
               << ", value: " << info.value << "}";
    }
};

struct UpdateInfo {
    std::string table_name;
    std::vector<SetClause> set_clauses;
    std::optional<std::string> where_clause;

    friend std::ostream &operator<<(std::ostream &ost, const UpdateInfo &info) {
        ost << "UpdateInfo{table_name: " << info.table_name
                << ", set_clauses: [";
        for (size_t i = 0; i < info.set_clauses.size(); ++i) {
            ost << info.set_clauses[i]
                    << (i < info.set_clauses.size() - 1 ? ", " : "");
        }
        ost << "]";
        if (info.where_clause) {
            ost << ", where_clause: " << *info.where_clause;
        }
        return ost << "}";
    }
};

enum class SortDirection {
    UNSPECIFIED,
    ASC_NULLS_FIRST,
    ASC_NULLS_LAST,
    DESC_NULLS_FIRST,
    DESC_NULLS_LAST,
    CLUSTERED,
};

inline std::ostream& operator<<(std::ostream& ost, const SortDirection &direction) {
    switch (direction) {
        case SortDirection::UNSPECIFIED:
            ost << "(Unspecified)";
            break;
        case SortDirection::ASC_NULLS_LAST:
            ost << "Asc (NULLS-last)";
            break;
        case SortDirection::ASC_NULLS_FIRST:
            ost << "Asc (NULLS-first)";
            break;
        case SortDirection::DESC_NULLS_LAST:
            ost << "Desc (NULLS-first)";
            break;
        case SortDirection::DESC_NULLS_FIRST:
            ost << "Desc (NULLS-first)";
            break;
        case SortDirection::CLUSTERED:
            ost << "Clustered";
            break;
    }
    return ost;
}

struct SortInfo {
    std::string column;
    SortDirection direction;

    friend std::ostream &operator<<(std::ostream &ost, const SortInfo &info) {
        return ost << "SortInfo{column: " << info.column
               << ", direction: " << info.direction << "}";
    }
};

struct WindowFunctionInfo {
    std::string function_name;
    std::vector<std::string> partition_by;

    friend std::ostream &operator<<(std::ostream &ost,
                                    const WindowFunctionInfo &info) {
        ost << "WindowFunctionInfo{function_name: " << info.function_name
                << ", partition_by: [";
        for (size_t i = 0; i < info.partition_by.size(); ++i) {
            ost << info.partition_by[i]
                    << (i < info.partition_by.size() - 1 ? ", " : "");
        }
        return ost << "]}";
    }
};

enum class SelectItemType { COLUMN, AGGREGATE_FUNCTION, WINDOW_FUNCTION };

struct AggregateFunctionInfo {
    std::string function_name;
    std::string column;

    friend std::ostream &operator<<(std::ostream &ost,
                                    const AggregateFunctionInfo &info) {
        return ost << "AggregateFunctionInfo{function_name: "
               << info.function_name << ", column: " << info.column << "}";
    }
};

struct SelectItem {
    SelectItemType type;
    std::string expression; // e.g., "col1", "row_number() OVER (...)"
    std::optional<AggregateFunctionInfo> agg_info;
    std::optional<WindowFunctionInfo> win_info;

    friend std::ostream &operator<<(std::ostream &ost, const SelectItem &info) {
        ost << "SelectItem{type: ";
        switch (info.type) {
            case SelectItemType::COLUMN:
                ost << "COLUMN";
                break;
            case SelectItemType::AGGREGATE_FUNCTION:
                ost << "AGGREGATE";
                break;
            case SelectItemType::WINDOW_FUNCTION:
                ost << "WINDOW";
                break;
        }
        ost << ", expression: " << info.expression;
        if (info.agg_info) {
            ost << ", agg_info: " << *info.agg_info;
        }
        if (info.win_info) {
            ost << ", win_info: " << *info.win_info;
        }
        return ost << "}";
    }
};

enum class JoinType { INNER, LEFT };

struct JoinInfo {
    JoinType type;
    std::string table;
    std::string on_condition;

    friend std::ostream &operator<<(std::ostream &ost, const JoinInfo &info) {
        return ost << "JoinInfo{type: "
               << (info.type == JoinType::INNER ? "INNER" : "LEFT")
               << ", table: " << info.table
               << ", on_condition: " << info.on_condition << "}";
    }
};

struct SelectInfo {
    std::vector<SelectItem> select_items;
    std::optional<std::string> from_table;
    std::optional<std::unique_ptr<SelectInfo> > from_subquery;
    std::vector<std::string> cross_join_tables;
    std::optional<std::string> where_condition;
    std::vector<JoinInfo> joins;
    std::vector<std::string> group_by_columns;
    std::vector<SortInfo> order_by_columns;
    int64_t limit = -1;
    // We will add more fields like window functions, etc. later

    friend std::ostream &operator<<(std::ostream &ost, const SelectInfo &info) {
        ost << "SelectInfo{select_items: [";
        for (size_t i = 0; i < info.select_items.size(); ++i) {
            ost << info.select_items[i]
                    << (i < info.select_items.size() - 1 ? ", " : "");
        }
        if (info.from_table) {
            ost << "], from_table: [" << *info.from_table;
        } else {
            ost << "], from_subquery: nested [";
        }
        if (!info.cross_join_tables.empty()) {
            ost << "], cross_join_tables: [";
            for (size_t i = 0; i < info.cross_join_tables.size(); ++i) {
                ost << info.cross_join_tables[i]
                        << (i < info.cross_join_tables.size() - 1 ? ", " : "");
            }
        }
        if (!info.joins.empty()) {
            ost << "], joins: [";
            for (size_t i = 0; i < info.joins.size(); ++i) {
                ost << info.joins[i] << (i < info.joins.size() - 1 ? ", " : "");
            }
        }
        ost << "]";
        if (info.where_condition) {
            ost << ", where: [" << *info.where_condition;
        }
        ost << "], group_by_columns: [";
        for (size_t i = 0; i < info.group_by_columns.size(); ++i) {
            ost << info.group_by_columns[i]
                    << (i < info.group_by_columns.size() - 1 ? ", " : "");
        }
        ost << "], order_by_columns: [";
        for (size_t i = 0; i < info.order_by_columns.size(); ++i) {
            ost << info.order_by_columns[i]
                    << (i < info.order_by_columns.size() - 1 ? ", " : "");
        }
        ost << "], limit: " << info.limit << "}";
        return ost;
    }
};

union Info {
    SelectInfo select_rows;
    InsertInfo insert_rows;
    UpdateInfo update_rows;
    DeleteInfo delete_rows;
    CreateTableInfo create_table;
    DropTableInfo drop_table;
};

enum class StatementType {
    UNSPECIFIED,
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE_TABLE,
    DROP_TABLE,
};

typedef std::variant<
    SelectInfo,
    UpdateInfo,
    InsertInfo,
    DeleteInfo,
    CreateTableInfo,
    DropTableInfo
    > Statement;

inline std::ostream& operator<<(std::ostream &ost, const Statement& st) {
    if (std::holds_alternative<SelectInfo>(st)) {
        const auto& info = std::get<SelectInfo>(st);
        ost << info;
    } else if (std::holds_alternative<UpdateInfo>(st)) {
        const auto& info = std::get<UpdateInfo>(st);
        ost << info;
    } else if (std::holds_alternative<InsertInfo>(st)) {
        const auto& info = std::get<InsertInfo>(st);
        ost << info;
    } else if (std::holds_alternative<DeleteInfo>(st)) {
        const auto& info = std::get<DeleteInfo>(st);
        ost << info;
    } else if (std::holds_alternative<CreateTableInfo>(st)) {
        const auto& info = std::get<CreateTableInfo>(st);
        ost << info;
    } else if (std::holds_alternative<DropTableInfo>(st)) {
        const auto& info = std::get<DropTableInfo>(st);
        ost << info;
    } else {
        ost << "(UNSPECIFIED)";
    }
    return ost;
};


// Main entry point for the new SQL parser.
absl::StatusOr<Statement> ParseSql(std::string_view sql);

// Specific parsers for DDL statements.
absl::StatusOr<CreateTableInfo> ParseCreateTable(std::string_view sql);

absl::StatusOr<DropTableInfo> ParseDropTable(std::string_view sql);

// Specific parsers for DML statements.
absl::StatusOr<DeleteInfo> ParseDeleteStatement(std::string_view sql);

absl::StatusOr<UpdateInfo> ParseUpdateStatement(std::string_view sql);

absl::StatusOr<SelectInfo> ParseSelectStatement(std::string_view &sql);

absl::StatusOr<InsertInfo> ParseInsertStatement(std::string_view &sql);

// Helper functions for parsing specific clauses.
absl::StatusOr<std::vector<SelectItem> > ParseSelectItems(std::string_view &sql);

absl::StatusOr<std::string> ParseFromClause(std::string_view &sql);

absl::StatusOr<std::vector<JoinInfo> > ParseJoinClauses(std::string_view &sql);

absl::StatusOr<std::vector<std::string> > ParseGroupByClause(
    std::string_view &sql);

absl::StatusOr<std::vector<SortInfo> > ParseOrderByClause(std::string_view &sql);

absl::StatusOr<int64_t> ParseLimitClause(std::string_view &sql);

absl::StatusOr<std::vector<SetClause> > ParseSetClauses(std::string_view &sql);

absl::StatusOr<std::string> ParseWhereClause(std::string_view &sql);

absl::StatusOr<std::vector<ColumnDef> > ParseColumnDefinitions(
    std::string_view &sql);

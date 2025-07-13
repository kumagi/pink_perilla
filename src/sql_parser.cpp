#include "detail/sql_parser.hpp"

#include <cctype>

#include "detail/utils.hpp"
#include "substrait/algebra.pb.h"

namespace {

// A utility to trim whitespace and semicolons from the right.
void TrimRight(std::string_view &sv) {
    if (const size_t last = sv.find_last_not_of(" \t\n\r;");
        last != std::string_view::npos) {
        sv.remove_suffix(sv.length() - (last + 1));
    } else {
        sv.remove_suffix(sv.length());
    }
}

// Consumes leading whitespace and comments.
void ConsumeWhitespace(std::string_view &sv) {
    while (true) {
        if (size_t first = sv.find_first_not_of(" \t\n\r");
            first != std::string_view::npos) {
            sv.remove_prefix(first);
        } else {
            sv.remove_prefix(sv.length());  // Consume all if only whitespace
            return;
        }

        if (sv.rfind("--", 0) == 0) {
            if (size_t end_of_line = sv.find('\n');
                end_of_line != std::string_view::npos) {
                sv.remove_prefix(end_of_line + 1);
            } else {
                sv.remove_prefix(sv.length());
            }
            continue;
        }

        if (sv.rfind("/*", 0) == 0) {
            if (size_t end_of_comment = sv.find("*/");
                end_of_comment != std::string_view::npos) {
                sv.remove_prefix(end_of_comment + 2);
            } else {
                // Unclosed comment, treat as end of string
                sv.remove_prefix(sv.length());
            }
            continue;
        }

        // No more whitespace or comments.
        return;
    }
}

// Tries to consume a specific keyword (case-insensitive).
bool ConsumeKeyword(std::string_view &sv, const std::string &keyword) {
    ConsumeWhitespace(sv);
    if (sv.length() >= keyword.length() &&
        (sv.length() == keyword.length() ||
         !std::isalnum(sv[keyword.length()]))) {
        if (strncasecmp(sv.data(), keyword.data(), keyword.length()) == 0) {
            sv.remove_prefix(keyword.length());
            return true;
        }
    }
    return false;
}

// Parses an identifier.
absl::StatusOr<std::string> ParseIdentifier(std::string_view &sv) {
    ConsumeWhitespace(sv);
    size_t i = 0;
    if (i < sv.length() && (std::isalpha(sv[i]) || sv[i] == '_')) {
        i++;
        while (i < sv.length() && (std::isalnum(sv[i]) || sv[i] == '_')) {
            i++;
        }
    }
    if (i > 0) {
        std::string identifier = std::string(sv.substr(0, i));
        sv.remove_prefix(i);
        return identifier;
    }
    return absl::InvalidArgumentError("Failed to parse identifier");
}

// Tries to consume a specific character.
bool ConsumeChar(std::string_view &sv, char c) {
    ConsumeWhitespace(sv);
    if (!sv.empty() && sv.front() == c) {
        sv.remove_prefix(1);
        return true;
    }
    return false;
}

// Parses a type name, which could be a simple identifier or something like
// "varchar(255)"
absl::StatusOr<std::string> ParseType(std::string_view &sv) {
    ConsumeWhitespace(sv);
    absl::StatusOr<std::string> type_name_status = ParseIdentifier(sv);
    if (!type_name_status.ok()) {
        return type_name_status.status();
    }
    const std::string &type_name = *type_name_status;

    std::string full_type = type_name;
    if (ConsumeChar(sv, '(')) {
        full_type += '(';
        std::string_view before_close = sv;
        size_t i = 0;
        int paren_level = 1;
        while (i < sv.length()) {
            if (sv[i] == '(') {
                paren_level++;
            }
            if (sv[i] == ')') {
                paren_level--;
            }
            if (paren_level == 0) {
                break;
            }
            i++;
        }

        if (paren_level == 0) {
            full_type += std::string(sv.substr(0, i));
            full_type += ')';
            sv.remove_prefix(i + 1);
        } else {
            return absl::InvalidArgumentError(
                "Mismatched parentheses in type definition");
        }
    }
    return full_type;
}

// Parses a column definition (name type).
absl::StatusOr<ColumnDef> ParseColumnDef(std::string_view &sv) {
    auto name_status = ParseIdentifier(sv);
    if (!name_status.ok())
        return name_status.status();

    auto type_status = ParseType(sv);
    if (!type_status.ok())
        return type_status.status();

    return ColumnDef{*name_status, *type_status};
}
}  // anonymous namespace

// --- Public Parser Functions ---

absl::StatusOr<CreateTableInfo> ParseCreateTable(std::string_view sql) {
    if (!ConsumeKeyword(sql, "CREATE") || !ConsumeKeyword(sql, "TABLE")) {
        return absl::InvalidArgumentError("Expected 'CREATE TABLE'");
    }

    auto table_name_status = ParseIdentifier(sql);
    if (!table_name_status.ok())
        return table_name_status.status();

    if (!ConsumeChar(sql, '('))
        return absl::InvalidArgumentError("Expected '(' after table name");

    std::vector<ColumnDef> columns;
    do {
        auto col_def_status = ParseColumnDef(sql);
        if (col_def_status.ok()) {
            columns.push_back(*col_def_status);
        } else {
            return col_def_status.status();
        }
    } while (ConsumeChar(sql, ','));

    if (!ConsumeChar(sql, ')'))
        return absl::InvalidArgumentError(
            "Expected ')' after column definitions");

    ConsumeWhitespace(sql);
    if (!sql.empty())
        return absl::InvalidArgumentError("Unexpected characters after ')'");

    return CreateTableInfo{*table_name_status, columns};
}

absl::StatusOr<DropTableInfo> ParseDropTable(std::string_view sql) {
    if (!ConsumeKeyword(sql, "DROP") || !ConsumeKeyword(sql, "TABLE")) {
        return absl::InvalidArgumentError("Expected 'DROP TABLE'");
    }
    auto table_name_status = ParseIdentifier(sql);
    if (!table_name_status.ok())
        return table_name_status.status();

    ConsumeWhitespace(sql);
    if (!sql.empty())
        return absl::InvalidArgumentError(
            "Unexpected characters after table name");

    return DropTableInfo{*table_name_status};
}

absl::StatusOr<DeleteInfo> ParseDeleteStatement(std::string_view sql) {
    if (!ConsumeKeyword(sql, "DELETE") || !ConsumeKeyword(sql, "FROM")) {
        return absl::InvalidArgumentError("Expected 'DELETE FROM'");
    }
    auto table_name_status = ParseIdentifier(sql);
    if (!table_name_status.ok())
        return table_name_status.status();

    std::optional<std::string> where_clause;
    if (ConsumeKeyword(sql, "WHERE")) {
        ConsumeWhitespace(sql);
        where_clause = std::string(sql);
    } else {
        ConsumeWhitespace(sql);
        if (!sql.empty())
            return absl::InvalidArgumentError(
                "Unexpected characters after table name");
    }

    return DeleteInfo{*table_name_status, where_clause};
}

absl::StatusOr<UpdateInfo> ParseUpdateStatement(std::string_view sql) {
    if (!ConsumeKeyword(sql, "UPDATE"))
        return absl::InvalidArgumentError("Expected 'UPDATE'");

    auto table_name_status = ParseIdentifier(sql);
    if (!table_name_status.ok())
        return table_name_status.status();

    if (!ConsumeKeyword(sql, "SET"))
        return absl::InvalidArgumentError("Expected 'SET'");

    std::vector<SetClause> set_clauses;
    do {
        auto column_name_status = ParseIdentifier(sql);
        if (!column_name_status.ok())
            return column_name_status.status();

        if (!ConsumeChar(sql, '='))
            return absl::InvalidArgumentError("Expected '=' after column name");

        ConsumeWhitespace(sql);

        size_t value_end =
            sql.find_first_of(",Ww");  // Look for comma or 'WHERE'
        if (value_end == std::string_view::npos) {
            value_end = sql.length();
        } else {
            std::string_view temp = sql.substr(value_end);
            if (ConsumeKeyword(temp, "WHERE")) {
                // found where
            } else {
                value_end = sql.find_first_of(',');
                if (value_end == std::string_view::npos) {
                    value_end = sql.length();
                }
            }
        }

        std::string value_str = std::string(sql.substr(0, value_end));
        size_t last = value_str.find_last_not_of(" \t\n\r");
        if (last != std::string::npos) {
            value_str = value_str.substr(0, last + 1);
        }

        sql.remove_prefix(value_end);

        set_clauses.push_back({*column_name_status, value_str});
    } while (ConsumeChar(sql, ','));

    std::optional<std::string> where_clause;
    if (ConsumeKeyword(sql, "WHERE")) {
        ConsumeWhitespace(sql);
        where_clause = std::string(sql);
    } else {
        ConsumeWhitespace(sql);
        if (!sql.empty())
            return absl::InvalidArgumentError(
                "Unexpected characters after SET clause");
    }

    return UpdateInfo{*table_name_status, set_clauses, where_clause};
}

absl::StatusOr<InsertInfo> ParseInsertStatement(std::string_view &sql) {
    if (!ConsumeKeyword(sql, "INSERT") || !ConsumeKeyword(sql, "INTO")) {
        return absl::InvalidArgumentError("Expected 'INSERT INTO'");
    }

    auto table_name_status = ParseIdentifier(sql);
    if (!table_name_status.ok())
        return table_name_status.status();

    if (!ConsumeChar(sql, '(')) {
        return absl::InvalidArgumentError("Expected '(' after table name");
    }

    std::vector<std::string> columns;
    do {
        auto col_status = ParseIdentifier(sql);
        if (!col_status.ok()) {
            return col_status.status();
        }
        columns.push_back(*col_status);
    } while (ConsumeChar(sql, ','));

    if (!ConsumeChar(sql, ')')) {
        return absl::InvalidArgumentError("Expected ')' after column list");
    }

    if (!ConsumeKeyword(sql, "VALUES")) {
        return absl::InvalidArgumentError("Expected 'VALUES'");
    }

    if (!ConsumeChar(sql, '(')) {
        return absl::InvalidArgumentError("Expected '(' before values");
    }

    std::vector<std::string> values;
    do {
        ConsumeWhitespace(sql);
        size_t value_end = sql.find_first_of(",)");
        if (value_end == std::string_view::npos) {
            return absl::InvalidArgumentError("Unterminated values list");
        }
        values.emplace_back(sql.substr(0, value_end));
        sql.remove_prefix(value_end);
    } while (ConsumeChar(sql, ','));

    if (!ConsumeChar(sql, ')'))
        return absl::InvalidArgumentError("Expected ')' after values");

    return InsertInfo{*table_name_status, columns, values};
}

absl::StatusOr<SelectItem> ParseSelectItem(std::string_view &sv) {
    ConsumeWhitespace(sv);
    std::string_view original_sv = sv;

    auto id1_status = ParseIdentifier(sv);
    if (!id1_status.ok())
        return id1_status.status();
    std::string id1 = *id1_status;

    if (ConsumeChar(sv, '(')) {
        std::optional<std::string> arg_id;
        if (!ConsumeChar(sv, ')')) {
            auto arg_id_status = ParseIdentifier(sv);
            if (!arg_id_status.ok()) {
                return arg_id_status.status();
            }
            arg_id = *arg_id_status;
            if (!ConsumeChar(sv, ')')) {
                return absl::InvalidArgumentError(
                    "Expected ')' after function argument");
            }
        }

        std::string_view sv_after_func = sv;
        size_t len = original_sv.length() - sv_after_func.length();
        std::string func_expr = std::string(original_sv.substr(0, len));

        if (ConsumeKeyword(sv, "OVER")) {
            if (!ConsumeChar(sv, '('))
                return absl::InvalidArgumentError("Expected '(' after OVER");
            if (!ConsumeKeyword(sv, "PARTITION") || !ConsumeKeyword(sv, "BY")) {
                return absl::InvalidArgumentError("Expected 'PARTITION BY'");
            }
            std::vector<std::string> partition_cols;
            do {
                auto col_status = ParseIdentifier(sv);
                if (!col_status.ok())
                    return col_status.status();
                partition_cols.push_back(*col_status);
            } while (ConsumeChar(sv, ','));

            if (!ConsumeChar(sv, ')'))
                return absl::InvalidArgumentError(
                    "Expected ')' after PARTITION BY clause");

            size_t full_len = original_sv.length() - sv.length();
            std::string full_expr =
                std::string(original_sv.substr(0, full_len));
            ConsumeWhitespace(sv);
            return SelectItem{SelectItemType::WINDOW_FUNCTION,
                              full_expr,
                              std::nullopt,
                              WindowFunctionInfo{id1, partition_cols}};
        }

        ConsumeWhitespace(sv);
        return SelectItem{SelectItemType::AGGREGATE_FUNCTION,
                          func_expr,
                          AggregateFunctionInfo{id1, arg_id.value_or("")},
                          std::nullopt};
    }

    std::string expression = id1;
    if (ConsumeKeyword(sv, "AS")) {
        auto alias_status = ParseIdentifier(sv);
        if (!alias_status.ok())
            return alias_status.status();
        expression += " AS " + *alias_status;
    }

    ConsumeWhitespace(sv);
    return SelectItem{
        SelectItemType::COLUMN, expression, std::nullopt, std::nullopt};
}

absl::StatusOr<SelectInfo> ParseSelectStatement(std::string_view &sql) {
    if (!ConsumeKeyword(sql, "SELECT"))
        return absl::InvalidArgumentError("Expected 'SELECT'");

    std::vector<SelectItem> select_items;
    if (ConsumeChar(sql, '*')) {
        select_items.push_back(
            {SelectItemType::COLUMN, "*", std::nullopt, std::nullopt});
    } else {
        auto first_item_status = ParseSelectItem(sql);
        if (!first_item_status.ok())
            return first_item_status.status();
        select_items.push_back(*first_item_status);

        while (ConsumeChar(sql, ',')) {
            auto next_item_status = ParseSelectItem(sql);
            if (!next_item_status.ok())
                return next_item_status.status();
            select_items.push_back(*next_item_status);
        }
    }

    if (!ConsumeKeyword(sql, "FROM")) {
        return absl::InvalidArgumentError("Expected 'FROM'");
    }
    SelectInfo result_info;
    result_info.select_items = select_items;

    ConsumeWhitespace(sql);
    if (ConsumeChar(sql, '(')) {
        auto subquery_status = ParseSelectStatement(sql);
        if (!subquery_status.ok())
            return subquery_status.status();
        if (!ConsumeChar(sql, ')'))
            return absl::InvalidArgumentError("Expected ')' after subquery");
        result_info.from_subquery =
            std::make_unique<SelectInfo>(std::move(*subquery_status));
    } else {
        auto table_name_status = ParseIdentifier(sql);
        if (!table_name_status.ok())
            return table_name_status.status();
        result_info.from_table = *table_name_status;
    }

    while (ConsumeChar(sql, ',')) {
        auto next_table_status = ParseIdentifier(sql);
        if (!next_table_status.ok())
            return next_table_status.status();
        result_info.cross_join_tables.push_back(*next_table_status);
    }

    while (true) {
        std::string_view pre_join_sql = sql;
        JoinType join_type;
        if (ConsumeKeyword(sql, "INNER")) {
            join_type = JoinType::INNER;
        } else if (ConsumeKeyword(sql, "LEFT")) {
            ConsumeKeyword(sql, "OUTER");
            join_type = JoinType::LEFT;
        } else {
            sql = pre_join_sql;
            break;
        }

        if (!ConsumeKeyword(sql, "JOIN"))
            return absl::InvalidArgumentError("Expected 'JOIN'");

        auto join_table_status = ParseIdentifier(sql);
        if (!join_table_status.ok())
            return join_table_status.status();

        if (!ConsumeKeyword(sql, "ON"))
            return absl::InvalidArgumentError("Expected 'ON'");

        ConsumeWhitespace(sql);
        std::string_view on_sql = sql;
        size_t on_end = 0;
        while (on_end < on_sql.length()) {
            std::string_view temp_view = on_sql.substr(on_end);
            if (ConsumeKeyword(temp_view, "WHERE") ||
                ConsumeKeyword(temp_view, "GROUP") ||
                ConsumeKeyword(temp_view, "ORDER") ||
                ConsumeKeyword(temp_view, "LIMIT") ||
                ConsumeKeyword(temp_view, "INNER") ||
                ConsumeKeyword(temp_view, "LEFT") ||
                ConsumeKeyword(temp_view, "JOIN")) {
                break;
            }
            on_end++;
        }

        std::string on_condition = std::string(on_sql.substr(0, on_end));
        size_t last = on_condition.find_last_not_of(" \t\n\r");
        if (last != std::string::npos)
            on_condition = on_condition.substr(0, last + 1);

        sql.remove_prefix(on_end);

        result_info.joins.push_back(
            {join_type, *join_table_status, on_condition});
    }

    if (ConsumeKeyword(sql, "WHERE")) {
        ConsumeWhitespace(sql);
        result_info.where_condition = std::string(sql);
    }

    if (ConsumeKeyword(sql, "GROUP") && ConsumeKeyword(sql, "BY")) {
        do {
            auto col_name_status = ParseIdentifier(sql);
            if (!col_name_status.ok())
                return col_name_status.status();
            result_info.group_by_columns.push_back(*col_name_status);
        } while (ConsumeChar(sql, ','));
    }

    if (ConsumeKeyword(sql, "ORDER") && ConsumeKeyword(sql, "BY")) {
        do {
            auto col_name_status = ParseIdentifier(sql);
            if (!col_name_status.ok())
                return col_name_status.status();

            SortInfo sort_info;
            sort_info.column = *col_name_status;
            if (ConsumeKeyword(sql, "DESC")) {
                sort_info.direction = SortDirection::DESC_NULLS_LAST;
            } else {
                ConsumeKeyword(sql, "ASC");
                sort_info.direction = SortDirection::ASC_NULLS_FIRST;
            }
            result_info.order_by_columns.push_back(sort_info);
        } while (ConsumeChar(sql, ','));
    }

    if (ConsumeKeyword(sql, "LIMIT")) {
        ConsumeWhitespace(sql);
        size_t i = 0;
        while (i < sql.length() && std::isdigit(sql[i])) {
            i++;
        }
        if (i > 0) {
            auto limit_val_status =
                pink_perilla::utils::StringToLongOptional(sql.substr(0, i));
            if (!limit_val_status)
                return absl::InvalidArgumentError(
                    "Failed to parse LIMIT value");
            result_info.limit = *limit_val_status;
            sql.remove_prefix(i);
        } else {
            return absl::InvalidArgumentError(
                "LIMIT must be followed by a number");
        }
    }

    ConsumeWhitespace(sql);
    return result_info;
}

// --- Main SQL Parser ---

absl::StatusOr<Statement> ParseSql(std::string_view sql) {
    ConsumeWhitespace(sql);
    std::string_view original_sql = sql;

    std::string_view temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto select_status = ParseSelectStatement(temp_sql);
        select_status.ok()) {
        return Statement{std::move(*select_status)};
    }

    temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto create_status = ParseCreateTable(temp_sql); create_status.ok()) {
        return Statement{std::move(*create_status)};
    }

    temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto drop_status = ParseDropTable(temp_sql); drop_status.ok()) {
        return Statement{std::move(*drop_status)};
    }

    temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto delete_status = ParseDeleteStatement(temp_sql);
        delete_status.ok()) {
        return Statement{std::move(*delete_status)};
    }

    temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto update_status = ParseUpdateStatement(temp_sql);
        update_status.ok()) {
        return Statement{std::move(*update_status)};
    }

    temp_sql = original_sql;
    TrimRight(temp_sql);
    if (auto insert_status = ParseInsertStatement(temp_sql);
        insert_status.ok()) {
        return Statement{std::move(*insert_status)};
    }

    return absl::InvalidArgumentError(
        "Failed to parse SQL statement or statement not supported.");
}

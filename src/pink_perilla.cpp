#include "pink_perilla.hpp"

#include <substrait/plan.pb.h>

#include "detail/sql_parser.hpp"
#include "detail/substrait_converter.hpp"

namespace pink_perilla {

// --- Main Parse Function ---
absl::StatusOr<substrait::Plan> Parse(
    std::string_view sql,
    const std::vector<TableDefinition>& table_definitions) {
    absl::StatusOr<Statement> plan = SqlParser::Parse(sql, table_definitions);
    if (!plan.ok()) {
        return absl::Status(absl::StatusCode::kInternal, "Failed to parse SQL");
    }
    if (std::holds_alternative<CreateTableInfo>(*plan)) {
        return converter::ToSubstrait(std::get<CreateTableInfo>(*plan));
    }
    if (std::holds_alternative<DropTableInfo>(*plan)) {
        return converter::ToSubstrait(std::get<DropTableInfo>(*plan));
    }
    if (std::holds_alternative<DeleteInfo>(*plan)) {
        return converter::ToSubstrait(std::get<DeleteInfo>(*plan));
    }
    if (std::holds_alternative<SelectInfo>(*plan)) {
        return converter::ToSubstrait(std::get<SelectInfo>(*plan));
    }
    if (std::holds_alternative<InsertInfo>(*plan)) {
        return converter::ToSubstrait(std::get<InsertInfo>(*plan));
    }
    if (std::holds_alternative<UpdateInfo>(*plan)) {
        return converter::ToSubstrait(std::get<UpdateInfo>(*plan));
    }
    return absl::Status(absl::StatusCode::kInternal, "Unknown statement type");
}

}  // namespace pink_perilla

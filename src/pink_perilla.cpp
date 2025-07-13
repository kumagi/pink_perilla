#include "pink_perilla.hpp"

#include <iostream>
#include <memory>

#include "sql_parser.hpp"
#include "substrait/plan.pb.h"

namespace pink_perilla {

// --- Main Parse Function ---
substrait::Plan Parse(std::string_view sql) {
    absl::StatusOr<substrait::Plan> plan_status = ParseSql(sql);
    if (!plan_status.ok()) {
        std::cerr << "Failed to parse SQL: " << plan_status.status() << std::endl;
        return substrait::Plan{};
    }
    return *plan_status;
}

}  // namespace pink_perilla


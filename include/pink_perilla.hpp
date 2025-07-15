#pragma once

#include <string_view>
#include <vector>

#include "absl/status/statusor.h"
#include "substrait/plan.pb.h"
#include "table_definition.hpp"

namespace pink_perilla {
absl::StatusOr<substrait::Plan> Parse(
    std::string_view sql,
    const std::vector<TableDefinition>& table_definitions = {});
}

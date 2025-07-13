#pragma once

#include <string_view>

#include "substrait/plan.pb.h"

namespace pink_perilla {
absl::StatusOr<substrait::Plan> Parse(std::string_view sql);
}

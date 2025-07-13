#pragma once

#include "sql_parser.hpp"
#include "substrait/plan.pb.h"

namespace pink_perilla::converter {

// Converts the intermediate representation (SelectInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const SelectInfo& info);

// Converts the intermediate representation (CreateTableInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const CreateTableInfo& info);

// Converts the intermediate representation (DropTableInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const DropTableInfo& info);

// Converts the intermediate representation (DeleteInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const DeleteInfo& info);

// Converts the intermediate representation (UpdateInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const UpdateInfo& info);

// Converts the intermediate representation (InsertInfo) into a Substrait Plan.
substrait::Plan ToSubstrait(const InsertInfo& info);


}  // namespace pink_perilla::converter

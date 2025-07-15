#pragma once

#include <string>
#include <vector>

namespace pink_perilla {

enum class DataType {
  kUnknown,
  kBoolean,
  kI8,
  kI16,
  kI32,
  kI64,
  kFp32,
  kFp64,
  kString,
  kVarChar,
  kFixedChar,
  kBinary,
  kFixedBinary,
  kDate,
  kTime,
  kTimestamp,
  kTimestampTz,
  kIntervalYear,
  kIntervalDay,
  kDecimal,
  kStruct,
  kList,
  kMap,
  kUuid,
};

struct ColumnDefinition {
  std::string name;
  DataType type;
  bool nullable = true;
  // For now, attributes will be simple strings.
  // This can be expanded later to be more structured.
  std::vector<std::string> attributes;
};

struct TableDefinition {
  std::string name;
  std::vector<ColumnDefinition> columns;
};

}  // namespace pink_perilla

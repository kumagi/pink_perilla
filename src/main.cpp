#include <iostream>
#include <string>

#include "absl/status/statusor.h"
#include "google/protobuf/util/json_util.h"
#include "pink_perilla.hpp"
#include "substrait/plan.pb.h"

int main(int argc, char* argv[]) {
    std::string sql;
    if (argc > 2 && std::string(argv[1]) == "--sql") {
        sql = argv[2];
    } else {
        std::getline(std::cin, sql);
    }

    if (sql.empty()) {
        std::cerr << "No SQL statement provided." << std::endl;
        return 1;
    }

    absl::StatusOr<substrait::Plan> plan = pink_perilla::Parse(sql);

    if (!plan.ok()) {
        std::cerr << plan.status().message() << std::endl;
        return 1;
    }

    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    if (google::protobuf::util::MessageToJsonString(*plan, &json_output, options).ok()) {
        std::cout << json_output << std::endl;
    } else {
        std::cerr << "Failed to parse JSON string." << std::endl;
        return 1;
    }

    std::cout << json_output << std::endl;

    return 0;
}

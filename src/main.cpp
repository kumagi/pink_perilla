#include <iostream>
#include <string>

#include "substrait/plan.pb.h"
#include "google/protobuf/util/json_util.h"
#include "pink_perilla.hpp"

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

    substrait::Plan plan = pink_perilla::Parse(sql);

    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    if (!google::protobuf::util::MessageToJsonString(plan, &json_output, options).ok()) {
        std::cerr << "Failed to parse JSON string." << std::endl;
        return 1;
    } else {
        std::cout << json_output << std::endl;
    }

    std::cout << json_output << std::endl;

    return 0;
}

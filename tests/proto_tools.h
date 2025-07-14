//
// Created by Hiroki Kumazaki on 25/07/14.
//

#ifndef PROTO_TOOLS_H
#define PROTO_TOOLS_H
#include <string>
#include <gtest/gtest.h>

#include "google/protobuf/util/message_differencer.h"
#include "substrait/plan.pb.h"

void ProtoEqual(const substrait::Plan &actual,
                const std::string &expected_prototext) {
    substrait::Plan expected;
    google::protobuf::TextFormat::ParseFromString(expected_prototext,
                                                  &expected);

    google::protobuf::util::MessageDifferencer differencer;
    std::string diff_report;
    differencer.ReportDifferencesToString(&diff_report);

    std::string actual_text;
    google::protobuf::TextFormat::PrintToString(actual, &actual_text);
    std::string expected_text;
    google::protobuf::TextFormat::PrintToString(expected, &expected_text);

    ASSERT_TRUE(differencer.Compare(actual, expected))
        << "Protobuf comparison failed:\n"
        << "\n--- Expected ---\n" << expected_text
        << "\n--- Actual ---\n" << actual_text
        << "\n--- Differences ---\n" << diff_report;
}

#endif //PROTO_TOOLS_H

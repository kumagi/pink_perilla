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
    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equivalent(
        actual, expected));
}

#endif //PROTO_TOOLS_H

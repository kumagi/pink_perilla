#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pink_perilla::utils {

inline std::optional<long long> StringToLongOptional(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    long long value = 0;
    size_t i = 0;
    bool is_negative = false;

    if (s[0] == '-') {
        is_negative = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }

    if (i == s.length()) {
        return std::nullopt;  // String contains only a sign.
    }

    for (; i < s.length(); ++i) {
        if (s[i] >= '0' && s[i] <= '9') {
            // Check for potential overflow before multiplication.
            if (value > (LLONG_MAX / 10) ||
                (value == LLONG_MAX / 10 && (s[i] - '0') > LLONG_MAX % 10)) {
                return std::nullopt;  // overflow.
            }
            value = value * 10 + (s[i] - '0');
        } else {
            return std::nullopt;  // Invalid character.
        }
    }

    return is_negative ? -value : value;
}

inline std::optional<int> StringToIntOptional(std::string_view s) {
    std::optional<long long> long_val = StringToLongOptional(s);
    if (!long_val) {
        return std::nullopt;
    }
    if (*long_val > INT_MAX || *long_val < INT_MIN) {
        return std::nullopt;  // Out of int range.
    }
    return static_cast<int>(*long_val);
}

} // namespace pink_perilla::utils


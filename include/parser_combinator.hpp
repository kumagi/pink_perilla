#pragma once

#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace parser_combinator {

// --- Core Data Structures ---

// Holds the result of a parse operation.
// It's a pair of the parsed value and the rest of the input string.
template <typename T>
using ParseResult = std::optional<std::pair<T, std::string_view>>;

// A Parser is a function that takes a string_view and returns a
// ParseResult.
template <typename T>
class Parser {
public:
    using ValueType = T;
    std::function<ParseResult<T>(std::string_view)> parse;

    Parser(std::function<ParseResult<T>(std::string_view)> p) : parse(p) {
    }

    // Functor 'map' operation to transform the result of a parser.
    template <typename F>
    Parser<decltype(f(std::declval<T>()))> map(F f) const {
        using U = decltype(f(std::declval<T>()));
        return Parser<U>([this, f](std::string_view input) -> ParseResult<U> {
            if (ParseResult<T> res = this->parse(input)) {
                return std::make_optional(
                    std::make_pair(f(res->first), res->second));
            }
            return std::nullopt;
        });
    }

    // Functor 'flatMap' operation (also known as 'bind' or '>>=').
    template <typename F>
    decltype(f(std::declval<T>())) flatMap(F f) const {
        using ParserU = decltype(f(std::declval<T>()));
        using ResultType = typename ParserU::ValueType;

        return Parser<ResultType>(
            [p = *this, f](std::string_view input) -> ParseResult<ResultType> {
                if (ParseResult<T> res = p.parse(input)) {
                    return f(res->first).parse(res->second);
                }
                return std::nullopt;
            });
    }
};

// --- Primitive Parsers ---

// A parser that matches a single character.
inline Parser<char> pchar(char c) {
    return Parser<char>([c](std::string_view input) -> ParseResult<char> {
        if (!input.empty() && input[0] == c) {
            return std::make_optional(std::make_pair(c, input.substr(1)));
        }
        return std::nullopt;
    });
}

// A parser that matches a specific string (case-insensitive).
inline Parser<std::string> pstring_ci(const std::string& s) {
    return Parser<std::string>(
        [s](std::string_view input) -> ParseResult<std::string> {
            if (input.length() >= s.length()) {
                bool match = true;
                for (size_t i = 0; i < s.length(); ++i) {
                    if (std::tolower(input[i]) != std::tolower(s[i])) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    return std::make_optional(
                        std::make_pair(std::string(input.substr(0, s.length())),
                                       input.substr(s.length())));
                }
            }
            return std::nullopt;
        });
}

// A parser that matches any character in a given string.
inline Parser<char> any_char_of(const std::string& chars) {
    return Parser<char>([chars](std::string_view input) -> ParseResult<char> {
        if (!input.empty() && chars.find(input[0]) != std::string::npos) {
            return std::make_optional(
                std::make_pair(input[0], input.substr(1)));
        }
        return std::nullopt;
    });
}

// A parser that matches one or more whitespace characters.
inline Parser<std::string> whitespace() {
    return Parser<std::string>(
        [](std::string_view input) -> ParseResult<std::string> {
            size_t i = 0;
            while (i < input.length() && std::isspace(input[i])) {
                i++;
            }
            if (i > 0) {
                return std::make_optional(std::make_pair(
                    std::string(input.substr(0, i)), input.substr(i)));
            }
            return std::nullopt;
        });
}

// A parser that matches one or more digits.
inline Parser<std::string> digits() {
    return Parser<std::string>(
        [](std::string_view input) -> ParseResult<std::string> {
            size_t i = 0;
            while (i < input.length() && std::isdigit(input[i])) {
                i++;
            }
            if (i > 0) {
                return std::make_optional(std::make_pair(
                    std::string(input.substr(0, i)), input.substr(i)));
            }
            return std::nullopt;
        });
}

// A parser that matches an identifier (alphanumeric characters and
// underscore).
inline Parser<std::string> identifier() {
    return Parser<std::string>(
        [](std::string_view input) -> ParseResult<std::string> {
            size_t i = 0;
            if (i < input.length() &&
                (std::isalpha(input[i]) || input[i] == '_')) {
                i++;
                while (i < input.length() &&
                       (std::isalnum(input[i]) || input[i] == '_')) {
                    i++;
                }
            }
            if (i > 0) {
                return std::make_optional(std::make_pair(
                    std::string(input.substr(0, i)), input.substr(i)));
            }
            return std::nullopt;
        });
}

// --- Combinators ---

// Tries a list of parsers and returns the result of the first one that
// succeeds.
template <typename T>
Parser<T> choice(const std::vector<Parser<T>>& parsers) {
    return Parser<T>([parsers](std::string_view input) -> ParseResult<T> {
        for (const auto& p : parsers) {
            ParseResult<T> result = p.parse(input);
            if (result) {
                return result;
            }
        }
        return std::nullopt;
    });
}

// Applies two parsers in sequence and returns a pair of their results.
template <typename T1, typename T2>
Parser<std::pair<T1, T2>> sequence(const Parser<T1>& p1, const Parser<T2>& p2) {
    return p1.flatMap([p2](T1 result1) {
        return p2.map(
            [result1](T2 result2) { return std::make_pair(result1, result2); });
    });
}

// Applies a parser zero or more times and collects the results in a vector.
template <typename T>
Parser<std::vector<T>> many(const Parser<T>& p) {
    return Parser<std::vector<T>>(
        [p](std::string_view input) -> ParseResult<std::vector<T>> {
            std::vector<T> results;
            std::string_view current_input = input;
            while (true) {
                ParseResult<T> result = p.parse(current_input);
                if (result) {
                    results.push_back(result->first);
                    current_input = result->second;
                } else {
                    break;
                }
            }
            return std::make_optional(std::make_pair(results, current_input));
        });
}

// Applies a parser one or more times.
template <typename T>
Parser<std::vector<T>> many1(const Parser<T>& p) {
    return Parser<std::vector<T>>(
        [p](std::string_view input) -> ParseResult<std::vector<T>> {
            ParseResult<T> first_result = p.parse(input);
            if (!first_result) {
                return std::nullopt;
            }
            std::vector<T> results;
            results.push_back(first_result->first);
            std::string_view current_input = first_result->second;

            while (true) {
                ParseResult<T> result = p.parse(current_input);
                if (result) {
                    results.push_back(result->first);
                    current_input = result->second;
                } else {
                    break;
                }
            }
            return std::make_optional(std::make_pair(results, current_input));
        });
}

// Optionally applies a parser.
template <typename T>
Parser<std::optional<T>> optional(const Parser<T>& p) {
    return Parser<std::optional<T>>(
        [p](std::string_view input) -> ParseResult<std::optional<T>> {
            ParseResult<T> result = p.parse(input);
            if (result) {
                return std::make_optional(std::make_pair(
                    std::make_optional(result->first), result->second));
            }
            return std::make_optional(std::make_pair(std::nullopt, input));
        });
}

// Helper to discard the result of the right parser.
template <typename T1, typename T2>
Parser<T1> left(const Parser<T1>& p1, const Parser<T2>& p2) {
    return sequence(p1, p2).map(
        [](std::pair<T1, T2> result) { return result.first; });
}

// Helper to discard the result of the left parser.
template <typename T1, typename T2>
Parser<T2> right(const Parser<T1>& p1, const Parser<T2>& p2) {
    return sequence(p1, p2).map(
        [](std::pair<T1, T2> result) { return result.second; });
}

// Wraps a parser, consuming any whitespace before and after it.
template <typename T>
Parser<T> lexeme(const Parser<T>& p) {
    Parser<std::optional<std::string>> ws = optional(whitespace());
    return right(ws, left(p, ws));
}

// A parser for a comma-separated list of values.
template <typename T>
Parser<std::vector<T>> separated_by(const Parser<T>& p,
                                    const Parser<char>& sep) {
    return sequence(p, many(right(sep, p)))
        .map([](std::pair<T, std::vector<T>> result) {
            std::vector<T> results;
            results.push_back(result.first);
            results.insert(
                results.end(), result.second.begin(), result.second.end());
            return results;
        });
}

}  // namespace parser_combinator

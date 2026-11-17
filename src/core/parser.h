#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "value.h"

namespace json {

// A syntax error with its place in the text. Lines and columns start at 1;
// columns count characters (not bytes), so Cyrillic text is counted right.
class ParseError : public std::runtime_error {
public:
    ParseError(std::string message, int line, int column, std::size_t offset);

    const std::string& message() const { return message_; }
    int line() const { return line_; }
    int column() const { return column_; }
    std::size_t offset() const { return offset_; }  // byte offset in the text

    // "ред 3, колона 7: очаквам ':' след ключа \"city\""
    std::string describe() const;

private:
    std::string message_;
    int line_;
    int column_;
    std::size_t offset_;
};

constexpr int kMaxDepth = 512;

// Strict JSON (RFC 8259). Throws ParseError.
Value parse(std::string_view text);

// The first error, or nothing if the text is valid JSON.
std::optional<ParseError> validate(std::string_view text);

// For values typed by a user: valid JSON is used as is ("42", "true",
// "\"text\"", "[1,2]"); anything else becomes a string ("New York").
Value parseValueOrString(std::string_view text);

}  // namespace json

#pragma once

#include <string>

#include "value.h"

namespace json {

// Turns a value into JSON text. indent > 0: pretty, one value per line;
// indent == 0: compact, on one line.
std::string write(const Value& value, int indent = 2);

// "text" with quotes and escapes, as it appears in a JSON file.
std::string quote(const std::string& text);

// Short one-line preview for lists and trees: strings without quotes are not
// used, so "42" and 42 can be told apart; long text is shortened with "…".
std::string preview(const Value& value, std::size_t maxLength = 60);

}  // namespace json

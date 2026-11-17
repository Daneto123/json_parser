#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "value.h"

namespace json {

// Paths point at a value: keys and array indexes separated by '/'.
//   address/city          the "city" key inside "address"
//   phoneNumbers/0/number  first element of the array, then its "number"
//   "" or "/"             the whole document
// A key that contains '/' is written with "~1", and '~' with "~0"
// (the same rules as JSON Pointer, RFC 6901).
using Path = std::vector<std::string>;

class PathError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

Path parsePath(std::string_view text);
std::string pathToString(const Path& path);

// The value at `path`. Throws PathError with a clear reason if it is missing.
Value& resolve(Value& root, const Path& path);
const Value& resolve(const Value& root, const Path& path);
bool exists(const Value& root, const Path& path);

// Changes the value at an existing path.
void setAt(Value& root, const Path& path, Value value);

// Adds a new key to an object, or a new element to an array (an index equal to
// the size, or "-", appends; a smaller index inserts). The path must not exist.
void createAt(Value& root, const Path& path, Value value);

// Removes and returns the value at `path` (not the whole document).
Value removeAt(Value& root, const Path& path);

// Moves a value to a new path (which must not exist yet). The document is
// changed only if the whole move succeeds.
void moveAt(Value& root, const Path& from, const Path& to);

// Every place where an object has the key `key` (exact match), in document order.
std::vector<Path> searchKey(const Value& root, const std::string& key);

// Keys and text/number values that contain `text` (ignores case for Latin and
// Cyrillic letters).
std::vector<Path> searchText(const Value& root, const std::string& text);

}  // namespace json

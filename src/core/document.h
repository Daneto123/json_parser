#pragma once

#include <string>

#include "value.h"

namespace json {

// An open JSON file: its contents, where it lives and whether it was changed.
class Document {
public:
    // Reads and parses a file. Throws ParseError (bad JSON) or
    // std::runtime_error (cannot read). On error nothing changes.
    void open(const std::string& path);

    void close();

    // Writes to the current path / a new path (and remembers it).
    // Writes to a temporary file first, so a failure never leaves half a file.
    void save(int indent = 2);
    void saveAs(const std::string& path, int indent = 2);

    bool isOpen() const { return open_; }
    bool isModified() const { return modified_; }
    void markModified() { modified_ = true; }
    const std::string& path() const { return path_; }

    Value& root() { return root_; }
    const Value& root() const { return root_; }

private:
    Value root_;
    std::string path_;
    bool open_ = false;
    bool modified_ = false;
};

std::string readTextFile(const std::string& path);   // throws std::runtime_error
void writeTextFile(const std::string& path, const std::string& text);  // atomic

}  // namespace json

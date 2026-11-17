#include "document.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "parser.h"
#include "writer.h"

namespace json {

namespace fs = std::filesystem;

std::string readTextFile(const std::string& path) {
    std::ifstream in(fs::u8path(path), std::ios::binary);
    if (!in) throw std::runtime_error("файлът \"" + path + "\" не може да се отвори");
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeTextFile(const std::string& path, const std::string& text) {
    const fs::path target = fs::u8path(path);
    fs::path temp = target;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("файлът \"" + path + "\" не може да се запише");
        out << text;
        out.flush();
        if (!out) throw std::runtime_error("грешка при запис на \"" + path + "\"");
    }
    std::error_code ec;
    fs::rename(temp, target, ec);
    if (ec) {
        // Windows cannot rename over an existing file on every file system.
        fs::remove(target, ec);
        fs::rename(temp, target, ec);
        if (ec) {
            fs::remove(temp, ec);
            throw std::runtime_error("файлът \"" + path + "\" не може да се запише");
        }
    }
}

void Document::open(const std::string& path) {
    Value parsed = parse(readTextFile(path));  // throws before anything changes
    root_ = std::move(parsed);
    path_ = path;
    open_ = true;
    modified_ = false;
}

void Document::close() {
    root_ = Value();
    path_.clear();
    open_ = false;
    modified_ = false;
}

void Document::save(int indent) { saveAs(path_, indent); }

void Document::saveAs(const std::string& path, int indent) {
    writeTextFile(path, write(root_, indent));
    path_ = path;
    open_ = true;
    modified_ = false;
}

}  // namespace json

#include "path.h"

#include <algorithm>
#include <cctype>

#include "writer.h"

namespace json {

namespace {

std::string unescape(std::string_view token) {
    std::string out;
    for (std::size_t i = 0; i < token.size(); ++i) {
        if (token[i] == '~' && i + 1 < token.size() && (token[i + 1] == '0' || token[i + 1] == '1')) {
            out += token[i + 1] == '0' ? '~' : '/';
            ++i;
        } else {
            out += token[i];
        }
    }
    return out;
}

std::string escape(const std::string& key) {
    std::string out;
    for (char c : key) {
        if (c == '~') out += "~0";
        else if (c == '/') out += "~1";
        else out += c;
    }
    return out;
}

bool isIndex(const std::string& token) {
    if (token.empty() || token.size() > 9) return false;
    for (char c : token) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return token.size() == 1 || token[0] != '0';
}

std::string where(const Path& path, std::size_t upTo) {
    if (upTo == 0) return "в корена";
    return "в " + pathToString(Path(path.begin(), path.begin() + static_cast<std::ptrdiff_t>(upTo)));
}

// Lower-case Latin and Cyrillic letters in UTF-8.
std::string fold(const std::string& s) {
    std::string out = s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        auto& c = reinterpret_cast<unsigned char&>(out[i]);
        if (c < 0x80) {
            c = static_cast<unsigned char>(std::tolower(c));
        } else if (c == 0xD0 && i + 1 < out.size()) {
            auto& n = reinterpret_cast<unsigned char&>(out[i + 1]);
            if (n >= 0x90 && n <= 0x9F) {  // А–П
                n = static_cast<unsigned char>(n + 0x20);
            } else if (n >= 0xA0 && n <= 0xAF) {  // Р–Я
                c = 0xD1;
                n = static_cast<unsigned char>(n - 0x20);
            } else if (n >= 0x80 && n <= 0x8F) {  // Ѐ–Џ
                c = 0xD1;
                n = static_cast<unsigned char>(n + 0x10);
            }
            ++i;
        }
    }
    return out;
}

void collect(const Value& v, Path& current, const std::string& key, bool byText, std::vector<Path>& out) {
    if (v.isObject()) {
        for (std::size_t i = 0; i < v.size(); ++i) {
            current.push_back(v.keyAt(i));
            const bool keyMatch = byText ? fold(v.keyAt(i)).find(key) != std::string::npos : v.keyAt(i) == key;
            if (keyMatch) out.push_back(current);
            else if (byText && !v.at(i).isContainer()) {
                const std::string text = v.at(i).isString() ? v.at(i).asString() : write(v.at(i), 0);
                if (fold(text).find(key) != std::string::npos) out.push_back(current);
            }
            collect(v.at(i), current, key, byText, out);
            current.pop_back();
        }
    } else if (v.isArray()) {
        for (std::size_t i = 0; i < v.size(); ++i) {
            current.push_back(std::to_string(i));
            if (byText && !v.at(i).isContainer()) {
                const std::string text = v.at(i).isString() ? v.at(i).asString() : write(v.at(i), 0);
                if (fold(text).find(key) != std::string::npos) out.push_back(current);
            }
            collect(v.at(i), current, key, byText, out);
            current.pop_back();
        }
    }
}

// Parent container and last token; the parent must exist and be a container.
Value& parentOf(Value& root, const Path& path) {
    Value& parent = resolve(root, Path(path.begin(), path.end() - 1));
    if (!parent.isContainer()) {
        throw PathError(pathToString(Path(path.begin(), path.end() - 1)) + " е " + typeName(parent.type()) +
                        ", а не обект или масив");
    }
    return parent;
}

}  // namespace

Path parsePath(std::string_view text) {
    Path path;
    // Trim spaces and an optional leading '/'.
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    if (!text.empty() && text.front() == '/') text.remove_prefix(1);
    if (text.empty()) return path;

    std::size_t start = 0;
    while (true) {
        const std::size_t slash = text.find('/', start);
        const std::string_view token = text.substr(start, slash == std::string_view::npos ? slash : slash - start);
        path.push_back(unescape(token));
        if (slash == std::string_view::npos) break;
        start = slash + 1;
    }
    return path;
}

std::string pathToString(const Path& path) {
    if (path.empty()) return "/";
    std::string out;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i > 0) out += '/';
        out += escape(path[i]);
    }
    return out;
}

const Value& resolve(const Value& root, const Path& path) {
    const Value* current = &root;
    for (std::size_t i = 0; i < path.size(); ++i) {
        const std::string& token = path[i];
        if (current->isObject()) {
            const Value* next = current->find(token);
            if (!next) throw PathError("няма ключ \"" + token + "\" " + where(path, i));
            current = next;
        } else if (current->isArray()) {
            if (!isIndex(token)) {
                throw PathError(where(path, i) + " има масив; очаквам номер на елемент (0, 1, 2 …), а не \"" + token +
                                "\"");
            }
            const std::size_t index = std::stoul(token);
            if (index >= current->size()) {
                throw PathError("елемент " + token + " " + where(path, i) + " не съществува (масивът има " +
                                std::to_string(current->size()) + ")");
            }
            current = &current->at(index);
        } else {
            throw PathError(pathToString(Path(path.begin(), path.begin() + static_cast<std::ptrdiff_t>(i))) +
                            " е " + typeName(current->type()) + " и няма вложени стойности");
        }
    }
    return *current;
}

Value& resolve(Value& root, const Path& path) {
    return const_cast<Value&>(resolve(static_cast<const Value&>(root), path));
}

bool exists(const Value& root, const Path& path) {
    try {
        resolve(root, path);
        return true;
    } catch (const PathError&) {
        return false;
    }
}

void setAt(Value& root, const Path& path, Value value) { resolve(root, path) = std::move(value); }

void createAt(Value& root, const Path& path, Value value) {
    if (path.empty()) throw PathError("документът вече съществува; използвайте set за да го смените");
    Value& parent = parentOf(root, path);
    const std::string& token = path.back();
    if (parent.isObject()) {
        if (parent.find(token)) {
            throw PathError(pathToString(path) + " вече съществува; използвайте set за да го смените");
        }
        parent.set(token, std::move(value));
        return;
    }
    if (token == "-") {
        parent.append(std::move(value));
        return;
    }
    if (!isIndex(token)) throw PathError("в масив се добавя по номер (0, 1, …) или с \"-\" в края");
    const std::size_t index = std::stoul(token);
    if (index > parent.size()) {
        throw PathError("масивът има " + std::to_string(parent.size()) + " елемента; може да добавите на място от 0 до " +
                        std::to_string(parent.size()));
    }
    parent.insert(index, std::move(value));
}

Value removeAt(Value& root, const Path& path) {
    if (path.empty()) throw PathError("целият документ не може да се изтрие");
    resolve(root, path);  // clear error if missing
    Value& parent = parentOf(root, path);
    if (parent.isObject()) return parent.removeKey(path.back());
    return parent.removeAt(std::stoul(path.back()));
}

void moveAt(Value& root, const Path& from, const Path& to) {
    if (from.empty()) throw PathError("целият документ не може да се премести");
    if (to.size() >= from.size() && std::equal(from.begin(), from.end(), to.begin())) {
        throw PathError("стойност не може да се премести в самата себе си");
    }
    Value copy = root;  // all or nothing
    Value moved = removeAt(copy, from);
    createAt(copy, to, std::move(moved));
    root = std::move(copy);
}

std::vector<Path> searchKey(const Value& root, const std::string& key) {
    std::vector<Path> out;
    Path current;
    collect(root, current, key, false, out);
    return out;
}

std::vector<Path> searchText(const Value& root, const std::string& text) {
    std::vector<Path> out;
    if (text.empty()) return out;
    Path current;
    collect(root, current, fold(text), true, out);
    return out;
}

}  // namespace json

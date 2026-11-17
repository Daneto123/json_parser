#include "writer.h"

namespace json {

namespace {

void writeValue(const Value& v, int indent, int level, std::string& out) {
    auto newline = [&](int lvl) {
        if (indent <= 0) return;
        out += '\n';
        out.append(static_cast<std::size_t>(indent * lvl), ' ');
    };

    switch (v.type()) {
        case Value::Type::Null: out += "null"; return;
        case Value::Type::Bool: out += v.asBool() ? "true" : "false"; return;
        case Value::Type::Number: out += v.numberText(); return;
        case Value::Type::String: out += quote(v.asString()); return;
        case Value::Type::Array:
            if (v.size() == 0) {
                out += "[]";
                return;
            }
            out += '[';
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i > 0) out += ',';
                newline(level + 1);
                writeValue(v.at(i), indent, level + 1, out);
            }
            newline(level);
            out += ']';
            return;
        case Value::Type::Object:
            if (v.size() == 0) {
                out += "{}";
                return;
            }
            out += '{';
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i > 0) out += ',';
                newline(level + 1);
                out += quote(v.keyAt(i));
                out += indent > 0 ? ": " : ":";
                writeValue(v.at(i), indent, level + 1, out);
            }
            newline(level);
            out += '}';
            return;
    }
}

}  // namespace

std::string quote(const std::string& text) {
    static const char* hex = "0123456789abcdef";
    std::string out = "\"";
    for (char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += c;  // UTF-8 is written as is
                }
        }
    }
    out += '"';
    return out;
}

std::string write(const Value& value, int indent) {
    std::string out;
    writeValue(value, indent, 0, out);
    if (indent > 0) out += '\n';
    return out;
}

std::string preview(const Value& value, std::size_t maxLength) {
    std::string text;
    if (value.isArray()) {
        text = "[" + std::to_string(value.size()) + (value.size() == 1 ? " елемент]" : " елемента]");
    } else if (value.isObject()) {
        text = "{" + std::to_string(value.size()) + (value.size() == 1 ? " ключ}" : " ключа}");
    } else {
        text = write(value, 0);
    }
    if (text.size() <= maxLength) return text;
    // Cut at a UTF-8 character boundary.
    std::size_t cut = maxLength;
    while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80) --cut;
    return text.substr(0, cut) + "…";
}

}  // namespace json

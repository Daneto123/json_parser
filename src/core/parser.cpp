#include "parser.h"

#include <cctype>
#include <cstdint>

namespace json {

ParseError::ParseError(std::string message, int line, int column, std::size_t offset)
    : std::runtime_error(message), message_(std::move(message)), line_(line), column_(column), offset_(offset) {}

std::string ParseError::describe() const {
    return "ред " + std::to_string(line_) + ", колона " + std::to_string(column_) + ": " + message_;
}

namespace {

std::string quoted(const std::string& s) { return "\"" + s + "\""; }

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    Value parseDocument() {
        // UTF-8 byte order mark from some Windows editors.
        if (text_.substr(0, 3) == "\xEF\xBB\xBF") pos_ = 3;
        skipWhitespace();
        if (atEnd()) fail("файлът е празен: няма JSON стойност");
        Value root = parseValue(0);
        skipWhitespace();
        if (!atEnd()) {
            fail("излишен текст след края на JSON: " + found() +
                 " (липсва ли запетая или сте сложили две стойности една след друга?)");
        }
        return root;
    }

private:
    struct Mark {
        int line;
        int column;
        std::size_t offset;
    };

    bool atEnd() const { return pos_ >= text_.size(); }
    char peek() const { return atEnd() ? '\0' : text_[pos_]; }

    Mark mark() const { return {line_, column(), pos_}; }

    int column() const {
        // Count characters, not bytes: skip UTF-8 continuation bytes.
        int col = 1;
        for (std::size_t i = lineStart_; i < pos_ && i < text_.size(); ++i) {
            if ((static_cast<unsigned char>(text_[i]) & 0xC0) != 0x80) ++col;
        }
        return col;
    }

    [[noreturn]] void fail(const std::string& message) const { failAt(mark(), message); }

    [[noreturn]] void failAt(const Mark& m, const std::string& message) const {
        throw ParseError(message, m.line, m.column, m.offset);
    }

    std::string found() const {
        if (atEnd()) return "краят на файла";
        const char c = peek();
        if (c == '\n') return "нов ред";
        if (c == '\t') return "табулация";
        // Show one whole UTF-8 character.
        std::size_t len = 1;
        const auto b = static_cast<unsigned char>(c);
        if (b >= 0xF0) len = 4;
        else if (b >= 0xE0) len = 3;
        else if (b >= 0xC0) len = 2;
        return "'" + std::string(text_.substr(pos_, len)) + "'";
    }

    void advance() {
        if (text_[pos_] == '\n') {
            ++line_;
            lineStart_ = pos_ + 1;
        }
        ++pos_;
    }

    void skipWhitespace() {
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                advance();
            } else if (c == '/' && pos_ + 1 < text_.size() && (text_[pos_ + 1] == '/' || text_[pos_ + 1] == '*')) {
                fail("коментари не са позволени в JSON");
            } else {
                return;
            }
        }
    }

    Value parseValue(int depth) {
        if (depth >= kMaxDepth) fail("прекалено дълбоко влагане (над " + std::to_string(kMaxDepth) + " нива)");
        const char c = peek();
        switch (c) {
            case '{': return parseObject(depth);
            case '[': return parseArray(depth);
            case '"': return Value::string(parseString());
            case '\'': fail("низовете се пишат в двойни кавички \"...\", не в единични");
            case '-':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                return parseNumber();
            default: break;
        }
        if (std::isalpha(static_cast<unsigned char>(c))) return parseLiteral();
        if (atEnd()) fail("неочакван край на файла: очаквам стойност");
        if (c == '}' || c == ']') fail("очаквам стойност, а намерих " + found());
        if (c == ',') fail("очаквам стойност, а намерих ',' (две запетаи една след друга?)");
        fail("очаквам стойност (обект, масив, низ, число, true, false или null), а намерих " + found());
    }

    Value parseLiteral() {
        const Mark start = mark();
        std::size_t end = pos_;
        while (end < text_.size() && (std::isalnum(static_cast<unsigned char>(text_[end])) || text_[end] == '_')) ++end;
        const std::string word(text_.substr(pos_, end - pos_));
        Value result;
        if (word == "true") result = Value::boolean(true);
        else if (word == "false") result = Value::boolean(false);
        else if (word == "null") result = Value::null();
        else {
            std::string hint;
            if (word == "True" || word == "TRUE") hint = " (пише се true)";
            else if (word == "False" || word == "FALSE") hint = " (пише се false)";
            else if (word == "None" || word == "NULL" || word == "Null" || word == "nil") hint = " (пише се null)";
            else hint = " (низовете трябва да са в кавички: \"" + word + "\")";
            failAt(start, "непозната дума '" + word + "'" + hint);
        }
        while (pos_ < end) advance();
        return result;
    }

    Value parseNumber() {
        const Mark start = mark();
        const std::size_t begin = pos_;
        auto digits = [&] {
            const std::size_t from = pos_;
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
            return pos_ - from;
        };

        if (peek() == '-') advance();
        if (peek() == '0') {
            advance();
            if (std::isdigit(static_cast<unsigned char>(peek()))) {
                failAt(start, "числата не може да започват с 0 (напр. 007); ако е код, сложете го в кавички");
            }
        } else if (digits() == 0) {
            failAt(start, "невалидно число: след '-' трябва цифра");
        }
        if (peek() == '.') {
            advance();
            if (digits() == 0) fail("невалидно число: след '.' трябва цифра");
        }
        if (peek() == 'e' || peek() == 'E') {
            advance();
            if (peek() == '+' || peek() == '-') advance();
            if (digits() == 0) fail("невалидно число: след 'e' трябва цифра");
        }
        if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
            failAt(start, "невалидно число (текстът трябва да е в кавички)");
        }
        return Value::numberFromText(std::string(text_.substr(begin, pos_ - begin)));
    }

    static void appendUtf8(std::string& out, std::uint32_t cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    std::uint32_t readHex4() {
        const Mark start = mark();
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = peek();
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<std::uint32_t>(c - 'A' + 10);
            else failAt(start, "невалиден код \\u: трябват 4 шестнадесетични цифри");
            advance();
        }
        return value;
    }

    std::string parseString() {
        const Mark start = mark();
        advance();  // opening quote
        std::string out;
        while (true) {
            if (atEnd()) {
                failAt(start, "незатворен низ: липсва затваряща кавичка \"");
            }
            const char c = peek();
            if (c == '"') {
                advance();
                return out;
            }
            if (c == '\n' || c == '\r') {
                failAt(start, "незатворен низ: липсва затваряща кавичка \" преди края на реда");
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                fail("непозволен управляващ символ в низа (табулацията се пише \\t)");
            }
            if (c != '\\') {
                out += c;
                advance();
                continue;
            }
            const Mark escape = mark();
            advance();
            const char e = peek();
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    advance();
                    std::uint32_t cp = readHex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (peek() != '\\' || pos_ + 1 >= text_.size() || text_[pos_ + 1] != 'u') {
                            failAt(escape, "непълна двойка \\u (surrogate pair)");
                        }
                        advance();
                        advance();
                        const std::uint32_t low = readHex4();
                        if (low < 0xDC00 || low > 0xDFFF) failAt(escape, "невалидна двойка \\u (surrogate pair)");
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        failAt(escape, "невалидна двойка \\u (surrogate pair)");
                    }
                    appendUtf8(out, cp);
                    continue;  // readHex4 already advanced
                }
                default:
                    failAt(escape, atEnd() ? "незатворен низ: липсва затваряща кавичка \""
                                           : "невалиден escape '\\" + std::string(1, e) +
                                                 "' (позволени: \\\" \\\\ \\/ \\b \\f \\n \\r \\t \\uXXXX)");
            }
            advance();
        }
    }

    Value parseArray(int depth) {
        const Mark open = mark();
        advance();  // [
        Value array = Value::array();
        skipWhitespace();
        if (peek() == ']') {
            advance();
            return array;
        }
        Mark comma{};
        while (true) {
            skipWhitespace();
            if (peek() == ']') failAt(comma, "излишна запетая преди ']'");
            array.append(parseValue(depth + 1));
            skipWhitespace();
            if (peek() == ',') {
                comma = mark();
                advance();
                continue;
            }
            if (peek() == ']') {
                advance();
                return array;
            }
            if (atEnd()) failAt(open, "незатворен масив: липсва ']'");
            fail("очаквам ',' или ']' в масива (отворен на ред " + std::to_string(open.line) + "), а намерих " +
                 found());
        }
    }

    Value parseObject(int depth) {
        const Mark open = mark();
        advance();  // {
        Value object = Value::object();
        skipWhitespace();
        if (peek() == '}') {
            advance();
            return object;
        }
        Mark comma{};
        while (true) {
            skipWhitespace();
            if (peek() == '}') failAt(comma, "излишна запетая преди '}'");
            if (atEnd()) failAt(open, "незатворен обект: липсва '}'");
            if (peek() == '\'') fail("ключовете се пишат в двойни кавички \"...\", не в единични");
            if (peek() != '"') {
                fail("очаквам ключ в кавички (напр. \"name\") или '}', а намерих " + found());
            }
            const Mark keyMark = mark();
            std::string key = parseString();
            if (object.find(key)) failAt(keyMark, "повторен ключ " + quoted(key) + " в същия обект");
            skipWhitespace();
            if (peek() != ':') fail("очаквам ':' след ключа " + quoted(key) + ", а намерих " + found());
            advance();
            skipWhitespace();
            if (peek() == ',' || peek() == '}') fail("липсва стойност за ключа " + quoted(key));
            object.set(key, parseValue(depth + 1));
            skipWhitespace();
            if (peek() == ',') {
                comma = mark();
                advance();
                continue;
            }
            if (peek() == '}') {
                advance();
                return object;
            }
            if (atEnd()) failAt(open, "незатворен обект: липсва '}'");
            if (peek() == '"') fail("липсва запетая преди ключа (след стойността на " + quoted(key) + ")");
            fail("очаквам ',' или '}' в обекта (отворен на ред " + std::to_string(open.line) + "), а намерих " +
                 found());
        }
    }

    std::string_view text_;
    std::size_t pos_ = 0;
    int line_ = 1;
    std::size_t lineStart_ = 0;
};

}  // namespace

Value parse(std::string_view text) { return Parser(text).parseDocument(); }

std::optional<ParseError> validate(std::string_view text) {
    try {
        parse(text);
        return std::nullopt;
    } catch (const ParseError& e) {
        return e;
    }
}

Value parseValueOrString(std::string_view text) {
    try {
        return parse(text);
    } catch (const ParseError&) {
        return Value::string(std::string(text));
    }
}

}  // namespace json

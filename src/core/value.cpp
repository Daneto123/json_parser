#include "value.h"

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace json {

Value Value::boolean(bool b) {
    Value v;
    v.type_ = Type::Bool;
    v.bool_ = b;
    return v;
}

Value Value::number(double d) {
    if (!std::isfinite(d)) throw std::invalid_argument("JSON numbers must be finite");
    std::ostringstream out;
    out.precision(17);
    out << d;
    // Prefer the shortest text that reads back as the same number.
    for (int p = 1; p <= 17; ++p) {
        std::ostringstream shorter;
        shorter.precision(p);
        shorter << d;
        if (std::strtod(shorter.str().c_str(), nullptr) == d) return numberFromText(shorter.str());
    }
    return numberFromText(out.str());
}

Value Value::numberFromText(std::string text) {
    Value v;
    v.type_ = Type::Number;
    v.text_ = std::move(text);
    return v;
}

Value Value::string(std::string s) {
    Value v;
    v.type_ = Type::String;
    v.text_ = std::move(s);
    return v;
}

Value Value::array() {
    Value v;
    v.type_ = Type::Array;
    return v;
}

Value Value::object() {
    Value v;
    v.type_ = Type::Object;
    return v;
}

double Value::asNumber() const { return std::strtod(text_.c_str(), nullptr); }

std::size_t Value::size() const { return isContainer() ? items_.size() : 0; }

void Value::insert(std::size_t index, Value v) {
    items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), std::move(v));
}

Value Value::removeAt(std::size_t index) {
    Value removed = std::move(items_[index]);
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    if (isObject()) keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(index));
    return removed;
}

long Value::indexOf(const std::string& key) const {
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        if (keys_[i] == key) return static_cast<long>(i);
    }
    return -1;
}

Value* Value::find(const std::string& key) {
    const long i = indexOf(key);
    return i < 0 ? nullptr : &items_[static_cast<std::size_t>(i)];
}

const Value* Value::find(const std::string& key) const {
    const long i = indexOf(key);
    return i < 0 ? nullptr : &items_[static_cast<std::size_t>(i)];
}

void Value::set(const std::string& key, Value v) {
    if (Value* existing = find(key)) {
        *existing = std::move(v);
        return;
    }
    keys_.push_back(key);
    items_.push_back(std::move(v));
}

Value Value::removeKey(const std::string& key) {
    return removeAt(static_cast<std::size_t>(indexOf(key)));
}

bool operator==(const Value& a, const Value& b) {
    if (a.type_ != b.type_) return false;
    switch (a.type_) {
        case Value::Type::Null: return true;
        case Value::Type::Bool: return a.bool_ == b.bool_;
        case Value::Type::Number: return a.asNumber() == b.asNumber();
        case Value::Type::String: return a.text_ == b.text_;
        case Value::Type::Array: return a.items_ == b.items_;
        case Value::Type::Object: return a.keys_ == b.keys_ && a.items_ == b.items_;
    }
    return false;
}

const char* typeName(Value::Type type) {
    switch (type) {
        case Value::Type::Null: return "null";
        case Value::Type::Bool: return "булева";
        case Value::Type::Number: return "число";
        case Value::Type::String: return "низ";
        case Value::Type::Array: return "масив";
        case Value::Type::Object: return "обект";
    }
    return "";
}

}  // namespace json

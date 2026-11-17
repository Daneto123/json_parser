#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace json {

// A JSON value: null, true/false, number, string, array or object.
// Objects keep the order of their keys, so a file is written back the way it
// was read. Numbers keep their original text ("1.50" stays "1.50").
class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Value() = default;  // null

    static Value null() { return Value(); }
    static Value boolean(bool b);
    static Value number(double d);
    static Value numberFromText(std::string text);  // text must be a valid JSON number
    static Value string(std::string s);
    static Value array();
    static Value object();

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }
    bool isContainer() const { return isArray() || isObject(); }

    bool asBool() const { return bool_; }
    double asNumber() const;
    const std::string& numberText() const { return text_; }
    const std::string& asString() const { return text_; }

    // Arrays and objects: number of elements / keys.
    std::size_t size() const;

    // Arrays.
    Value& at(std::size_t index) { return items_[index]; }
    const Value& at(std::size_t index) const { return items_[index]; }
    void append(Value v) { items_.push_back(std::move(v)); }
    void insert(std::size_t index, Value v);
    Value removeAt(std::size_t index);

    // Objects (keys_[i] belongs to items_[i]).
    const std::string& keyAt(std::size_t index) const { return keys_[index]; }
    long indexOf(const std::string& key) const;  // -1 if missing
    Value* find(const std::string& key);
    const Value* find(const std::string& key) const;
    void set(const std::string& key, Value v);  // replaces or appends
    Value removeKey(const std::string& key);    // key must exist
    void renameKey(std::size_t index, std::string newKey) { keys_[index] = std::move(newKey); }

    friend bool operator==(const Value& a, const Value& b);
    friend bool operator!=(const Value& a, const Value& b) { return !(a == b); }

private:
    Type type_ = Type::Null;
    bool bool_ = false;
    std::string text_;  // number text or string contents (UTF-8)
    std::vector<Value> items_;
    std::vector<std::string> keys_;
};

const char* typeName(Value::Type type);  // "обект", "масив", ...

}  // namespace json

#ifndef JSON_H
#define JSON_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
using namespace std;

const int char_buffer = 64;

class Json_struct {
private:
    std::string _key;
    std::string _value;
public:
    Json_struct() : _key(""), _value("") {}
    Json_struct(const std::string &_key, const std::string &_value) : _key(_key), _value(_value) {}
    Json_struct(const Json_struct &other) : _key(other._key), _value(other._value) {}
    Json_struct& operator=(const Json_struct &other) {
        if(this != &other) {
            _key = other._key;
            _value = other._value; 
        }
        return *this;
    }
    
    void set_key(const std::string &_key) { this->_key = _key; }
    void set_value(const std::string &_value) { this->_value = _value; }
    
    std::string get_key() const { return this->_key; }
    std::string get_value() const { return this->_value; }
};

extern std::vector<std::string> local_read;  // За all_read()

class Json {
private:
    std::vector<Json_struct> file_contents;
    std::vector<std::string> local_read;
public:
    Json() : file_contents() {}
    
    Json(const std::vector<Json_struct>& fileContents) : file_contents(fileContents) {}
    
    Json(const Json &other) : file_contents(other.file_contents) {}
    
    Json& operator=(const Json &other) {
        if(this != &other) {
            file_contents = other.file_contents;
        }
        return *this;
    }
    
    bool validate(const std::string& file_path);
    std::string readFile(const std::string& file_path);
    bool set(const std::string& path, const std::string& word_for_replace);
    bool create(const std::string& path, const std::string& value);
    bool deletefunc(const std::string& path);
    bool move(const std::string &from_path, const std::string &to_path);
    void save(const std::string &path);
    void save_as(const std::string &file_path, const std::string &path);
    void read_from_file(const std::string &file_path);
    void print_vector() const;
    void all_read(const std::string &file_path);
    void save_to_file(const std::string &file_path) const;
};

#endif
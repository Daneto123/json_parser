#include "json.h"

/**
 * намира и заменя дума зададена от потребителя
 * @param path - думата съседна на търсената
 * @param word_for_replace - думата с която искаме да заменим
 */ 

bool Json::set(const std::string &key, const std::string &new_value){
    bool found = false;

    for (auto &item : file_contents){
        if (item.get_key() == key){
            item.set_value(new_value);
            found = true;
        }
    }

    return found;
}

/**
 * намира и заменя дума зададена от потребителя
 * @param file_path - името на фаелът в които искаме заменим думата
 */ 

void Json::all_read(const std::string &file_path){
    std::ifstream file(file_path);

    if (!file.is_open()) return;

    local_read.clear();

    std::string word;
    while (file >> word){
        local_read.push_back(word);
    }
}

/**
 * служи за четене от фаел
 * @param file_path - името на фаелът който искаме да прочетем
 */

void Json::read_from_file(const std::string &file_path) {
    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cout << "can't open file\n";
        return;
    }

    file_contents.clear();

    std::string key, value;
    char ch;

    while (file >> ch) {

        if (ch == '{' || ch == ',' || ch == '}')
            continue;

        file >> key >> value;

        if (!key.empty() && !value.empty()) {
            file_contents.emplace_back(Json_struct(key, value));
        }
    }
}
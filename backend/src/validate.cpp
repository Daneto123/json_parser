#include "json.h"

/**
 * проверява json фаелът дали е валиден като синтаксис изкарва следващата дума след грешната
 * @param file_path - масивът на който искаме да намерим дължината
 */ 

bool Json::validate(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return false;

    int depth = 0;
    bool in_string = false;
    bool escape = false;

    char c;

    while (file.get(c)) {

        if (escape) {
            escape = false;
            continue;
        }

        if (c == '\\') {
            escape = true;
            continue;
        }

        if (c == '"') {
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            continue;
        }

        if (c == '{' || c == '[') {
            depth++;
        }
        else if (c == '}' || c == ']') {
            depth--;
            if (depth < 0) return false;
        }
    }

    return depth == 0 && !in_string && !escape;
}
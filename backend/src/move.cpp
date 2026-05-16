#include "json.h"

/**
 * премества ред от едно място да друго
 * @param from_path - редът който искаме да преместим
 * @param to_path - мястото където искаме да поставим редът
 */ 

bool Json::move(const std::string &from_path, const std::string &to_path) {
    int fromIndex = -1;
    int toIndex = -1;

    for (size_t i = 0; i < file_contents.size(); i++) {
        if (file_contents[i].get_key() == from_path)
            fromIndex = i;

        if (file_contents[i].get_key() == to_path)
            toIndex = i;
    }

    if (fromIndex == -1 || toIndex == -1)
        return false;

    if (fromIndex == toIndex)
        return true;

    auto element = file_contents[fromIndex];

    file_contents.erase(file_contents.begin() + fromIndex);

    // ако премахването е преди toIndex, index се мести
    if (fromIndex < toIndex)
        toIndex--;

    file_contents.insert(file_contents.begin() + toIndex, element);

    return true;
}
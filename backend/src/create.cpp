#include "json.h"

/**
 * по зададен път да добавим дума в фаелът
 * @param path - думата съседна на търсената
 * @param word_for_create - думата която искаме да създадем
 */ 

bool Json::create(const std::string &path, const std::string &word_for_create){

    std::string new_word = "";
    bool errorWord = false;

    for (size_t i = 0; i < file_contents.size(); i++){
        if(file_contents[i].get_key() == path){
            new_word = " " + file_contents[i].get_key() + " " + word_for_create;
            file_contents[i].set_key(new_word);
        }else if(file_contents[i].get_value() == path){
            new_word = " " + file_contents[i].get_value() + " " + word_for_create;
            file_contents[i].set_value(new_word);
        }else{
            errorWord = true;
        }
    }

    return errorWord;
}
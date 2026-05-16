#include "json.h"

/**
 * намира и изтрива дума 
 * @param file_path - името на фаелът в който искаме да работим
 * @param path - думата която искаме да изтрием
 */ 

bool Json::deletefunc(const std::string &path){
    bool errorFileFound = false;

    for (size_t i = 0; i < file_contents.size(); i++){
        if(file_contents[i].get_key() == path){
            file_contents[i].set_key("");
            break;
        }else if(file_contents[i].get_value() == path){
            file_contents[i].set_value("");
        }else{
            errorFileFound = true;
        }
    }

    return errorFileFound;
}
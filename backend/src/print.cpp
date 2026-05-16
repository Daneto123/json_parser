#include "json.h"

/**
 * принти 
 * @param file_path - името на фаелът който искаме да принтираме
 */ 

std::string Json::readFile(const std::string& file_path){

    ifstream input_stream;
    input_stream.open(file_path, ios::in);

    if(!input_stream.is_open()) {
        throw std::runtime_error("can't open file");
    }

    std::string content, line;
    while (std::getline(input_stream, line)) {
        content += line + "\n";
    }

    return content;

}
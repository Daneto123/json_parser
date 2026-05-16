#include "json.h"
#include "command.h"

std::vector<std::string> split(const std::string& input) {
    std::vector<std::string> words;
    std::string word;
    
    std::istringstream iss(input);
    while (iss >> word) {
        words.push_back(word);
    }
    
    return words;
}

int main(){

    std::cout<<"in to"<<std::endl;
    std::string input;
    getline(std::cin,input);

    Command commands;
    Json json;

    while(input != "exit"){

        auto words = split(input);
        std::string command = words[0];

        if(command == "open"){

           json.all_read("experiment.json");
           json.read_from_file("experiment.json");
           std::cout<<"the file is succsessfully open"<<std::endl;

        }else if(command == "save"){

           json.save_to_file("expo.json");

        }else if(command == "save_as"){

            std::string file_name = words[1];
            json.save_to_file(file_name);

        }else if(command == "help"){

            commands.help();

        }else if(command == "move"){

            std::string from_path = words[1];
            std::string to_path = words[2];
            json.move(from_path, to_path);
            json.save_to_file("expo.json");

        }else if(command == "print"){

            char* file_name = new char[words[1].length()+1];
            strcpy(file_name, words[1].c_str());
            json.readFile(file_name);
            delete[] file_name;

        }else if(command == "create"){

            std::string path = words[1];
            std::string word_for_create = words[2];
            json.create(path, word_for_create);
            json.save_to_file("expo.json");

        }else if(command == "save_file"){

            std::string path = words[1];
            json.save(path);

        }else if(command == "save_as_file"){

            std::string file_path = words[1];
            std::string path = words[2];
            json.save_as(file_path, path);

        }else if(command == "replace"){

            std::string path = words[1];
            std::string word_for_repalce = words[2];
            json.set(path, word_for_repalce);
            json.save_to_file("expo.json");

        }else if(command == "validate"){

            char* file_name = new char[words[1].length()+1];
            strcpy(file_name, words[1].c_str());
            json.validate(file_name);
            delete[] file_name;

        }else if(command == "delete"){

            std::string path = words[1];
            json.deletefunc(path);
            json.save_to_file("expo.json");

        }

        getline(std::cin, input);
    }


}
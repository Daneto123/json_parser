#include "json.h"

/**
 * запазва елемент в файл
 * @param path - редът които искаме да запазим
 */ 

void Json::save(const string &path){

    string file_path = "exp.json";

    fstream file1; 
    file1.open(file_path);

    if (file1.is_open()){ 
        file1 << path;
        file1.close(); 
        cout<<"successfully save word to file "<<file_path<<endl;
    }else{
        cout<<"can't open the file"<<endl;
    }
}

/**
 * намира дължината на чар*
 * @param path - пътят до елементът който искаме да презапишем
 */ 

void Json::save_as(const string &file_path, const string &path){//da zapazva dadeniqt red v file koito potrebitelqt iska

    fstream file1; 
    file1.open(file_path);

    if (file1.is_open()){
        file1 << path;
        file1.close();
        cout<<"successfully save word to file "<<file_path<<endl;
    }else{
        cout<<"can't open the file"<<endl;
    }
}

void Json::save_to_file(const string &file_path) const {
    
    fstream file;
    file.open(file_path);
    int count_vector;

    if(file.is_open()) {
        for (size_t k = 0; k < local_read.size(); k++){
            if(local_read[k] == "{" || local_read[k] == "}" || local_read[k] == "}," || local_read[k] == "],"){
                   file<<local_read[k]<<endl;   
            }else if(local_read[k+1] == "{" || local_read[k+1] == "[" || local_read[k+1] == "[],"){
                file<<local_read[k]<<" "<<local_read[k+1]<<endl;
                k++;
            }else if(local_read[k].find('"') != string::npos && count_vector <= file_contents.size()){
                file<<" "<<file_contents[count_vector].get_key()<<" "<<file_contents[count_vector].get_value()<<endl;
                count_vector++;
                k ++;
            }
        }
        file.close();
    }else{
        cout<<"can't open the file"<<endl;
    } 
}
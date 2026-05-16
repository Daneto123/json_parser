#ifndef COMMAND_H
#define COMMAND_H

#include <fstream>
#include <iostream>

class Command
{
protected:
    std::string current_file;
public:
    void help();
};

#endif
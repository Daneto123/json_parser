// JSON парсер: конзолна версия.
//
//   json_cli                 интерактивно, команда по команда
//   json_cli файл.json       отваря файла веднага

#include <iostream>
#include <string>

#include "shell.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    // Cyrillic in the Windows console.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    json::Shell shell(std::cout);
    std::cout << "JSON парсер. Напишете help за командите.\n";
    if (argc > 1) shell.execute(std::string("open \"") + argv[1] + "\"");

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;  // end of input (Ctrl+Z / Ctrl+D)
        if (!shell.execute(line)) return 0;
    }
    std::cout << '\n';
    return 0;
}

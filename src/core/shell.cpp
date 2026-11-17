#include "shell.h"

#include <cctype>

#include "parser.h"
#include "path.h"
#include "writer.h"

namespace json {

namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

// Takes the first argument from `rest`: a word, or "text in quotes".
std::string takeArg(std::string& rest) {
    rest = trim(rest);
    if (rest.empty()) return {};
    std::string arg;
    std::size_t end;
    if (rest[0] == '"') {
        end = rest.find('"', 1);
        if (end == std::string::npos) end = rest.size();
        arg = rest.substr(1, end - 1);
        ++end;
    } else {
        end = 0;
        while (end < rest.size() && !std::isspace(static_cast<unsigned char>(rest[end]))) ++end;
        arg = rest.substr(0, end);
    }
    rest = end < rest.size() ? trim(rest.substr(end)) : std::string();
    return arg;
}

// A file name: the rest of the line, without surrounding quotes.
std::string fileArg(const std::string& rest) {
    std::string f = trim(rest);
    if (f.size() >= 2 && f.front() == '"' && f.back() == '"') f = f.substr(1, f.size() - 2);
    return f;
}

}  // namespace

bool Shell::execute(const std::string& rawLine) {
    std::string rest = trim(rawLine);
    if (rest.empty()) return true;
    std::string command = takeArg(rest);
    for (auto& c : command) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (command != "close" && command != "exit" && command != "open") pendingDiscard_.clear();

    try {
        if (command == "exit" || command == "quit") {
            if (!confirmDiscard("exit")) return true;
            out_ << "Довиждане!\n";
            return false;
        }
        if (command == "help" || command == "?") printHelp();
        else if (command == "open") cmdOpen(fileArg(rest));
        else if (command == "close") cmdClose();
        else if (command == "save") cmdSave();
        else if (command == "saveas") cmdSaveAs(fileArg(rest));
        else if (command == "validate") cmdValidate(fileArg(rest));
        else if (command == "print") cmdPrint(rest);
        else if (command == "search") cmdSearch(fileArg(rest), false);
        else if (command == "find") cmdSearch(fileArg(rest), true);
        else if (command == "set") cmdSet(rest, false);
        else if (command == "create") cmdSet(rest, true);
        else if (command == "delete") cmdDelete(rest);
        else if (command == "move") cmdMove(rest);
        else out_ << "Непозната команда \"" << command << "\". Напишете help за списък с командите.\n";
    } catch (const ParseError& e) {
        out_ << "Грешка: " << e.describe() << '\n';
    } catch (const std::exception& e) {
        out_ << "Грешка: " << e.what() << '\n';
    }
    return true;
}

void Shell::printHelp() {
    out_ << "Команди:\n"
            "  open <файл>              отваря JSON файл\n"
            "  close                    затваря файла\n"
            "  save                     записва промените в същия файл\n"
            "  saveas <файл>            записва в нов файл\n"
            "  validate [файл]          проверява синтаксиса и показва къде е грешката\n"
            "  print [път]              показва целия файл или част от него\n"
            "  search <ключ>            намира всички места с този ключ\n"
            "  find <текст>             търси текст в ключовете и стойностите\n"
            "  set <път> <стойност>     сменя стойност\n"
            "  create <път> <стойност>  добавя нов ключ или елемент\n"
            "  delete <път>             изтрива ключ или елемент\n"
            "  move <от> <към>          премества стойност на нов път\n"
            "  help                     тази помощ\n"
            "  exit                     изход\n"
            "\n"
            "Пътят е от ключове и номера, разделени с '/': address/city, phoneNumbers/0/number\n"
            "Стойността е JSON (42, true, null, [1,2], {\"a\":1}) или обикновен текст (New York).\n";
}

bool Shell::requireOpen() {
    if (doc_.isOpen()) return true;
    out_ << "Няма отворен файл. Използвайте: open <файл>\n";
    return false;
}

bool Shell::confirmDiscard(const std::string& command) {
    if (!doc_.isModified() || pendingDiscard_ == command) {
        pendingDiscard_.clear();
        return true;
    }
    pendingDiscard_ = command;
    out_ << "Има незапазени промени. Напишете save, за да ги запишете, или " << command
         << " отново, за да ги изоставите.\n";
    return false;
}

void Shell::cmdOpen(const std::string& file) {
    if (file.empty()) {
        out_ << "Използване: open <файл>\n";
        return;
    }
    if (!confirmDiscard("open")) return;
    doc_.open(file);
    out_ << "Отворен е \"" << file << "\".\n";
}

void Shell::cmdClose() {
    if (!requireOpen()) return;
    if (!confirmDiscard("close")) return;
    const std::string name = doc_.path();
    doc_.close();
    out_ << "Затворен е \"" << name << "\".\n";
}

void Shell::cmdSave() {
    if (!requireOpen()) return;
    doc_.save();
    out_ << "Записано в \"" << doc_.path() << "\".\n";
}

void Shell::cmdSaveAs(const std::string& file) {
    if (!requireOpen()) return;
    if (file.empty()) {
        out_ << "Използване: saveas <файл>\n";
        return;
    }
    doc_.saveAs(file);
    out_ << "Записано в \"" << file << "\".\n";
}

void Shell::cmdValidate(const std::string& file) {
    std::string target = file;
    if (target.empty()) {
        if (!doc_.isOpen()) {
            out_ << "Използване: validate <файл> (или първо open <файл>)\n";
            return;
        }
        target = doc_.path();
    }
    const auto error = validate(readTextFile(target));
    if (error) out_ << "✗ \"" << target << "\" НЕ е валиден JSON — " << error->describe() << '\n';
    else out_ << "✓ \"" << target << "\" е валиден JSON.\n";
}

void Shell::cmdPrint(const std::string& path) {
    if (!requireOpen()) return;
    out_ << write(resolve(doc_.root(), parsePath(path)));
}

void Shell::cmdSearch(const std::string& text, bool byText) {
    if (!requireOpen()) return;
    if (text.empty()) {
        out_ << (byText ? "Използване: find <текст>\n" : "Използване: search <ключ>\n");
        return;
    }
    const auto results = byText ? searchText(doc_.root(), text) : searchKey(doc_.root(), text);
    if (results.empty()) {
        out_ << "Нищо не е намерено.\n";
        return;
    }
    for (const auto& p : results) {
        out_ << "  " << pathToString(p) << " = " << preview(resolve(doc_.root(), p)) << '\n';
    }
    out_ << "Намерени: " << results.size() << '\n';
}

void Shell::cmdSet(const std::string& args, bool create) {
    if (!requireOpen()) return;
    std::string rest = args;
    const std::string pathText = takeArg(rest);
    if (pathText.empty() || rest.empty()) {
        out_ << (create ? "Използване: create <път> <стойност>\n" : "Използване: set <път> <стойност>\n");
        return;
    }
    const Path path = parsePath(pathText);
    Value value = parseValueOrString(rest);
    const std::string shown = preview(value);
    if (create) createAt(doc_.root(), path, std::move(value));
    else setAt(doc_.root(), path, std::move(value));
    doc_.markModified();
    out_ << (create ? "Добавено: " : "Променено: ") << pathToString(path) << " = " << shown << '\n';
}

void Shell::cmdDelete(const std::string& args) {
    if (!requireOpen()) return;
    std::string rest = args;
    const std::string pathText = takeArg(rest);
    if (pathText.empty()) {
        out_ << "Използване: delete <път>\n";
        return;
    }
    const Path path = parsePath(pathText);
    const Value removed = removeAt(doc_.root(), path);
    doc_.markModified();
    out_ << "Изтрито: " << pathToString(path) << " (беше " << preview(removed) << ")\n";
}

void Shell::cmdMove(const std::string& args) {
    if (!requireOpen()) return;
    std::string rest = args;
    const std::string from = takeArg(rest);
    const std::string to = takeArg(rest);
    if (from.empty() || to.empty()) {
        out_ << "Използване: move <от> <към>\n";
        return;
    }
    moveAt(doc_.root(), parsePath(from), parsePath(to));
    doc_.markModified();
    out_ << "Преместено: " << pathToString(parsePath(from)) << " → " << pathToString(parsePath(to)) << '\n';
}

}  // namespace json

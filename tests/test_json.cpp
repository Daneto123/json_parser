// Tests without external libraries: a tiny CHECK macro and a list of cases.

#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "document.h"
#include "parser.h"
#include "path.h"
#include "shell.h"
#include "writer.h"

using namespace json;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        ++g_checks;                                                                       \
        if (!(cond)) {                                                                    \
            ++g_failures;                                                                 \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #cond "\n"; \
        }                                                                                 \
    } while (0)

#define CHECK_EQ(a, b)                                                                                   \
    do {                                                                                                 \
        ++g_checks;                                                                                      \
        const auto va_ = (a);                                                                            \
        const auto vb_ = (b);                                                                            \
        if (!(va_ == vb_)) {                                                                             \
            ++g_failures;                                                                                \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK_EQ failed: " #a " == " #b "\n  got:      " \
                      << va_ << "\n  expected: " << vb_ << "\n";                                         \
        }                                                                                                \
    } while (0)

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

std::string example(const std::string& name) { return std::string(EXAMPLES_DIR) + "/" + name; }

// Expects a parse error at line:column whose message contains `part`.
void expectError(const std::string& text, int line, int column, const std::string& part) {
    const auto err = validate(text);
    CHECK(err.has_value());
    if (!err) {
        std::cerr << "  (no error for: " << text << ")\n";
        return;
    }
    CHECK_EQ(err->line(), line);
    CHECK_EQ(err->column(), column);
    if (!contains(err->message(), part)) {
        ++g_failures;
        std::cerr << "  message \"" << err->message() << "\" does not contain \"" << part << "\"\n";
    }
}

// ------------------------------------------------------------------ parsing

void test_parses_valid_documents() {
    const Value person = parse(readTextFile(example("person.json")));
    CHECK(person.isObject());
    CHECK_EQ(resolve(person, parsePath("address/city")).asString(), std::string("New York"));
    CHECK_EQ(resolve(person, parsePath("phoneNumbers/1/number")).asString(), std::string("646 555-4567"));
    CHECK(resolve(person, parsePath("spouse")).isNull());
    CHECK(resolve(person, parsePath("isAlive")).asBool());
    CHECK_EQ(resolve(person, parsePath("age")).asNumber(), 27.0);
    CHECK_EQ(resolve(person, parsePath("children")).size(), std::size_t{0});

    const Value library = parse(readTextFile(example("library.json")));
    CHECK_EQ(resolve(library, parsePath("книги/0/автор")).asString(), std::string("Иван Вазов"));
    CHECK_EQ(resolve(library, parsePath("работно време/делник")).asString(), std::string("09:00–19:00"));
}

void test_strings_and_numbers() {
    const Value v = parse(R"(["a\"b", "line\nnext", "\u0041\u00e9\u0436", "\ud83d\ude00", "\/", -0.5e10, 1.50, 0])");
    CHECK_EQ(v.at(0).asString(), std::string("a\"b"));
    CHECK_EQ(v.at(1).asString(), std::string("line\nnext"));
    CHECK_EQ(v.at(2).asString(), std::string("Aéж"));
    CHECK_EQ(v.at(3).asString(), std::string("😀"));
    CHECK_EQ(v.at(4).asString(), std::string("/"));
    CHECK_EQ(v.at(5).asNumber(), -0.5e10);
    CHECK_EQ(v.at(6).numberText(), std::string("1.50"));  // written back exactly as in the file
    CHECK_EQ(write(v.at(6), 0), std::string("1.50"));
    CHECK_EQ(Value::number(0.1).numberText(), std::string("0.1"));
    CHECK_EQ(Value::number(42).numberText(), std::string("42"));
    CHECK(parse("\xEF\xBB\xBF{}").isObject());  // UTF-8 BOM is fine
}

void test_errors_point_at_the_problem() {
    // The three cases the 2020 validator accepted.
    expectError("{]", 1, 2, "очаквам ключ");
    expectError("{\"a\" 1}", 1, 6, "очаквам ':' след ключа \"a\"");
    expectError("{\"a\": 1,}", 1, 8, "излишна запетая преди '}'");

    expectError("[1, 2", 1, 1, "незатворен масив");
    expectError("{\"a\": 1", 1, 1, "незатворен обект");
    expectError("[1,,2]", 1, 4, "две запетаи");
    expectError("{\"a\": tru}", 1, 7, "непозната дума 'tru'");
    expectError("{\"a\": True}", 1, 7, "пише се true");
    expectError("{\"a\": hello}", 1, 7, "в кавички");
    expectError("{'a': 1}", 1, 2, "двойни кавички");
    expectError("[\"abc", 1, 2, "незатворен низ");
    expectError("[\"abc\n\"]", 1, 2, "преди края на реда");
    expectError("{\"a\":1}{\"b\":2}", 1, 8, "излишен текст");
    expectError("{\"a\":1 \"b\":2}", 1, 8, "липсва запетая");
    expectError("{\"a\":1,\"a\":2}", 1, 8, "повторен ключ \"a\"");
    expectError("{\"a\": }", 1, 7, "липсва стойност за ключа \"a\"");
    expectError("[007]", 1, 2, "започват с 0");
    expectError("[1.]", 1, 4, "след '.'");
    expectError("[\"\\x\"]", 1, 3, "невалиден escape");
    expectError("[\"\\u12G4\"]", 1, 5, "\\u");
    expectError("", 1, 1, "празен");
    expectError("// коментар\n{}", 1, 1, "коментари");
    expectError(std::string(600, '['), 1, kMaxDepth + 1, "дълбоко");

    // Columns count characters, not bytes: "име" is 3 letters (6 bytes).
    expectError("{\"име\": x}", 1, 9, "непозната дума 'x'");

    // Errors on later lines.
    const std::string broken = readTextFile(example("broken.json"));
    expectError(broken, 3, 20, "излишна запетая преди ']'");
    expectError("{\n  \"a\": 1,\n  \"b\" 2\n}", 3, 7, "очаквам ':' след ключа \"b\"");
    expectError("{\n  \"x\": [1, 2}\n}", 2, 13, "очаквам ',' или ']' в масива (отворен на ред 2)");
}

void test_error_describe() {
    const auto err = validate("{\"a\" 1}");
    CHECK(err.has_value());
    CHECK_EQ(err->describe(), std::string("ред 1, колона 6: очаквам ':' след ключа \"a\", а намерих '1'"));
}

// ------------------------------------------------------------------ writing

void test_write_round_trip() {
    for (const char* name : {"person.json", "library.json"}) {
        const std::string text = readTextFile(example(name));
        const Value v = parse(text);
        CHECK_EQ(write(v), text);  // the examples are already formatted with 2 spaces
        CHECK(parse(write(v, 0)) == v);
        CHECK(parse(write(v, 4)) == v);
    }
    CHECK_EQ(write(parse("{ \"a\" : [ 1 , { } , [ ] ] }"), 0), std::string("{\"a\":[1,{},[]]}"));
    CHECK_EQ(quote("a\"b\\c\n\t\x01"), std::string("\"a\\\"b\\\\c\\n\\t\\u0001\""));
}

void test_preview() {
    CHECK_EQ(preview(parse("[1,2,3]")), std::string("[3 елемента]"));
    CHECK_EQ(preview(parse("{\"a\":1}")), std::string("{1 ключ}"));
    CHECK_EQ(preview(Value::string("здравей")), std::string("\"здравей\""));
    const std::string longText = preview(Value::string(std::string(100, 'x')), 10);
    CHECK_EQ(longText, std::string("\"xxxxxxxxx…"));
}

// ------------------------------------------------------------------ paths

void test_path_syntax() {
    CHECK(parsePath("").empty());
    CHECK(parsePath("/").empty());
    CHECK((parsePath("/a/0/b") == Path{"a", "0", "b"}));
    CHECK((parsePath(" работно време/делник ") == Path{"работно време", "делник"}));
    CHECK((parsePath("a~1b/c~0d") == Path{"a/b", "c~d"}));
    CHECK_EQ(pathToString(Path{"a/b", "c~d", "0"}), std::string("a~1b/c~0d/0"));
    CHECK_EQ(pathToString(Path{}), std::string("/"));
}

void test_path_errors_are_clear() {
    const Value v = parse(readTextFile(example("person.json")));
    auto message = [&](const std::string& path) {
        try {
            resolve(v, parsePath(path));
        } catch (const PathError& e) {
            return std::string(e.what());
        }
        return std::string("(no error)");
    };
    CHECK(contains(message("address/town"), "няма ключ \"town\" в address"));
    CHECK(contains(message("phoneNumbers/5"), "масивът има 2"));
    CHECK(contains(message("phoneNumbers/home"), "номер на елемент"));
    CHECK(contains(message("age/x"), "age е число"));
}

void test_set_create_delete() {
    Value v = parse(readTextFile(example("person.json")));

    setAt(v, parsePath("address/city"), Value::string("Sofia"));
    CHECK_EQ(resolve(v, parsePath("address/city")).asString(), std::string("Sofia"));

    createAt(v, parsePath("email"), Value::string("john@example.com"));
    CHECK_EQ(v.keyAt(v.size() - 1), std::string("email"));  // new keys go at the end

    bool threw = false;
    try {
        createAt(v, parsePath("email"), Value::null());
    } catch (const PathError& e) {
        threw = contains(e.what(), "вече съществува");
    }
    CHECK(threw);

    createAt(v, parsePath("children/-"), Value::string("Ann"));
    createAt(v, parsePath("children/0"), Value::string("Bob"));  // insert before
    CHECK_EQ(write(resolve(v, parsePath("children")), 0), std::string("[\"Bob\",\"Ann\"]"));

    const Value removed = removeAt(v, parsePath("phoneNumbers/0"));
    CHECK_EQ(resolve(removed, parsePath("type")).asString(), std::string("home"));
    CHECK_EQ(resolve(v, parsePath("phoneNumbers")).size(), std::size_t{1});

    removeAt(v, parsePath("spouse"));
    CHECK(!exists(v, parsePath("spouse")));

    threw = false;
    try {
        removeAt(v, parsePath(""));
    } catch (const PathError&) {
        threw = true;
    }
    CHECK(threw);
}

void test_move() {
    Value v = parse(readTextFile(example("person.json")));
    moveAt(v, parsePath("address/city"), parsePath("city"));
    CHECK(!exists(v, parsePath("address/city")));
    CHECK_EQ(resolve(v, parsePath("city")).asString(), std::string("New York"));

    // Within the same array: the indexes after removal are used.
    Value a = parse("[\"a\",\"b\",\"c\"]");
    moveAt(a, parsePath("0"), parsePath("-"));
    CHECK_EQ(write(a, 0), std::string("[\"b\",\"c\",\"a\"]"));

    // A failing move changes nothing.
    const Value before = v;
    bool threw = false;
    try {
        moveAt(v, parsePath("age"), parsePath("firstName"));  // target exists
    } catch (const PathError&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(v == before);

    threw = false;
    try {
        moveAt(v, parsePath("address"), parsePath("address/inner"));
    } catch (const PathError& e) {
        threw = contains(e.what(), "самата себе си");
    }
    CHECK(threw);
}

void test_search() {
    const Value v = parse(readTextFile(example("person.json")));
    const auto numbers = searchKey(v, "number");
    CHECK_EQ(numbers.size(), std::size_t{2});
    CHECK_EQ(pathToString(numbers[0]), std::string("phoneNumbers/0/number"));

    const Value library = parse(readTextFile(example("library.json")));
    const auto sofia = searchText(library, "СОФИЯ");  // ignores case, Cyrillic too
    CHECK_EQ(sofia.size(), std::size_t{1});
    CHECK_EQ(pathToString(sofia[0]), std::string("град"));
    const auto novels = searchText(library, "роман");
    CHECK_EQ(novels.size(), std::size_t{2});
    CHECK_EQ(pathToString(novels[0]), std::string("книги/0/жанрове/0"));
    CHECK_EQ(searchText(library, "1951").size(), std::size_t{1});  // numbers too
}

// ------------------------------------------------------------------ console

void test_shell_session() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "json_parser_test";
    fs::create_directories(dir);
    const std::string copy = (dir / "person.json").string();
    const std::string other = (dir / "друг файл.json").string();
    fs::copy_file(example("person.json"), copy, fs::copy_options::overwrite_existing);

    std::ostringstream out;
    Shell shell(out);
    auto exec = [&](const std::string& line) {
        out.str("");
        const bool keepGoing = shell.execute(line);
        return std::make_pair(keepGoing, out.str());
    };

    // Nothing open yet, empty lines, bad commands: no crashes.
    CHECK(exec("").first);
    CHECK(contains(exec("print").second, "Няма отворен файл"));
    CHECK(contains(exec("frobnicate").second, "Непозната команда"));
    CHECK(contains(exec("open").second, "Използване"));
    CHECK(contains(exec("open no-such-file.json").second, "не може да се отвори"));

    CHECK(contains(exec("open " + copy).second, "Отворен"));
    CHECK(contains(exec("print address/city").second, "\"New York\""));
    CHECK(contains(exec("set address/city Sofia Center").second, "= \"Sofia Center\""));
    CHECK(contains(exec("set age 28").second, "= 28"));
    CHECK(contains(exec("create tags [\"a\", \"b\"]").second, "[2 елемента]"));
    CHECK(contains(exec("create \"home town\" Plovdiv").second, "home town"));
    CHECK(contains(exec("delete spouse").second, "Изтрито: spouse"));
    CHECK(contains(exec("move phoneNumbers/1 work").second, "Преместено"));
    CHECK(contains(exec("set nothing/here 1").second, "няма ключ \"nothing\""));
    CHECK(contains(exec("set").second, "Използване"));
    CHECK(contains(exec("search number").second, "Намерени: 2"));
    CHECK(contains(exec("find SOFIA").second, "address/city"));

    // Unsaved changes: the first exit only warns.
    auto first = exec("exit");
    CHECK(first.first);
    CHECK(contains(first.second, "незапазени промени"));

    CHECK(contains(exec("saveas " + other).second, "Записано"));
    CHECK(contains(exec("validate").second, "е валиден JSON"));
    CHECK(contains(exec("validate " + example("broken.json")).second, "ред 3, колона 20"));

    // What was written can be read back.
    const Value saved = parse(readTextFile(other));
    CHECK_EQ(resolve(saved, parsePath("address/city")).asString(), std::string("Sofia Center"));
    CHECK_EQ(resolve(saved, parsePath("age")).numberText(), std::string("28"));
    CHECK_EQ(resolve(saved, parsePath("home town")).asString(), std::string("Plovdiv"));
    CHECK_EQ(resolve(saved, parsePath("work/type")).asString(), std::string("office"));
    CHECK(!exists(saved, parsePath("spouse")));

    CHECK(contains(exec("close").second, "Затворен"));
    CHECK(!exec("exit").first);
    fs::remove_all(dir);
}

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"parses valid documents", test_parses_valid_documents},
        {"strings and numbers", test_strings_and_numbers},
        {"errors point at the problem", test_errors_point_at_the_problem},
        {"error describe", test_error_describe},
        {"write round trip", test_write_round_trip},
        {"preview", test_preview},
        {"path syntax", test_path_syntax},
        {"path errors are clear", test_path_errors_are_clear},
        {"set / create / delete", test_set_create_delete},
        {"move", test_move},
        {"search", test_search},
        {"console session", test_shell_session},
    };
    for (const auto& [name, fn] : tests) {
        const int before = g_failures;
        try {
            fn();
        } catch (const std::exception& e) {
            ++g_failures;
            std::cerr << "unexpected exception: " << e.what() << '\n';
        }
        std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << name << '\n';
    }
    std::cout << '\n' << g_checks << " checks, " << g_failures << " failed\n";
    return g_failures == 0 ? 0 : 1;
}

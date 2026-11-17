#pragma once

#include <ostream>
#include <string>

#include "document.h"

namespace json {

// The console commands. Reads one line at a time, writes the answers to `out`.
// Kept apart from main() so the tests can drive it with strings.
class Shell {
public:
    explicit Shell(std::ostream& out) : out_(out) {}

    // Runs one command line. Returns false after "exit".
    bool execute(const std::string& line);

    void printHelp();
    const Document& document() const { return doc_; }

private:
    bool requireOpen();
    bool confirmDiscard(const std::string& command);

    void cmdOpen(const std::string& file);
    void cmdClose();
    void cmdSave();
    void cmdSaveAs(const std::string& file);
    void cmdValidate(const std::string& file);
    void cmdPrint(const std::string& path);
    void cmdSearch(const std::string& key, bool byText);
    void cmdSet(const std::string& args, bool create);
    void cmdDelete(const std::string& path);
    void cmdMove(const std::string& args);

    std::ostream& out_;
    Document doc_;
    std::string pendingDiscard_;  // command that was warned about unsaved changes
};

}  // namespace json

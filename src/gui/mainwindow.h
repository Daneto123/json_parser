#pragma once

#include <QMainWindow>

#include <functional>
#include <optional>
#include <vector>

#include "parser.h"
#include "path.h"
#include "value.h"

class CodeEditor;
class JsonTree;
class QLabel;
class QLineEdit;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    bool openFile(const QString& path);
    void setText(const QString& text);  // for a new document or tests

    bool isValid() const { return !error_.has_value(); }
    const json::Value& document() const { return root_; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void buildUi();
    void buildActions();

    void newFile();
    void open();
    bool save();
    bool saveAs();
    bool writeTo(const QString& path);
    bool maybeSave();

    void validateNow();
    void showDocument();
    void format(int indent);

    // Tree operations: change root_, then rewrite the editor text.
    void applyChange(const std::function<void(json::Value&)>& change, const QString& done);
    void addValue();
    void editValue();
    void deleteValue();
    void moveValue();
    void onValueEdited(const json::Path& path, const QString& text);
    void onKeyRenamed(const json::Path& path, const QString& key);
    void applyEdit(const json::Path& path, const QString& text);
    void applyRename(const json::Path& path, const QString& key);

    void search();
    void nextMatch();

    void updateTitle();
    QString selectedPathText() const;

    CodeEditor* editor_;
    JsonTree* tree_;
    QLineEdit* search_;
    QLabel* searchInfo_;
    QLabel* status_;
    QTimer* validateTimer_;

    QString path_;
    json::Value root_;  // last valid document
    std::optional<json::ParseError> error_;
    std::vector<json::Path> matches_;
    std::size_t matchIndex_ = 0;
    QList<QAction*> needsValid_;
};

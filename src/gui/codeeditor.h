#pragma once

#include <QPlainTextEdit>

// Text editor with line numbers and a marked error line.
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);

    // Marks (or clears, with line 0) the line and character of a syntax error.
    void setErrorAt(int line, int column);
    void clearError() { setErrorAt(0, 0); }

    // Moves the cursor to line:column (both from 1) and shows it.
    void goTo(int line, int column);

    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent* event);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateMargins();
    void updateLineNumberArea(const QRect& rect, int dy);
    void refreshSelections();

    QWidget* lineNumberArea_;
    int errorLine_ = 0;
    int errorColumn_ = 0;
};

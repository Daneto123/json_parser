#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Colours for keys, strings, numbers, true/false/null and brackets.
class JsonHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit JsonHighlighter(QTextDocument* document);

protected:
    void highlightBlock(const QString& text) override;

private:
    QRegularExpression string_;
    QRegularExpression number_;
    QRegularExpression keyword_;
    QTextCharFormat keyFormat_;
    QTextCharFormat stringFormat_;
    QTextCharFormat numberFormat_;
    QTextCharFormat keywordFormat_;
    QTextCharFormat punctuationFormat_;
};

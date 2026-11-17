#include "highlighter.h"

JsonHighlighter::JsonHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document),
      string_(QStringLiteral(R"("(?:[^"\\]|\\.)*"?)")),
      number_(QStringLiteral(R"(-?\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b)")),
      keyword_(QStringLiteral(R"(\b(?:true|false|null)\b)")) {
    keyFormat_.setForeground(QColor(0x7c, 0x3a, 0xed));
    keyFormat_.setFontWeight(QFont::DemiBold);
    stringFormat_.setForeground(QColor(0x15, 0x80, 0x3d));
    numberFormat_.setForeground(QColor(0xc2, 0x41, 0x0c));
    keywordFormat_.setForeground(QColor(0x25, 0x63, 0xeb));
    keywordFormat_.setFontWeight(QFont::Bold);
    punctuationFormat_.setForeground(QColor(0x64, 0x74, 0x8b));
}

void JsonHighlighter::highlightBlock(const QString& text) {
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text[i];
        if (c == QLatin1Char('{') || c == QLatin1Char('}') || c == QLatin1Char('[') || c == QLatin1Char(']') ||
            c == QLatin1Char(':') || c == QLatin1Char(',')) {
            setFormat(i, 1, punctuationFormat_);
        }
    }

    auto it = number_.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), numberFormat_);
    }
    it = keyword_.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), keywordFormat_);
    }

    // Strings last, so numbers and words inside them are not coloured.
    it = string_.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        int after = m.capturedEnd();
        while (after < text.size() && text[after].isSpace()) ++after;
        const bool isKey = after < text.size() && text[after] == QLatin1Char(':');
        setFormat(m.capturedStart(), m.capturedLength(), isKey ? keyFormat_ : stringFormat_);
    }
}

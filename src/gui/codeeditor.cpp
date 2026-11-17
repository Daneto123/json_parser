#include "codeeditor.h"

#include <algorithm>

#include <QFontDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QTextBlock>

namespace {

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), editor_(editor) {}
    QSize sizeHint() const override { return {editor_->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent* event) override { editor_->paintLineNumbers(event); }

private:
    CodeEditor* editor_;
};

const QColor kErrorLine(0xfe, 0xe2, 0xe2);
const QColor kErrorMark(0xdc, 0x26, 0x26);

}  // namespace

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent), lineNumberArea_(new LineNumberArea(this)) {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(std::max(font.pointSize(), 11));
    setFont(font);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 2);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateMargins);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::refreshSelections);
    updateMargins();
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    for (int max = std::max(1, blockCount()); max >= 10; max /= 10) ++digits;
    return 14 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * std::max(digits, 3);
}

void CodeEditor::updateMargins() { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) lineNumberArea_->scroll(0, dy);
    else lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateMargins();
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::paintLineNumbers(QPaintEvent* event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

    QTextBlock block = firstVisibleBlock();
    int number = block.blockNumber() + 1;
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int current = textCursor().blockNumber() + 1;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (number == errorLine_) {
                painter.fillRect(0, top, lineNumberArea_->width(), bottom - top, kErrorMark);
                painter.setPen(Qt::white);
            } else {
                painter.setPen(number == current ? palette().color(QPalette::Text)
                                                 : palette().color(QPalette::PlaceholderText));
            }
            painter.drawText(0, top, lineNumberArea_->width() - 6, fontMetrics().height(), Qt::AlignRight,
                             QString::number(number));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++number;
    }
}

void CodeEditor::setErrorAt(int line, int column) {
    errorLine_ = line;
    errorColumn_ = column;
    refreshSelections();
    lineNumberArea_->update();
}

void CodeEditor::refreshSelections() {
    QList<QTextEdit::ExtraSelection> selections;

    if (errorLine_ > 0) {
        const QTextBlock block = document()->findBlockByNumber(errorLine_ - 1);
        if (block.isValid()) {
            QTextEdit::ExtraSelection line;
            line.format.setBackground(kErrorLine);
            line.format.setProperty(QTextFormat::FullWidthSelection, true);
            line.cursor = QTextCursor(block);
            selections.append(line);

            // The character where the error is (or the end of the line).
            QTextEdit::ExtraSelection mark;
            mark.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            mark.format.setUnderlineColor(kErrorMark);
            mark.format.setBackground(QColor(0xfc, 0xa5, 0xa5));
            QTextCursor c(block);
            const int pos = std::min(errorColumn_ - 1, std::max(0, block.length() - 1));
            c.setPosition(block.position() + std::max(0, pos));
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            mark.cursor = c;
            selections.append(mark);
        }
    }
    setExtraSelections(selections);
    lineNumberArea_->update();
}

void CodeEditor::goTo(int line, int column) {
    const QTextBlock block = document()->findBlockByNumber(std::max(0, line - 1));
    if (!block.isValid()) return;
    QTextCursor c(block);
    c.setPosition(block.position() + std::clamp(column - 1, 0, std::max(0, block.length() - 1)));
    setTextCursor(c);
    centerCursor();
    setFocus();
}

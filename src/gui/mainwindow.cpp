#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "codeeditor.h"
#include "document.h"
#include "highlighter.h"
#include "jsontree.h"
#include "writer.h"

using json::Path;
using json::Value;

namespace {

const QString kFilter = QObject::tr("JSON файлове (*.json);;Всички файлове (*)");

QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QString& s) { return s.toStdString(); }

const char* kSample = R"({
  "name": "Нов документ",
  "items": [1, 2, 3],
  "ok": true
}
)";

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    validateTimer_ = new QTimer(this);
    validateTimer_->setSingleShot(true);
    validateTimer_->setInterval(250);
    connect(validateTimer_, &QTimer::timeout, this, &MainWindow::validateNow);

    buildUi();
    buildActions();
    setAcceptDrops(true);

    QSettings settings;
    if (!restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray())) resize(1200, 720);

    setText(QString::fromUtf8(kSample));
}

void MainWindow::buildUi() {
    editor_ = new CodeEditor;
    new JsonHighlighter(editor_->document());
    connect(editor_, &QPlainTextEdit::textChanged, validateTimer_, qOverload<>(&QTimer::start));
    connect(editor_->document(), &QTextDocument::modificationChanged, this, &MainWindow::updateTitle);

    tree_ = new JsonTree;
    connect(tree_, &JsonTree::valueEdited, this, &MainWindow::onValueEdited);
    connect(tree_, &JsonTree::keyRenamed, this, &MainWindow::onKeyRenamed);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this] {
        statusBar()->showMessage(tr("Път: %1").arg(selectedPathText()));
    });

    search_ = new QLineEdit;
    search_->setPlaceholderText(tr("Търсене в ключове и стойности… (Enter = следващо)"));
    search_->setClearButtonEnabled(true);
    connect(search_, &QLineEdit::textChanged, this, &MainWindow::search);
    connect(search_, &QLineEdit::returnPressed, this, &MainWindow::nextMatch);
    searchInfo_ = new QLabel;
    searchInfo_->setMinimumWidth(90);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(search_, 1);
    searchRow->addWidget(searchInfo_);

    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(searchRow);
    rightLayout->addWidget(tree_, 1);

    auto* splitter = new QSplitter;
    splitter->addWidget(editor_);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    status_ = new QLabel;
    status_->setTextFormat(Qt::RichText);
    status_->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(status_, &QLabel::linkActivated, this, [this] {
        if (error_) editor_->goTo(error_->line(), error_->column());
    });
    statusBar()->addPermanentWidget(status_, 1);
}

void MainWindow::buildActions() {
    auto* style = this->style();
    auto make = [this](const QIcon& icon, const QString& text, const QKeySequence& keys, auto slot) {
        auto* action = new QAction(icon, text, this);
        action->setShortcut(keys);
        connect(action, &QAction::triggered, this, slot);
        return action;
    };

    auto* newAct = make(style->standardIcon(QStyle::SP_FileIcon), tr("Нов"), QKeySequence::New, [this] { newFile(); });
    auto* openAct = make(style->standardIcon(QStyle::SP_DialogOpenButton), tr("Отвори…"), QKeySequence::Open,
                         [this] { open(); });
    auto* saveAct = make(style->standardIcon(QStyle::SP_DialogSaveButton), tr("Запази"), QKeySequence::Save,
                         [this] { save(); });
    auto* saveAsAct = make(QIcon(), tr("Запази като…"), QKeySequence::SaveAs, [this] { saveAs(); });
    auto* quitAct = make(QIcon(), tr("Изход"), QKeySequence::Quit, [this] { close(); });

    auto* formatAct = make(style->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Форматирай"),
                           QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), [this] { format(2); });
    formatAct->setToolTip(tr("Подрежда отстъпите (Ctrl+Shift+F)"));
    auto* compactAct = make(QIcon(), tr("Компактен (един ред)"), QKeySequence(), [this] { format(0); });

    auto* addAct = make(style->standardIcon(QStyle::SP_FileDialogNewFolder), tr("Добави…"),
                        QKeySequence(Qt::CTRL | Qt::Key_Plus), [this] { addValue(); });
    auto* editAct = make(QIcon(), tr("Промени стойност…"), QKeySequence(Qt::Key_F2), [this] { editValue(); });
    auto* deleteAct = make(style->standardIcon(QStyle::SP_TrashIcon), tr("Изтрий"), QKeySequence(),
                           [this] { deleteValue(); });
    auto* moveAct = make(style->standardIcon(QStyle::SP_ArrowRight), tr("Премести…"), QKeySequence(),
                         [this] { moveValue(); });
    auto* findAct = make(QIcon(), tr("Търси"), QKeySequence::Find, [this] {
        search_->setFocus();
        search_->selectAll();
    });

    // Delete works on the tree only, so it does not steal Del from the editor.
    auto* treeDelete = new QAction(this);
    treeDelete->setShortcut(QKeySequence::Delete);
    treeDelete->setShortcutContext(Qt::WidgetShortcut);
    connect(treeDelete, &QAction::triggered, this, &MainWindow::deleteValue);
    tree_->addAction(treeDelete);

    needsValid_ = {formatAct, compactAct, addAct, editAct, deleteAct, moveAct};

    auto* file = menuBar()->addMenu(tr("&Файл"));
    file->addActions({newAct, openAct, saveAct, saveAsAct});
    file->addSeparator();
    file->addAction(quitAct);

    auto* edit = menuBar()->addMenu(tr("&Редактиране"));
    edit->addActions({formatAct, compactAct});
    edit->addSeparator();
    edit->addActions({addAct, editAct, deleteAct, moveAct});
    edit->addSeparator();
    edit->addAction(findAct);

    auto* help = menuBar()->addMenu(tr("&Помощ"));
    help->addAction(make(QIcon(), tr("За програмата"), QKeySequence(), [this] {
        QMessageBox::about(this, tr("JSON парсер"),
                           tr("<h3>JSON парсер 2.0</h3>"
                              "<p>Проверява JSON и показва точно къде е грешката, показва документа като "
                              "дърво и позволява да се променя по път (напр. <code>address/city</code>).</p>"
                              "<p>Първата версия е писана по Обектно-ориентирано програмиране, "
                              "летен семестър 2019/2020.</p>"));
    }));

    auto* toolbar = addToolBar(tr("Действия"));
    toolbar->setObjectName(QStringLiteral("actions"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addActions({newAct, openAct, saveAct});
    toolbar->addSeparator();
    toolbar->addAction(formatAct);
    toolbar->addSeparator();
    toolbar->addActions({addAct, deleteAct, moveAct});

    tree_->setContextMenuPolicy(Qt::ActionsContextMenu);
    tree_->addActions({addAct, editAct, deleteAct, moveAct});
}

// ------------------------------------------------------------------ files

void MainWindow::setText(const QString& text) {
    // A new document: the tree must not keep showing the previous one.
    root_ = Value();
    tree_->clear();
    editor_->setPlainText(text);
    editor_->document()->setModified(false);
    validateNow();
    updateTitle();
}

bool MainWindow::openFile(const QString& path) {
    std::string text;
    try {
        text = json::readTextFile(ss(path));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Отваряне"), QString::fromUtf8(e.what()));
        return false;
    }
    path_ = path;
    // Invalid files open too: the error is shown and can be fixed here.
    setText(QString::fromUtf8(text.data(), static_cast<int>(text.size())));
    if (error_) editor_->goTo(error_->line(), error_->column());
    QSettings().setValue(QStringLiteral("lastDir"), QFileInfo(path).absolutePath());
    return true;
}

void MainWindow::newFile() {
    if (!maybeSave()) return;
    path_.clear();
    setText(QString::fromUtf8(kSample));
}

void MainWindow::open() {
    if (!maybeSave()) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Отвори JSON"),
                                                      QSettings().value(QStringLiteral("lastDir")).toString(), kFilter);
    if (!path.isEmpty()) openFile(path);
}

bool MainWindow::save() { return path_.isEmpty() ? saveAs() : writeTo(path_); }

bool MainWindow::saveAs() {
    QString suggested = path_.isEmpty() ? QSettings().value(QStringLiteral("lastDir")).toString() : path_;
    const QString path = QFileDialog::getSaveFileName(this, tr("Запази като"), suggested, kFilter);
    if (path.isEmpty()) return false;
    return writeTo(path);
}

bool MainWindow::writeTo(const QString& path) {
    validateNow();
    if (error_) {
        const auto answer = QMessageBox::question(
            this, tr("Запис"),
            tr("Документът има грешка (%1).\nДа се запише ли въпреки това?").arg(qs(error_->describe())));
        if (answer != QMessageBox::Yes) return false;
    }
    try {
        json::writeTextFile(ss(path), ss(editor_->toPlainText()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Запис"), QString::fromUtf8(e.what()));
        return false;
    }
    path_ = path;
    editor_->document()->setModified(false);
    updateTitle();
    statusBar()->showMessage(tr("Записано в %1").arg(QDir::toNativeSeparators(path)), 4000);
    return true;
}

bool MainWindow::maybeSave() {
    if (!editor_->document()->isModified()) return true;
    const auto answer = QMessageBox::warning(this, tr("Незапазени промени"),
                                             tr("Документът е променен. Да се запишат ли промените?"),
                                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) return save();
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    QSettings().setValue(QStringLiteral("geometry"), saveGeometry());
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty() && urls.first().isLocalFile() && maybeSave()) openFile(urls.first().toLocalFile());
}

// ------------------------------------------------------------------ validation

void MainWindow::validateNow() {
    validateTimer_->stop();
    const std::string text = ss(editor_->toPlainText());
    try {
        root_ = json::parse(text);
        error_.reset();
    } catch (const json::ParseError& e) {
        error_ = e;
    }

    if (error_) {
        editor_->setErrorAt(error_->line(), error_->column());
        status_->setText(tr("<span style='color:#b91c1c'><b>✗ Грешка</b> — <a href='#'>ред %1, колона %2</a>: %3</span>")
                             .arg(error_->line())
                             .arg(error_->column())
                             .arg(qs(error_->message()).toHtmlEscaped()));
        tree_->setEnabled(false);
        tree_->setToolTip(tr("Показва последната валидна версия. Поправете грешката, за да се обнови."));
    } else {
        editor_->clearError();
        status_->setText(tr("<span style='color:#15803d'><b>✓ Валиден JSON</b></span>"));
        tree_->setEnabled(true);
        tree_->setToolTip(QString());
        showDocument();
    }
    for (auto* a : needsValid_) a->setEnabled(!error_);
}

void MainWindow::showDocument() {
    tree_->showDocument(root_);
    search();
}

void MainWindow::format(int indent) {
    if (error_) return;
    // Through the cursor, so Ctrl+Z can undo it.
    QTextCursor cursor(editor_->document());
    cursor.select(QTextCursor::Document);
    cursor.insertText(qs(json::write(root_, indent)));
    validateNow();
}

// ------------------------------------------------------------------ editing by path

void MainWindow::applyChange(const std::function<void(Value&)>& change, const QString& done) {
    if (error_) return;
    Value copy = root_;
    try {
        change(copy);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Не може"), QString::fromUtf8(e.what()));
        showDocument();  // undo an in-place edit in the tree
        return;
    }
    QTextCursor cursor(editor_->document());
    cursor.select(QTextCursor::Document);
    cursor.insertText(qs(json::write(copy, 2)));
    validateNow();
    statusBar()->showMessage(done, 4000);
}

QString MainWindow::selectedPathText() const {
    const auto p = tree_->selectedPath();
    return p ? qs(json::pathToString(*p)) : QString();
}

void MainWindow::addValue() {
    QString base = selectedPathText();
    if (const auto p = tree_->selectedPath()) {
        // Suggest a new key inside the selected object/array, or next to a leaf.
        const Value& v = json::resolve(root_, *p);
        Path parent = v.isContainer() ? *p : Path(p->begin(), p->end() - (p->empty() ? 0 : 1));
        const Value& pv = json::resolve(root_, parent);
        base = qs(json::pathToString(parent));
        base = (base == QStringLiteral("/") ? QString() : base + QLatin1Char('/')) +
               (pv.isArray() ? QStringLiteral("-") : tr("нов_ключ"));
    }
    bool ok = false;
    const QString pathText = QInputDialog::getText(this, tr("Добави"),
                                                   tr("Път на новата стойност (\"-\" в масив = в края):"),
                                                   QLineEdit::Normal, base, &ok);
    if (!ok || pathText.trimmed().isEmpty()) return;
    const QString valueText =
        QInputDialog::getText(this, tr("Добави"), tr("Стойност (JSON или текст):"), QLineEdit::Normal, QString(), &ok);
    if (!ok) return;
    const Path path = json::parsePath(ss(pathText));
    applyChange([&](Value& root) { json::createAt(root, path, json::parseValueOrString(ss(valueText))); },
                tr("Добавено: %1").arg(pathText));
    if (path.back() != "-") tree_->selectPath(path);
}

void MainWindow::editValue() {
    const auto p = tree_->selectedPath();
    if (!p) {
        QMessageBox::information(this, tr("Промяна"), tr("Изберете стойност в дървото."));
        return;
    }
    bool ok = false;
    const QString current = qs(json::write(json::resolve(root_, *p), 0));
    const QString text = QInputDialog::getText(this, tr("Промени %1").arg(selectedPathText()),
                                               tr("Нова стойност (JSON или текст):"), QLineEdit::Normal, current, &ok);
    if (!ok) return;
    const Path path = *p;
    applyChange([&](Value& root) { json::setAt(root, path, json::parseValueOrString(ss(text))); },
                tr("Променено: %1").arg(qs(json::pathToString(path))));
    tree_->selectPath(path);
}

void MainWindow::deleteValue() {
    const auto p = tree_->selectedPath();
    if (!p || p->empty()) return;
    const QString name = selectedPathText();
    const auto answer = QMessageBox::question(this, tr("Изтриване"), tr("Да се изтрие ли %1?").arg(name));
    if (answer != QMessageBox::Yes) return;
    const Path path = *p;
    applyChange([&](Value& root) { json::removeAt(root, path); }, tr("Изтрито: %1").arg(name));
}

void MainWindow::moveValue() {
    bool ok = false;
    const QString from = QInputDialog::getText(this, tr("Премести"), tr("От път:"), QLineEdit::Normal,
                                               selectedPathText(), &ok);
    if (!ok || from.trimmed().isEmpty()) return;
    const QString to =
        QInputDialog::getText(this, tr("Премести"), tr("Към нов път:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || to.trimmed().isEmpty()) return;
    const Path target = json::parsePath(ss(to));
    applyChange([&](Value& root) { json::moveAt(root, json::parsePath(ss(from)), target); },
                tr("Преместено: %1 → %2").arg(from, to));
    tree_->selectPath(target);
}

void MainWindow::onValueEdited(const Path& path, const QString& text) {
    // Later, not inside the tree's own signal: applying rebuilds the tree.
    QTimer::singleShot(0, this, [this, path, text] { applyEdit(path, text); });
}

void MainWindow::applyEdit(const Path& path, const QString& text) {
    applyChange([&](Value& root) { json::setAt(root, path, json::parseValueOrString(ss(text))); },
                tr("Променено: %1").arg(qs(json::pathToString(path))));
    tree_->selectPath(path);
}

void MainWindow::onKeyRenamed(const Path& path, const QString& key) {
    QTimer::singleShot(0, this, [this, path, key] { applyRename(path, key); });
}

void MainWindow::applyRename(const Path& path, const QString& key) {
    if (path.empty() || ss(key) == path.back()) return;
    Path target = path;
    target.back() = ss(key);
    applyChange(
        [&](Value& root) {
            Value& parent = json::resolve(root, Path(path.begin(), path.end() - 1));
            if (key.isEmpty()) throw json::PathError("ключът не може да е празен");
            if (parent.find(target.back())) throw json::PathError("вече има ключ \"" + target.back() + "\"");
            parent.renameKey(static_cast<std::size_t>(parent.indexOf(path.back())), target.back());
        },
        tr("Преименувано на „%1“").arg(key));
    tree_->selectPath(target);
}

// ------------------------------------------------------------------ search

void MainWindow::search() {
    const QString text = search_->text().trimmed();
    matches_ = text.isEmpty() ? std::vector<Path>{} : json::searchText(root_, ss(text));
    matchIndex_ = 0;
    tree_->highlight(matches_);
    if (text.isEmpty()) searchInfo_->clear();
    else if (matches_.empty()) searchInfo_->setText(tr("няма"));
    else searchInfo_->setText(matches_.size() == 1 ? tr("1 намерен") : tr("%1 намерени").arg(matches_.size()));
}

void MainWindow::nextMatch() {
    if (matches_.empty()) return;
    tree_->selectPath(matches_[matchIndex_ % matches_.size()]);
    searchInfo_->setText(tr("%1 от %2").arg(matchIndex_ % matches_.size() + 1).arg(matches_.size()));
    ++matchIndex_;
}

void MainWindow::updateTitle() {
    const QString name = path_.isEmpty() ? tr("без име") : QFileInfo(path_).fileName();
    setWindowTitle(QStringLiteral("%1%2 — JSON парсер").arg(editor_->document()->isModified() ? QStringLiteral("*") : QString(), name));
}

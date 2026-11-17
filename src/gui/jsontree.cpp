#include "jsontree.h"

#include <QHeaderView>

#include "writer.h"

using json::Path;
using json::Value;

QString pathKey(const Path& path) { return QString::fromStdString(json::pathToString(path)); }

namespace {

const QColor kMatch(0xfe, 0xf0, 0x8a);

QString typeLabel(const Value& v) { return QString::fromUtf8(json::typeName(v.type())); }

QColor typeColor(const Value& v) {
    switch (v.type()) {
        case Value::Type::String: return {0x15, 0x80, 0x3d};
        case Value::Type::Number: return {0xc2, 0x41, 0x0c};
        case Value::Type::Bool:
        case Value::Type::Null: return {0x25, 0x63, 0xeb};
        default: return {0x64, 0x74, 0x8b};
    }
}

}  // namespace

JsonTree::JsonTree(QWidget* parent) : QTreeWidget(parent) {
    setColumnCount(3);
    setHeaderLabels({tr("Ключ"), tr("Стойност"), tr("Тип")});
    header()->setSectionResizeMode(KeyColumn, QHeaderView::Interactive);
    header()->setSectionResizeMode(ValueColumn, QHeaderView::Stretch);
    header()->setSectionResizeMode(TypeColumn, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    setColumnWidth(KeyColumn, 200);
    setAlternatingRowColors(true);
    setUniformRowHeights(true);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    connect(this, &QTreeWidget::itemChanged, this, &JsonTree::onItemChanged);
}

QTreeWidgetItem* JsonTree::addItem(QTreeWidgetItem* parent, const QString& key, const Value& value,
                                   const Path& path, bool keyEditable) {
    auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);
    item->setText(KeyColumn, key);
    item->setText(TypeColumn, typeLabel(value));
    item->setData(KeyColumn, PathRole, pathKey(path));
    item->setData(KeyColumn, KeyEditableRole, keyEditable);
    item->setData(KeyColumn, ContainerRole, value.isContainer());
    item->setForeground(ValueColumn, typeColor(value));
    item->setForeground(TypeColumn, palette().color(QPalette::PlaceholderText));

    // Editable: leaf values, and the keys of object members (to rename them).
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (!value.isContainer() || keyEditable) flags |= Qt::ItemIsEditable;
    item->setFlags(flags);

    if (value.isContainer()) {
        item->setText(ValueColumn, QString::fromStdString(json::preview(value)));
        item->setForeground(ValueColumn, palette().color(QPalette::PlaceholderText));
        for (std::size_t i = 0; i < value.size(); ++i) {
            Path child = path;
            if (value.isObject()) {
                child.push_back(value.keyAt(i));
                addItem(item, QString::fromStdString(value.keyAt(i)), value.at(i), child, true);
            } else {
                child.push_back(std::to_string(i));
                addItem(item, QStringLiteral("[%1]").arg(i), value.at(i), child, false);
            }
        }
    } else {
        item->setText(ValueColumn, QString::fromStdString(json::write(value, 0)));
    }
    return item;
}

void JsonTree::showDocument(const Value& root) {
    // Remember what was open and selected.
    QSet<QString> expanded;
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        if ((*it)->isExpanded()) expanded.insert((*it)->data(KeyColumn, PathRole).toString());
    }
    const bool firstTime = topLevelItemCount() == 0;
    const auto selected = selectedPath();

    rebuilding_ = true;
    clear();
    addItem(nullptr, tr("(документ)"), root, {}, false);

    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        const QString key = (*it)->data(KeyColumn, PathRole).toString();
        // First time: open the document and its first level.
        const int depth = key == QStringLiteral("/") ? 0 : static_cast<int>(key.count(QLatin1Char('/'))) + 1;
        if (firstTime ? depth <= 1 : expanded.contains(key)) (*it)->setExpanded(true);
    }
    rebuilding_ = false;

    if (selected) selectPath(*selected);
}

QTreeWidgetItem* JsonTree::itemFor(const Path& path) const {
    const QString key = pathKey(path);
    for (QTreeWidgetItemIterator it(const_cast<JsonTree*>(this)); *it; ++it) {
        if ((*it)->data(KeyColumn, PathRole).toString() == key) return *it;
    }
    return nullptr;
}

std::optional<Path> JsonTree::selectedPath() const {
    const auto items = selectedItems();
    if (items.isEmpty()) return std::nullopt;
    return json::parsePath(items.first()->data(KeyColumn, PathRole).toString().toStdString());
}

void JsonTree::selectPath(const Path& path) {
    QTreeWidgetItem* item = itemFor(path);
    if (!item) return;
    for (auto* p = item->parent(); p; p = p->parent()) p->setExpanded(true);
    setCurrentItem(item);
    scrollToItem(item);
}

void JsonTree::highlight(const std::vector<Path>& paths) {
    QSet<QString> keys;
    for (const auto& p : paths) keys.insert(pathKey(p));
    rebuilding_ = true;
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        const bool match = keys.contains((*it)->data(KeyColumn, PathRole).toString());
        for (int c = 0; c < columnCount(); ++c) (*it)->setBackground(c, match ? QBrush(kMatch) : QBrush());
        if (match) {
            for (auto* p = (*it)->parent(); p; p = p->parent()) p->setExpanded(true);
        }
    }
    rebuilding_ = false;
}

bool JsonTree::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) {
    if (trigger & (QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed)) {
        const QModelIndex keyIndex = index.siblingAtColumn(KeyColumn);
        if (index.column() == TypeColumn) return false;
        if (index.column() == KeyColumn && !keyIndex.data(KeyEditableRole).toBool()) return false;
        if (index.column() == ValueColumn && keyIndex.data(ContainerRole).toBool()) return false;
    }
    return QTreeWidget::edit(index, trigger, event);
}

void JsonTree::onItemChanged(QTreeWidgetItem* item, int column) {
    if (rebuilding_) return;
    const Path path = json::parsePath(item->data(KeyColumn, PathRole).toString().toStdString());
    if (column == ValueColumn) emit valueEdited(path, item->text(ValueColumn));
    else if (column == KeyColumn) emit keyRenamed(path, item->text(KeyColumn));
}

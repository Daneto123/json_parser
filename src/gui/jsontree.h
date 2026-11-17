#pragma once

#include <QSet>
#include <QTreeWidget>

#include "path.h"
#include "value.h"

// The document as a tree: key, value and type. Values (and object keys) can be
// edited in place with a double click.
class JsonTree : public QTreeWidget {
    Q_OBJECT

public:
    enum Column { KeyColumn, ValueColumn, TypeColumn };
    static constexpr int PathRole = Qt::UserRole + 1;
    static constexpr int KeyEditableRole = Qt::UserRole + 2;
    static constexpr int ContainerRole = Qt::UserRole + 3;

    explicit JsonTree(QWidget* parent = nullptr);

    // Rebuilds the tree, keeping expanded branches and the selection.
    void showDocument(const json::Value& root);

    // Selected item's path (empty path = root; nullopt = nothing selected).
    std::optional<json::Path> selectedPath() const;
    void selectPath(const json::Path& path);

    // Marks the given paths (search results).
    void highlight(const std::vector<json::Path>& paths);

protected:
    // Only leaf values and object keys can be edited, not "{3 ключа}" or "[0]".
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

signals:
    void valueEdited(const json::Path& path, const QString& newText);
    void keyRenamed(const json::Path& path, const QString& newKey);

private:
    QTreeWidgetItem* addItem(QTreeWidgetItem* parent, const QString& key, const json::Value& value,
                             const json::Path& path, bool keyEditable);
    QTreeWidgetItem* itemFor(const json::Path& path) const;
    void onItemChanged(QTreeWidgetItem* item, int column);

    bool rebuilding_ = false;
};

QString pathKey(const json::Path& path);  // stable text form for QSet / item data

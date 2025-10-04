#pragma once

#include <QWidget>

class QLineEdit;
class QTextEdit;
class StoryNode;

class NodeInspectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NodeInspectorWidget(QWidget *parent = nullptr);

    void setNode(StoryNode *node);

signals:
    void nodeUpdated(const QString &nodeId);

private slots:
    void onTitleEdited(const QString &text);
    void onScriptEdited();

private:
    void refresh();

    StoryNode *m_node{nullptr};
    QLineEdit *m_titleEdit{nullptr};
    QTextEdit *m_scriptEdit{nullptr};
};

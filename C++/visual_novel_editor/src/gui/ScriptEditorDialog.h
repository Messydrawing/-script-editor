#pragma once

#include <QDialog>

class QTextEdit;
class StoryNode;

class ScriptEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScriptEditorDialog(StoryNode *node, QWidget *parent = nullptr);

protected:
    void accept() override;

private:
    StoryNode *m_node{nullptr};
    QTextEdit *m_editor{nullptr};
};

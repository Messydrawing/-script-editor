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
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();

    StoryNode *m_node{nullptr};
    QTextEdit *m_editor{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};

#include "ScriptEditorDialog.h"

#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include "model/StoryNode.h"

ScriptEditorDialog::ScriptEditorDialog(StoryNode *node, QWidget *parent)
    : QDialog(parent)
    , m_node(node)
    , m_editor(new QTextEdit(this))
{
    setWindowTitle(tr("Script Editor"));
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ScriptEditorDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ScriptEditorDialog::reject);

    if (m_node) {
        m_editor->setPlainText(m_node->script());
    }
}

void ScriptEditorDialog::accept()
{
    if (m_node) {
        m_node->setScript(m_editor->toPlainText());
    }
    QDialog::accept();
}

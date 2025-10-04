#include "ScriptEditorDialog.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <Qt>

#include "model/StoryNode.h"

ScriptEditorDialog::ScriptEditorDialog(StoryNode *node, QWidget *parent)
    : QDialog(parent)
    , m_node(node)
    , m_editor(new QTextEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ScriptEditorDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ScriptEditorDialog::reject);

    if (m_node) {
        const QString script = m_node->script();
        if (Qt::mightBeRichText(script)) {
            m_editor->setHtml(script);
        } else {
            m_editor->setPlainText(script);
        }
    }

    retranslateUi();
}

void ScriptEditorDialog::accept()
{
    if (m_node) {
        m_node->setScript(m_editor->toHtml());
    }
    QDialog::accept();
}

void ScriptEditorDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void ScriptEditorDialog::retranslateUi()
{
    setWindowTitle(tr("Script Editor"));
    if (m_buttonBox) {
        if (QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok)) {
            okButton->setText(tr("OK"));
        }
        if (QPushButton *cancelButton = m_buttonBox->button(QDialogButtonBox::Cancel)) {
            cancelButton->setText(tr("Cancel"));
        }
    }
}

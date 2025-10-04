#include "NodeInspectorWidget.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTextEdit>

#include "model/StoryNode.h"

NodeInspectorWidget::NodeInspectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_titleEdit(new QLineEdit(this))
    , m_scriptEdit(new QTextEdit(this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(tr("Title"), m_titleEdit);
    layout->addRow(tr("Script"), m_scriptEdit);

    connect(m_titleEdit, &QLineEdit::textEdited, this, &NodeInspectorWidget::onTitleEdited);
    connect(m_scriptEdit, &QTextEdit::textChanged, this, &NodeInspectorWidget::onScriptEdited);
}

void NodeInspectorWidget::setNode(StoryNode *node)
{
    m_node = node;
    refresh();
}

void NodeInspectorWidget::onTitleEdited(const QString &text)
{
    if (!m_node) {
        return;
    }
    m_node->setTitle(text);
    emit nodeUpdated(m_node->id());
}

void NodeInspectorWidget::onScriptEdited()
{
    if (!m_node) {
        return;
    }
    m_node->setScript(m_scriptEdit->toPlainText());
    emit nodeUpdated(m_node->id());
}

void NodeInspectorWidget::refresh()
{
    const QSignalBlocker blocker1(m_titleEdit);
    const QSignalBlocker blocker2(m_scriptEdit);
    if (m_node) {
        m_titleEdit->setText(m_node->title());
        m_scriptEdit->setPlainText(m_node->script());
    } else {
        m_titleEdit->clear();
        m_scriptEdit->clear();
    }
}

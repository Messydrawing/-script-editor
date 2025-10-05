#include "NodeInspectorWidget.h"

#include <QAction>
#include <cmath>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSize>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <Qt>

#include "model/StoryNode.h"

NodeInspectorWidget::NodeInspectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_titleEdit(new QLineEdit(this))
    , m_scriptEdit(new QTextEdit(this))
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto *headerLayout = new QHBoxLayout;
    m_headerLabel = new QLabel(this);
    m_headerLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    headerLayout->addWidget(m_headerLabel);
    headerLayout->addStretch();
    m_expandButton = new QToolButton(this);
    m_expandButton->setCheckable(true);
    m_expandButton->setAutoRaise(true);
    m_expandButton->setIconSize(QSize(16, 16));
    headerLayout->addWidget(m_expandButton);
    mainLayout->addLayout(headerLayout);

    auto *formLayout = new QFormLayout;
    m_titleLabel = new QLabel(this);
    formLayout->addRow(m_titleLabel, m_titleEdit);
    mainLayout->addLayout(formLayout);

    m_formatToolbar = new QToolBar(this);
    m_formatToolbar->setIconSize(QSize(16, 16));
    m_formatToolbar->setMovable(false);
    m_formatToolbar->setFloatable(false);

    m_boldAction = m_formatToolbar->addAction(QString());
    m_boldAction->setIcon(QIcon(QStringLiteral(":/icons/text_bold.svg")));
    m_boldAction->setShortcut(QKeySequence::Bold);
    m_boldAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QFont boldFont = font();
    boldFont.setBold(true);
    m_boldAction->setFont(boldFont);
    m_boldAction->setCheckable(true);

    m_italicAction = m_formatToolbar->addAction(QString());
    m_italicAction->setIcon(QIcon(QStringLiteral(":/icons/text_italic.svg")));
    m_italicAction->setShortcut(QKeySequence::Italic);
    m_italicAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QFont italicFont = font();
    italicFont.setItalic(true);
    m_italicAction->setFont(italicFont);
    m_italicAction->setCheckable(true);

    m_underlineAction = m_formatToolbar->addAction(QString());
    m_underlineAction->setIcon(QIcon(QStringLiteral(":/icons/text_underline.svg")));
    m_underlineAction->setShortcut(QKeySequence::Underline);
    m_underlineAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QFont underlineFont = font();
    underlineFont.setUnderline(true);
    m_underlineAction->setFont(underlineFont);
    m_underlineAction->setCheckable(true);

    m_formatToolbar->addSeparator();

    m_colorButton = new QToolButton(this);
    m_colorButton->setIcon(QIcon(QStringLiteral(":/icons/palette.svg")));
    m_colorButton->setAutoRaise(true);
    m_formatToolbar->addWidget(m_colorButton);

    m_fontSizeCombo = new QComboBox(this);
    m_fontSizeCombo->setEditable(true);
    m_fontSizeCombo->setInsertPolicy(QComboBox::NoInsert);
    m_fontSizeCombo->lineEdit()->setValidator(new QIntValidator(1, 200, m_fontSizeCombo));
    const QList<int> sizes = QFontDatabase::standardSizes();
    for (int size : sizes) {
        m_fontSizeCombo->addItem(QString::number(size));
    }
    m_fontSizeCombo->setCurrentText(QString::number(m_scriptEdit->fontPointSize() > 0 ? static_cast<int>(m_scriptEdit->fontPointSize()) : 12));
    m_formatToolbar->addWidget(m_fontSizeCombo);

    mainLayout->addWidget(m_formatToolbar);
    mainLayout->addWidget(m_scriptEdit, 1);

    connect(m_titleEdit, &QLineEdit::textEdited, this, &NodeInspectorWidget::onTitleEdited);
    connect(m_scriptEdit, &QTextEdit::textChanged, this, &NodeInspectorWidget::onScriptEdited);
    connect(m_scriptEdit, &QTextEdit::currentCharFormatChanged, this, &NodeInspectorWidget::onCurrentCharFormatChanged);
    connect(m_scriptEdit, &QTextEdit::cursorPositionChanged, this, [this]() {
        onCurrentCharFormatChanged(m_scriptEdit->currentCharFormat());
    });
    connect(m_expandButton, &QToolButton::toggled, this, &NodeInspectorWidget::onExpandToggled);
    connect(m_boldAction, &QAction::toggled, this, &NodeInspectorWidget::applyBold);
    connect(m_italicAction, &QAction::toggled, this, &NodeInspectorWidget::applyItalic);
    connect(m_underlineAction, &QAction::toggled, this, &NodeInspectorWidget::applyUnderline);
    connect(m_fontSizeCombo, &QComboBox::currentTextChanged, this, &NodeInspectorWidget::changeFontSize);
    connect(m_colorButton, &QToolButton::clicked, this, &NodeInspectorWidget::chooseTextColor);

    m_scriptEdit->setAcceptRichText(true);

    retranslateUi();
}

void NodeInspectorWidget::setNode(StoryNode *node)
{
    m_node = node;
    refresh();
}

void NodeInspectorWidget::setExpanded(bool expanded)
{
    if (m_isExpanded == expanded) {
        return;
    }

    m_isExpanded = expanded;
    const QSignalBlocker blocker(m_expandButton);
    m_expandButton->setChecked(expanded);
    updateExpandButtonAppearance();
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
    m_node->setScript(m_scriptEdit->toHtml());
    emit nodeUpdated(m_node->id());
}

void NodeInspectorWidget::onExpandToggled(bool expanded)
{
    if (m_isExpanded == expanded) {
        updateExpandButtonAppearance();
    } else {
        m_isExpanded = expanded;
        updateExpandButtonAppearance();
        emit expandRequested(expanded);
    }
}

void NodeInspectorWidget::applyBold(bool enabled)
{
    if (m_blockFormatSignals) {
        return;
    }
    QTextCharFormat format;
    format.setFontWeight(enabled ? QFont::Bold : QFont::Normal);
    mergeFormatOnSelection(format);
}

void NodeInspectorWidget::applyItalic(bool enabled)
{
    if (m_blockFormatSignals) {
        return;
    }
    QTextCharFormat format;
    format.setFontItalic(enabled);
    mergeFormatOnSelection(format);
}

void NodeInspectorWidget::applyUnderline(bool enabled)
{
    if (m_blockFormatSignals) {
        return;
    }
    QTextCharFormat format;
    format.setFontUnderline(enabled);
    mergeFormatOnSelection(format);
}

void NodeInspectorWidget::changeFontSize(const QString &sizeText)
{
    if (m_blockFormatSignals) {
        return;
    }
    bool ok = false;
    const qreal size = sizeText.toDouble(&ok);
    if (!ok || size <= 0.0) {
        return;
    }
    QTextCharFormat format;
    format.setFontPointSize(size);
    mergeFormatOnSelection(format);
}

void NodeInspectorWidget::chooseTextColor()
{
    const QColor color = QColorDialog::getColor(m_scriptEdit->currentCharFormat().foreground().color(), this, tr("Select Text Color"));
    if (!color.isValid()) {
        return;
    }
    QTextCharFormat format;
    format.setForeground(color);
    mergeFormatOnSelection(format);

    if (m_colorButton) {
        m_colorButton->setStyleSheet(QStringLiteral("background-color: %1").arg(color.name()));
    }
}

void NodeInspectorWidget::onCurrentCharFormatChanged(const QTextCharFormat &format)
{
    updateFormatControls(format);
}

void NodeInspectorWidget::refresh()
{
    const QSignalBlocker blocker1(m_titleEdit);
    const QSignalBlocker blocker2(m_scriptEdit);
    if (m_node) {
        m_titleEdit->setText(m_node->title());
        const QString script = m_node->script();
        if (Qt::mightBeRichText(script)) {
            m_scriptEdit->setHtml(script);
        } else {
            m_scriptEdit->setPlainText(script);
        }
    } else {
        m_titleEdit->clear();
        m_scriptEdit->clear();
    }
    updateFormatControls(m_scriptEdit->currentCharFormat());
}

void NodeInspectorWidget::updateExpandButtonAppearance()
{
    if (!m_expandButton) {
        return;
    }
    m_expandButton->setIcon(style()->standardIcon(m_isExpanded ? QStyle::SP_TitleBarNormalButton
                                                              : QStyle::SP_TitleBarMaxButton));
    m_expandButton->setText(QString());
    const QString tooltip = m_isExpanded ? tr("Restore inspector to sidebar")
                                         : tr("Expand inspector to full window");
    m_expandButton->setToolTip(tooltip);
    m_expandButton->setStatusTip(tooltip);
    m_expandButton->setAccessibleName(tooltip);
}

void NodeInspectorWidget::mergeFormatOnSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = m_scriptEdit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    m_scriptEdit->mergeCurrentCharFormat(format);
}

void NodeInspectorWidget::updateFormatControls(const QTextCharFormat &format)
{
    m_blockFormatSignals = true;

    if (m_boldAction) {
        m_boldAction->setChecked(format.fontWeight() == QFont::Bold);
    }
    if (m_italicAction) {
        m_italicAction->setChecked(format.fontItalic());
    }
    if (m_underlineAction) {
        m_underlineAction->setChecked(format.fontUnderline());
    }
    if (m_fontSizeCombo) {
        QSignalBlocker blocker(m_fontSizeCombo);
        const qreal size = format.fontPointSize();
        if (size > 0.0) {
            const QString sizeText = QString::number(static_cast<int>(std::round(size)));
            const int index = m_fontSizeCombo->findText(sizeText);
            if (index >= 0) {
                m_fontSizeCombo->setCurrentIndex(index);
            } else {
                m_fontSizeCombo->setEditText(sizeText);
            }
        }
    }
    if (m_colorButton) {
        const QColor color = format.foreground().color();
        if (color.isValid()) {
            m_colorButton->setStyleSheet(QStringLiteral("background-color: %1").arg(color.name()));
        } else {
            m_colorButton->setStyleSheet(QString());
        }
    }

    m_blockFormatSignals = false;
}

void NodeInspectorWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void NodeInspectorWidget::retranslateUi()
{
    if (m_headerLabel) {
        m_headerLabel->setText(tr("Node Inspector"));
    }
    if (m_titleLabel) {
        m_titleLabel->setText(tr("Title"));
    }
    if (m_boldAction) {
        m_boldAction->setText(tr("Bold"));
        const QString shortcutText = m_boldAction->shortcut().toString(QKeySequence::NativeText);
        const QString tip = shortcutText.isEmpty() ? tr("Toggle bold formatting")
                                                   : tr("Toggle bold formatting (%1)").arg(shortcutText);
        m_boldAction->setToolTip(tip);
        m_boldAction->setStatusTip(tip);
    }
    if (m_italicAction) {
        m_italicAction->setText(tr("Italic"));
        const QString shortcutText = m_italicAction->shortcut().toString(QKeySequence::NativeText);
        const QString tip = shortcutText.isEmpty() ? tr("Toggle italic formatting")
                                                   : tr("Toggle italic formatting (%1)").arg(shortcutText);
        m_italicAction->setToolTip(tip);
        m_italicAction->setStatusTip(tip);
    }
    if (m_underlineAction) {
        m_underlineAction->setText(tr("Underline"));
        const QString shortcutText = m_underlineAction->shortcut().toString(QKeySequence::NativeText);
        const QString tip = shortcutText.isEmpty() ? tr("Toggle underline formatting")
                                                   : tr("Toggle underline formatting (%1)").arg(shortcutText);
        m_underlineAction->setToolTip(tip);
        m_underlineAction->setStatusTip(tip);
    }
    if (m_colorButton) {
        m_colorButton->setText(tr("Color"));
        const QString tip = tr("Choose the text color");
        m_colorButton->setToolTip(tip);
        m_colorButton->setStatusTip(tip);
        m_colorButton->setAccessibleName(tr("Text color"));
    }
    updateExpandButtonAppearance();
}

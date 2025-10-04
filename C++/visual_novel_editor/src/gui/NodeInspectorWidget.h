#pragma once

#include <QString>
#include <QWidget>

class QAction;
class QComboBox;
class QToolBar;
class QToolButton;
class QLabel;

class QLineEdit;
class QTextEdit;
class StoryNode;
class QTextCharFormat;

class NodeInspectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NodeInspectorWidget(QWidget *parent = nullptr);

    void setNode(StoryNode *node);
    void setExpanded(bool expanded);

signals:
    void nodeUpdated(const QString &nodeId);
    void expandRequested(bool expanded);

private slots:
    void onTitleEdited(const QString &text);
    void onScriptEdited();
    void onExpandToggled(bool expanded);
    void applyBold(bool enabled);
    void applyItalic(bool enabled);
    void applyUnderline(bool enabled);
    void changeFontSize(const QString &sizeText);
    void chooseTextColor();
    void onCurrentCharFormatChanged(const QTextCharFormat &format);

protected:
    void changeEvent(QEvent *event) override;

private:
    void refresh();
    void updateExpandButtonAppearance();
    void mergeFormatOnSelection(const QTextCharFormat &format);
    void updateFormatControls(const QTextCharFormat &format);
    void retranslateUi();

    StoryNode *m_node{nullptr};
    QLineEdit *m_titleEdit{nullptr};
    QTextEdit *m_scriptEdit{nullptr};
    QToolButton *m_expandButton{nullptr};
    QToolBar *m_formatToolbar{nullptr};
    QAction *m_boldAction{nullptr};
    QAction *m_italicAction{nullptr};
    QAction *m_underlineAction{nullptr};
    QComboBox *m_fontSizeCombo{nullptr};
    QToolButton *m_colorButton{nullptr};
    QLabel *m_headerLabel{nullptr};
    QLabel *m_titleLabel{nullptr};
    bool m_isExpanded{false};
    bool m_blockFormatSignals{false};
};

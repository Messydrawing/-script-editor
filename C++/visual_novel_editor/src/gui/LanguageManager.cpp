#include "LanguageManager.h"

#include <QCoreApplication>
#include <QHash>
#include <QTranslator>

namespace {

QString makeKey(const char *context, const char *sourceText)
{
    return QString::fromLatin1(context) + QLatin1Char('\004') + QString::fromUtf8(sourceText);
}

} // namespace

class LanguageManager::InlineTranslator : public QTranslator
{
public:
    InlineTranslator()
    {
        const QHash<QString, QString> translations = {
            {makeKey("MainWindow", "&File"), QStringLiteral("文件(&F)")},
            {makeKey("MainWindow", "&New"), QStringLiteral("新建(&N)")},
            {makeKey("MainWindow", "&Open"), QStringLiteral("打开(&O)")},
            {makeKey("MainWindow", "&Save"), QStringLiteral("保存(&S)")},
            {makeKey("MainWindow", "E&xit"), QStringLiteral("退出(&X)")},
            {makeKey("MainWindow", "&Edit"), QStringLiteral("编辑(&E)")},
            {makeKey("MainWindow", "Add Node"), QStringLiteral("添加节点")},
            {makeKey("MainWindow", "Delete"), QStringLiteral("删除")},
            {makeKey("MainWindow", "Edit Script"), QStringLiteral("编辑脚本")},
            {makeKey("MainWindow", "&Export"), QStringLiteral("导出(&E)")},
            {makeKey("MainWindow", "Export to Ren'Py"), QStringLiteral("导出为 Ren'Py")},
            {makeKey("MainWindow", "Tools"), QStringLiteral("工具")},
            {makeKey("MainWindow", "Export"), QStringLiteral("导出")},
            {makeKey("MainWindow", "Inspector"), QStringLiteral("检查器")},
            {makeKey("MainWindow", "Ready"), QStringLiteral("就绪")},
            {makeKey("MainWindow", "Created new project"), QStringLiteral("已创建新项目")},
            {makeKey("MainWindow", "Open Project"), QStringLiteral("打开项目")},
            {makeKey("MainWindow", "Project (*.json)"), QStringLiteral("项目文件 (*.json)")},
            {makeKey("MainWindow", "Load Failed"), QStringLiteral("加载失败")},
            {makeKey("MainWindow", "Unable to open project file."), QStringLiteral("无法打开项目文件。")},
            {makeKey("MainWindow", "Project loaded"), QStringLiteral("项目已加载")},
            {makeKey("MainWindow", "Save Project"), QStringLiteral("保存项目")},
            {makeKey("MainWindow", "Save Failed"), QStringLiteral("保存失败")},
            {makeKey("MainWindow", "Unable to write project file."), QStringLiteral("无法写入项目文件。")},
            {makeKey("MainWindow", "Project saved"), QStringLiteral("项目已保存")},
            {makeKey("MainWindow", "Dialogue"), QStringLiteral("对话")},
            {makeKey("MainWindow", "# dialogue script"), QStringLiteral("# 对话脚本")},
            {makeKey("MainWindow", "Node added"), QStringLiteral("节点已添加")},
            {makeKey("MainWindow", "Export Ren'Py Script"), QStringLiteral("导出 Ren'Py 脚本")},
            {makeKey("MainWindow", "Ren'Py Script (*.rpy)"), QStringLiteral("Ren'Py 脚本 (*.rpy)")},
            {makeKey("MainWindow", "Export Failed"), QStringLiteral("导出失败")},
            {makeKey("MainWindow", "Could not export Ren'Py script."), QStringLiteral("无法导出 Ren'Py 脚本。")},
            {makeKey("MainWindow", "Exported to Ren'Py"), QStringLiteral("已导出至 Ren'Py")},
            {makeKey("MainWindow", "Settings"), QStringLiteral("设置")},
            {makeKey("MainWindow", "Language"), QStringLiteral("语言")},
            {makeKey("MainWindow", "English"), QStringLiteral("英语")},
            {makeKey("MainWindow", "Chinese"), QStringLiteral("中文")},
            {makeKey("MainWindow", "OK"), QStringLiteral("确定")},
            {makeKey("MainWindow", "Cancel"), QStringLiteral("取消")},
            {makeKey("GraphScene", "Copy"), QStringLiteral("复制")},
            {makeKey("GraphScene", "Cut"), QStringLiteral("剪切")},
            {makeKey("GraphScene", "Delete"), QStringLiteral("删除")},
            {makeKey("GraphScene", "Create Branch"), QStringLiteral("创建分支")},
            {makeKey("GraphScene", "Add Node"), QStringLiteral("添加节点")},
            {makeKey("NodeInspectorWidget", "Node Inspector"), QStringLiteral("节点检查器")},
            {makeKey("NodeInspectorWidget", "Expand inspector to full window"), QStringLiteral("将检查器扩展为全窗口")},
            {makeKey("NodeInspectorWidget", "Restore inspector to sidebar"), QStringLiteral("将检查器还原到侧栏")},
            {makeKey("NodeInspectorWidget", "Title"), QStringLiteral("标题")},
            {makeKey("NodeInspectorWidget", "B"), QStringLiteral("B")},
            {makeKey("NodeInspectorWidget", "I"), QStringLiteral("I")},
            {makeKey("NodeInspectorWidget", "U"), QStringLiteral("U")},
            {makeKey("NodeInspectorWidget", "Color"), QStringLiteral("颜色")},
            {makeKey("NodeInspectorWidget", "Select Text Color"), QStringLiteral("选择文本颜色")},
            {makeKey("ScriptEditorDialog", "Script Editor"), QStringLiteral("脚本编辑器")},
            {makeKey("ScriptEditorDialog", "OK"), QStringLiteral("确定")},
            {makeKey("ScriptEditorDialog", "Cancel"), QStringLiteral("取消")},
        };

        m_translations = translations;
    }

    QString translate(const char *context, const char *sourceText,
                       const char *disambiguation, int n) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);
        if (!context || !sourceText) {
            return {};
        }
        const QString key = makeKey(context, sourceText);
        const auto it = m_translations.constFind(key);
        if (it != m_translations.cend()) {
            return *it;
        }
        return {};
    }

private:
    QHash<QString, QString> m_translations;
};

LanguageManager &LanguageManager::instance()
{
    static LanguageManager manager;
    return manager;
}

LanguageManager::LanguageManager()
    : QObject(nullptr)
    , m_chineseTranslator(std::make_unique<InlineTranslator>())
{
}

void LanguageManager::initialize(QCoreApplication *app)
{
    m_app = app;
    if (m_app && m_language == Language::Chinese) {
        m_app->installTranslator(m_chineseTranslator.get());
    }
}

void LanguageManager::setLanguage(Language language)
{
    if (language == m_language) {
        return;
    }
    m_language = language;

    if (!m_app) {
        emit languageChanged(m_language);
        return;
    }

    if (m_language == Language::Chinese) {
        if (!m_app->installTranslator(m_chineseTranslator.get())) {
            emit languageChanged(m_language);
            return;
        }
    } else {
        m_app->removeTranslator(m_chineseTranslator.get());
    }

    emit languageChanged(m_language);
}

LanguageManager::Language LanguageManager::language() const
{
    return m_language;
}


#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDockWidget>
#include <QEvent>
#include <QEventLoop>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QStatusBar>
#include <QToolBar>
#include <QKeySequence>

#include <memory>

#include "GraphScene.h"
#include "NodeInspectorWidget.h"
#include "NodeItem.h"
#include "ScriptEditorDialog.h"
#include "model/Project.h"
#include "model/StoryNode.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createMenus();
    createToolbars();
    setupScene();
    if (m_scene && m_inspector) {
        m_presenter = std::make_unique<gui::presenter::ProjectPresenter>(*this, *m_scene, *m_inspector);
        if (m_project) {
            m_presenter->setProject(m_project);
        }
    }
    updateLanguageMenuState();
    retranslateUi();
    setStatusMessage(QStringLiteral("Ready"));

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, [this](LanguageManager::Language) {
        updateLanguageMenuState();
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::setProject(Project *project)
{
    m_project = project;
    if (m_presenter) {
        m_presenter->setProject(m_project);
    } else if (m_scene) {
        m_scene->setProject(m_project);
    }
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(QString());
    m_newAction = m_fileMenu->addAction(QString(), this, &MainWindow::newProject);
    m_newAction->setShortcut(QKeySequence::New);
    m_openAction = m_fileMenu->addAction(QString(), this, &MainWindow::openProject);
    m_openAction->setShortcut(QKeySequence::Open);
    m_saveAction = m_fileMenu->addAction(QString(), this, &MainWindow::saveProject);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_fileMenu->addSeparator();
    m_exitAction = m_fileMenu->addAction(QString(), this, &QWidget::close);

    m_editMenu = menuBar()->addMenu(QString());
    m_addNodeAction = m_editMenu->addAction(QString(), this, &MainWindow::addNode);
    m_deleteAction = m_editMenu->addAction(QString(), this, &MainWindow::deleteSelection);
    m_editScriptAction = m_editMenu->addAction(QString(), this, &MainWindow::editScript);

    m_exportMenu = menuBar()->addMenu(QString());
    m_exportRenpyAction = m_exportMenu->addAction(QString(), this, &MainWindow::exportToRenpy);

    m_settingsMenu = menuBar()->addMenu(QString());
    m_languageMenu = m_settingsMenu->addMenu(QString());

    m_languageGroup = new QActionGroup(this);
    m_languageGroup->setExclusive(true);

    m_languageEnglishAction = m_languageMenu->addAction(QString());
    m_languageEnglishAction->setCheckable(true);
    m_languageChineseAction = m_languageMenu->addAction(QString());
    m_languageChineseAction->setCheckable(true);

    m_languageGroup->addAction(m_languageEnglishAction);
    m_languageGroup->addAction(m_languageChineseAction);

    connect(m_languageEnglishAction, &QAction::triggered, this, []() {
        LanguageManager::instance().setLanguage(LanguageManager::Language::English);
    });
    connect(m_languageChineseAction, &QAction::triggered, this, []() {
        LanguageManager::instance().setLanguage(LanguageManager::Language::Chinese);
    });
}

void MainWindow::createToolbars()
{
    m_mainToolbar = addToolBar(QString());
    if (m_mainToolbar) {
        m_mainToolbar->addAction(m_addNodeAction);
        m_mainToolbar->addAction(m_editScriptAction);
        m_mainToolbar->addAction(m_exportRenpyAction);
    }
}

void MainWindow::setupScene()
{
    m_scene = new GraphScene(this);
    connect(m_scene, &GraphScene::nodeSelected, this, &MainWindow::onNodeSelected);
    connect(m_scene, &GraphScene::nodeDoubleClicked, this, &MainWindow::onNodeDoubleClicked);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setRubberBandSelectionMode(Qt::IntersectsItemShape);
    setCentralWidget(m_view);

    m_inspectorDock = new QDockWidget(tr("Inspector"), this);
    m_inspector = new NodeInspectorWidget(m_inspectorDock);
    connect(m_inspector, &NodeInspectorWidget::nodeUpdated, this, [this](const QString &id) {
        if (m_scene && !id.isEmpty()) {
            m_scene->refreshNode(id);
        }
    });
    connect(m_inspector, &NodeInspectorWidget::expandRequested, this, &MainWindow::toggleInspectorExpanded);
    m_inspectorDock->setWidget(m_inspector);
    addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);
}

void MainWindow::newProject()
{
    if (m_presenter) {
        m_presenter->newProject();
    }
}

void MainWindow::openProject()
{
    if (!m_project) {
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), QString(), tr("Project (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    if (!m_project->loadFromFile(fileName)) {
        QMessageBox::warning(this, tr("Load Failed"), tr("Unable to open project file."));
        return;
    }
    m_currentProjectFile = fileName;
    m_scene->setProject(m_project);
    setStatusMessage(QStringLiteral("Project loaded"), 2000);
}

void MainWindow::saveProject()
{
    if (!m_project) {
        return;
    }

    QString fileName = m_currentProjectFile;
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save Project"), QString(), tr("Project (*.json)"));
    }
    if (fileName.isEmpty()) {
        return;
    }

    if (!m_project->saveToFile(fileName)) {
        QMessageBox::warning(this, tr("Save Failed"), tr("Unable to write project file."));
        return;
    }
    m_currentProjectFile = fileName;
    setStatusMessage(QStringLiteral("Project saved"), 2000);
}

void MainWindow::addNode()
{
    if (m_presenter) {
        m_presenter->addNode();
    }
}

void MainWindow::deleteSelection()
{
    if (m_presenter) {
        m_presenter->deleteSelection();
    }
}

void MainWindow::editScript()
{
    if (!m_scene || !m_project) {
        return;
    }

    const QList<QGraphicsItem *> selection = m_scene->selectedItems();
    if (selection.isEmpty()) {
        return;
    }

    const auto *nodeItem = qgraphicsitem_cast<NodeItem *>(selection.first());
    if (!nodeItem) {
        return;
    }

    openScriptEditorForNode(nodeItem->storyNode());
}

void MainWindow::exportToRenpy()
{
    if (m_presenter) {
        m_presenter->exportToRenpy();
    }
}

void MainWindow::onNodeSelected(const QString &nodeId)
{
    if (!m_project) {
        return;
    }

    if (m_inspector) {
        if (StoryNode *node = m_project->getNode(nodeId)) {
            m_inspector->setNode(node);
        }
    }
}

void MainWindow::toggleInspectorExpanded(bool expanded)
{
    if (!m_inspector || !m_inspectorDock) {
        return;
    }

    if (m_isInspectorExpanded == expanded) {
        m_inspector->setExpanded(expanded);
        return;
    }

    if (expanded) {
        QWidget *currentCentral = takeCentralWidget();
        if (currentCentral) {
            m_previousCentralWidget = currentCentral;
            m_previousCentralWidget->setParent(this);
            m_previousCentralWidget->hide();
        }

        m_inspectorDock->setWidget(nullptr);
        m_inspectorDock->hide();
        m_inspector->setParent(this);
        setCentralWidget(m_inspector);
    } else {
        QWidget *currentCentral = takeCentralWidget();
        if (currentCentral && currentCentral != m_inspector) {
            currentCentral->setParent(this);
        }

        QWidget *widgetToRestore = m_previousCentralWidget ? m_previousCentralWidget : m_view;
        if (widgetToRestore) {
            widgetToRestore->setParent(this);
            widgetToRestore->show();
            setCentralWidget(widgetToRestore);
        }

        m_inspectorDock->setWidget(m_inspector);
        m_inspectorDock->show();
        m_previousCentralWidget = nullptr;
    }

    m_isInspectorExpanded = expanded;
    m_inspector->setExpanded(expanded);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        if (!m_lastStatusKey.isEmpty() && statusBar() && !statusBar()->currentMessage().isEmpty()) {
            const QByteArray utf8 = m_lastStatusKey.toUtf8();
            statusBar()->showMessage(tr(utf8.constData()), m_lastStatusTimeout);
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi()
{
    if (m_fileMenu) {
        m_fileMenu->setTitle(tr("&File"));
    }
    if (m_newAction) {
        m_newAction->setText(tr("&New"));
    }
    if (m_openAction) {
        m_openAction->setText(tr("&Open"));
    }
    if (m_saveAction) {
        m_saveAction->setText(tr("&Save"));
    }
    if (m_exitAction) {
        m_exitAction->setText(tr("E&xit"));
    }

    if (m_editMenu) {
        m_editMenu->setTitle(tr("&Edit"));
    }
    if (m_addNodeAction) {
        m_addNodeAction->setText(tr("Add Node"));
    }
    if (m_deleteAction) {
        m_deleteAction->setText(tr("Delete"));
    }
    if (m_editScriptAction) {
        m_editScriptAction->setText(tr("Edit Script"));
    }

    if (m_exportMenu) {
        m_exportMenu->setTitle(tr("&Export"));
    }
    if (m_exportRenpyAction) {
        m_exportRenpyAction->setText(tr("Export to Ren'Py"));
    }

    if (m_settingsMenu) {
        m_settingsMenu->setTitle(tr("Settings"));
    }
    if (m_languageMenu) {
        m_languageMenu->setTitle(tr("Language"));
    }
    if (m_languageEnglishAction) {
        m_languageEnglishAction->setText(tr("English"));
    }
    if (m_languageChineseAction) {
        m_languageChineseAction->setText(tr("Chinese"));
    }

    if (m_mainToolbar) {
        m_mainToolbar->setWindowTitle(tr("Tools"));
    }
    if (m_inspectorDock) {
        m_inspectorDock->setWindowTitle(tr("Inspector"));
    }
}

void MainWindow::updateLanguageMenuState()
{
    const auto currentLanguage = LanguageManager::instance().language();
    if (m_languageEnglishAction) {
        m_languageEnglishAction->setChecked(currentLanguage == LanguageManager::Language::English);
    }
    if (m_languageChineseAction) {
        m_languageChineseAction->setChecked(currentLanguage == LanguageManager::Language::Chinese);
    }
}

void MainWindow::openScriptEditorForNode(StoryNode *node)
{
    if (!node) {
        return;
    }

    ScriptEditorDialog dialog(node, this);
    const int result = dialog.exec();
    Q_UNUSED(result);

    if (m_scene) {
        m_scene->refreshNode(node->id());
    }
    if (m_inspector && m_project) {
        if (StoryNode *updated = m_project->getNode(node->id())) {
            m_inspector->setNode(updated);
        }
    }
}

void MainWindow::onNodeDoubleClicked(const QString &nodeId)
{
    if (!m_project) {
        return;
    }
    if (StoryNode *node = m_project->getNode(nodeId)) {
        openScriptEditorForNode(node);
    }
}

void MainWindow::setStatusMessage(const QString &key, int timeoutMs)
{
    m_lastStatusKey = key;
    m_lastStatusTimeout = timeoutMs;
    if (!statusBar() || key.isEmpty()) {
        return;
    }
    const QByteArray utf8 = key.toUtf8();
    statusBar()->showMessage(tr(utf8.constData()), timeoutMs);
}

QString MainWindow::promptSaveFile(const QString &titleKey, const QString &filterKey)
{
    const QByteArray titleUtf8 = titleKey.toUtf8();
    const QByteArray filterUtf8 = filterKey.toUtf8();
    return QFileDialog::getSaveFileName(this, tr(titleUtf8.constData()), QString(), tr(filterUtf8.constData()));
}

void MainWindow::showWarningMessage(const QString &titleKey, const QString &messageKey)
{
    const QByteArray titleUtf8 = titleKey.toUtf8();
    const QByteArray messageUtf8 = messageKey.toUtf8();
    QMessageBox::warning(this, tr(titleUtf8.constData()), tr(messageUtf8.constData()));
}

void MainWindow::displayStatusMessage(const QString &key, int timeoutMs)
{
    setStatusMessage(key, timeoutMs);
}

void MainWindow::resetProjectFilePath()
{
    m_currentProjectFile.clear();
}

namespace {
class ProgressDialogHandle : public gui::presenter::IExportProgressView
{
public:
    explicit ProgressDialogHandle(QProgressDialog *dialog)
        : m_dialog(dialog)
    {
    }

    bool update(int current, int total) override
    {
        if (!m_dialog) {
            return false;
        }
        if (total > 0) {
            m_dialog->setMaximum(total);
        }
        m_dialog->setValue(current);
        return !m_dialog->wasCanceled();
    }

    void close() override
    {
        if (!m_dialog) {
            return;
        }
        m_dialog->close();
        m_dialog->deleteLater();
        m_dialog = nullptr;
    }

private:
    QPointer<QProgressDialog> m_dialog;
};
} // namespace

std::unique_ptr<gui::presenter::IExportProgressView> MainWindow::createExportProgressDialog(const QString &titleKey,
                                                                                            const QString &labelKey,
                                                                                            const QString &cancelKey)
{
    const QByteArray titleUtf8 = titleKey.toUtf8();
    const QByteArray labelUtf8 = labelKey.toUtf8();
    const QByteArray cancelUtf8 = cancelKey.toUtf8();

    auto dialog = new QProgressDialog(tr(labelUtf8.constData()), tr(cancelUtf8.constData()), 0, 1, this);
    dialog->setWindowTitle(tr(titleUtf8.constData()));
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setMinimumDuration(0);
    dialog->setAutoClose(false);
    dialog->setAutoReset(false);
    dialog->show();

    return std::make_unique<ProgressDialogHandle>(dialog);
}

void MainWindow::processEvents()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

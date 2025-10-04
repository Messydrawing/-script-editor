#include "MainWindow.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QKeySequence>

#include "GraphScene.h"
#include "NodeInspectorWidget.h"
#include "NodeItem.h"
#include "ScriptEditorDialog.h"
#include "export/ExporterRenpy.h"
#include "model/Project.h"
#include "model/StoryNode.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createMenus();
    createToolbars();
    setupScene();
    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;

void MainWindow::setProject(Project *project)
{
    m_project = project;
    if (m_scene) {
        m_scene->setProject(m_project);
    }
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newAction = fileMenu->addAction(tr("&New"), this, &MainWindow::newProject);
    newAction->setShortcut(QKeySequence::New);
    QAction *openAction = fileMenu->addAction(tr("&Open"), this, &MainWindow::openProject);
    openAction->setShortcut(QKeySequence::Open);
    QAction *saveAction = fileMenu->addAction(tr("&Save"), this, &MainWindow::saveProject);
    saveAction->setShortcut(QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("Add Node"), this, &MainWindow::addNode);
    editMenu->addAction(tr("Delete"), this, &MainWindow::deleteSelection);
    editMenu->addAction(tr("Edit Script"), this, &MainWindow::editScript);

    QMenu *exportMenu = menuBar()->addMenu(tr("&Export"));
    exportMenu->addAction(tr("Export to Ren'Py"), this, &MainWindow::exportToRenpy);
}

void MainWindow::createToolbars()
{
    QToolBar *toolbar = addToolBar(tr("Tools"));
    toolbar->addAction(tr("Add Node"), this, &MainWindow::addNode);
    toolbar->addAction(tr("Edit Script"), this, &MainWindow::editScript);
    toolbar->addAction(tr("Export"), this, &MainWindow::exportToRenpy);
}

void MainWindow::setupScene()
{
    m_scene = new GraphScene(this);
    connect(m_scene, &GraphScene::nodeSelected, this, &MainWindow::onNodeSelected);

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
    if (!m_project) {
        return;
    }
    m_project->clear();
    m_currentProjectFile.clear();
    m_scene->setProject(m_project);
    statusBar()->showMessage(tr("Created new project"), 2000);
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
    statusBar()->showMessage(tr("Project loaded"), 2000);
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
    statusBar()->showMessage(tr("Project saved"), 2000);
}

void MainWindow::addNode()
{
    if (!m_project || !m_scene) {
        return;
    }

    StoryNode *node = m_project->addNode(StoryNode::Type::Dialogue);
    node->setTitle(tr("Dialogue"));
    node->setScript(tr("# dialogue script"));
    m_scene->setProject(m_project);
    statusBar()->showMessage(tr("Node added"), 1500);
}

void MainWindow::deleteSelection()
{
    if (!m_project || !m_scene) {
        return;
    }

    const QList<QGraphicsItem *> selection = m_scene->selectedItems();
    for (QGraphicsItem *item : selection) {
        const auto *nodeItem = qgraphicsitem_cast<NodeItem *>(item);
        if (nodeItem) {
            m_project->removeNode(nodeItem->storyNode()->id());
        }
    }
    m_scene->setProject(m_project);
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

    ScriptEditorDialog dialog(nodeItem->storyNode(), this);
    dialog.exec();
}

void MainWindow::exportToRenpy()
{
    if (!m_project) {
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Ren'Py Script"), QString(), tr("Ren'Py Script (*.rpy)"));
    if (fileName.isEmpty()) {
        return;
    }

    ExporterRenpy exporter(m_project);
    if (!exporter.exportToFile(fileName)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not export Ren'Py script."));
    } else {
        statusBar()->showMessage(tr("Exported to Ren'Py"), 2000);
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

#include "ProjectPresenter.h"

#include <QObject>
#include <QStringList>

#include "export/ExporterRenpy.h"
#include "model/Project.h"
#include "model/StoryNode.h"

namespace gui::presenter {

ProjectPresenter::ProjectPresenter(IMainWindowView &mainWindowView,
                                   IGraphSceneView &graphSceneView,
                                   INodeInspectorView &inspectorView)
    : m_mainWindowView(mainWindowView)
    , m_graphSceneView(graphSceneView)
    , m_inspectorView(inspectorView)
{
}

void ProjectPresenter::setProject(Project *project)
{
    m_project = project;
    m_graphSceneView.setProject(m_project);
}

void ProjectPresenter::newProject()
{
    if (!m_project) {
        return;
    }

    m_project->clear();
    m_graphSceneView.setProject(m_project);
    m_mainWindowView.resetProjectFilePath();
    m_mainWindowView.displayStatusMessage(QStringLiteral("Created new project"), 2000);
}

void ProjectPresenter::addNode()
{
    if (!m_project) {
        return;
    }

    StoryNode *node = m_project->addNode(StoryNode::Type::Dialogue);
    if (node) {
        node->setTitle(QObject::tr("Dialogue"));
        node->setScript(QObject::tr("# dialogue script"));
    }
    m_graphSceneView.setProject(m_project);
    m_mainWindowView.displayStatusMessage(QStringLiteral("Node added"), 1500);
}

void ProjectPresenter::deleteSelection()
{
    if (!m_project) {
        return;
    }

    const QStringList selectedIds = m_graphSceneView.selectedNodeIds();
    if (selectedIds.isEmpty()) {
        return;
    }

    for (const QString &id : selectedIds) {
        m_project->removeNode(id);
    }

    m_graphSceneView.setProject(m_project);
}

void ProjectPresenter::exportToRenpy()
{
    if (!m_project) {
        return;
    }

    const QString fileName = m_mainWindowView.promptSaveFile(
        QStringLiteral("Export Ren'Py Script"),
        QStringLiteral("Ren'Py Script (*.rpy)"));
    if (fileName.isEmpty()) {
        return;
    }

    auto progressDialog = m_mainWindowView.createExportProgressDialog(
        QStringLiteral("Exporting"),
        QStringLiteral("Exporting Ren'Py script..."),
        QStringLiteral("Cancel"));

    ExporterRenpy exporter(m_project);
    const QStringList selectedNodeIds = m_graphSceneView.selectedNodeIds();
    if (!selectedNodeIds.isEmpty()) {
        exporter.setSelectedNodeIds(selectedNodeIds);
    }

    exporter.setProgressCallback([&](int current, int total) {
        if (progressDialog) {
            if (!progressDialog->update(current, total)) {
                return false;
            }
        }
        m_mainWindowView.processEvents();
        return true;
    });

    const bool exportResult = exporter.exportToFile(fileName);

    if (progressDialog) {
        progressDialog->close();
    }

    if (exporter.wasCanceled()) {
        m_mainWindowView.displayStatusMessage(QStringLiteral("Export canceled"), 2000);
        return;
    }

    if (!exportResult) {
        m_mainWindowView.showWarningMessage(QStringLiteral("Export Failed"),
                                            QStringLiteral("Could not export Ren'Py script."));
        return;
    }

    m_mainWindowView.displayStatusMessage(QStringLiteral("Exported to Ren'Py"), 2000);
}

} // namespace gui::presenter


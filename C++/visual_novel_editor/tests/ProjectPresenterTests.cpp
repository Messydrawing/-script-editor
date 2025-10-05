#include <cassert>
#include <memory>
#include <vector>

#include <QStringList>

#include "gui/presenter/ProjectPresenter.h"
#include "model/Project.h"
#include "model/StoryNode.h"

using gui::presenter::IExportProgressView;
using gui::presenter::IGraphSceneView;
using gui::presenter::IMainWindowView;
using gui::presenter::INodeInspectorView;
using gui::presenter::ProjectPresenter;

namespace {

class DummyProgressView : public IExportProgressView
{
public:
    bool update(int, int) override { return continueUpdates; }
    void close() override { closed = true; }

    bool continueUpdates{true};
    bool closed{false};
};

class StubMainWindowView : public IMainWindowView
{
public:
    QString promptSaveFile(const QString &, const QString &) override { return saveFileResponse; }

    void showWarningMessage(const QString &titleKey, const QString &messageKey) override
    {
        warnings.emplace_back(titleKey, messageKey);
    }

    void displayStatusMessage(const QString &key, int timeoutMs) override
    {
        statusMessages.emplace_back(key, timeoutMs);
    }

    void resetProjectFilePath() override { projectFileReset = true; }

    std::unique_ptr<IExportProgressView> createExportProgressDialog(const QString &,
                                                                    const QString &,
                                                                    const QString &) override
    {
        createdProgressDialog = true;
        return std::make_unique<DummyProgressView>();
    }

    void processEvents() override { processedEvents = true; }

    QString saveFileResponse;
    bool projectFileReset{false};
    bool createdProgressDialog{false};
    bool processedEvents{false};
    std::vector<std::pair<QString, int>> statusMessages;
    std::vector<std::pair<QString, QString>> warnings;
};

class StubGraphSceneView : public IGraphSceneView
{
public:
    void setProject(Project *project) override { lastProject = project; ++setProjectCalls; }

    QStringList selectedNodeIds() const override { return selectedIds; }

    Project *lastProject{nullptr};
    mutable QStringList selectedIds;
    int setProjectCalls{0};
};

class StubInspectorView : public INodeInspectorView
{
public:
    void setNode(StoryNode *node) override { lastNode = node; }
    void setExpanded(bool expanded) override { lastExpanded = expanded; }

    StoryNode *lastNode{nullptr};
    bool lastExpanded{false};
};

} // namespace

void testNewProjectResetsState()
{
    StubMainWindowView mainWindow;
    StubGraphSceneView scene;
    StubInspectorView inspector;
    ProjectPresenter presenter(mainWindow, scene, inspector);

    Project project;
    presenter.setProject(&project);

    project.addNode(StoryNode::Type::Dialogue);
    assert(!project.nodes().isEmpty());

    presenter.newProject();

    assert(project.nodes().isEmpty());
    assert(mainWindow.projectFileReset);
    assert(!mainWindow.statusMessages.empty());
    assert(mainWindow.statusMessages.back().first == QStringLiteral("Created new project"));
    assert(scene.lastProject == &project);
}

void testAddNodeCreatesStoryNode()
{
    StubMainWindowView mainWindow;
    StubGraphSceneView scene;
    StubInspectorView inspector;
    ProjectPresenter presenter(mainWindow, scene, inspector);

    Project project;
    presenter.setProject(&project);

    assert(project.nodes().isEmpty());

    presenter.addNode();

    assert(!project.nodes().isEmpty());
    assert(scene.lastProject == &project);
    assert(scene.setProjectCalls >= 2); // initial set + refresh after add
    assert(!mainWindow.statusMessages.empty());
    assert(mainWindow.statusMessages.back().first == QStringLiteral("Node added"));
}

void testDeleteSelectionRemovesNodes()
{
    StubMainWindowView mainWindow;
    StubGraphSceneView scene;
    StubInspectorView inspector;
    ProjectPresenter presenter(mainWindow, scene, inspector);

    Project project;
    presenter.setProject(&project);

    StoryNode *node = project.addNode(StoryNode::Type::Dialogue);
    const QString nodeId = node->id();

    scene.selectedIds = QStringList{nodeId};

    presenter.deleteSelection();

    assert(project.getNode(nodeId) == nullptr);
    assert(scene.setProjectCalls >= 2);
}

int main()
{
    testNewProjectResetsState();
    testAddNodeCreatesStoryNode();
    testDeleteSelectionRemovesNodes();

    return 0;
}


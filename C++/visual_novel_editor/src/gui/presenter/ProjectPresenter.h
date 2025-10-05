#pragma once

#include "ViewInterfaces.h"

class Project;

namespace gui::presenter {

class ProjectPresenter
{
public:
    ProjectPresenter(IMainWindowView &mainWindowView,
                     IGraphSceneView &graphSceneView,
                     INodeInspectorView &inspectorView);

    void setProject(Project *project);

    void newProject();
    void addNode();
    void deleteSelection();
    void exportToRenpy();

private:
    Project *m_project{nullptr};
    IMainWindowView &m_mainWindowView;
    IGraphSceneView &m_graphSceneView;
    INodeInspectorView &m_inspectorView;
};

} // namespace gui::presenter


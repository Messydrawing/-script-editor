#pragma once

#include <QMainWindow>
#include <QString>

class GraphScene;
class NodeInspectorWidget;
class Project;
class QGraphicsView;
class QDockWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setProject(Project *project);

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void addNode();
    void deleteSelection();
    void editScript();
    void exportToRenpy();
    void onNodeSelected(const QString &nodeId);
    void toggleInspectorExpanded(bool expanded);

private:
    void createMenus();
    void createToolbars();
    void setupScene();

    GraphScene *m_scene{nullptr};
    QGraphicsView *m_view{nullptr};
    NodeInspectorWidget *m_inspector{nullptr};
    QDockWidget *m_inspectorDock{nullptr};
    QWidget *m_previousCentralWidget{nullptr};
    bool m_isInspectorExpanded{false};
    Project *m_project{nullptr};
    QString m_currentProjectFile;
};

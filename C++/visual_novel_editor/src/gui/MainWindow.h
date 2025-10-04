#pragma once

#include <QMainWindow>

class GraphScene;
class NodeInspectorWidget;
class Project;
class QGraphicsView;

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

private:
    void createMenus();
    void createToolbars();
    void setupScene();

    GraphScene *m_scene{nullptr};
    QGraphicsView *m_view{nullptr};
    NodeInspectorWidget *m_inspector{nullptr};
    Project *m_project{nullptr};
    QString m_currentProjectFile;
};

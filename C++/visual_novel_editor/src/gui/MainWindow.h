#pragma once

#include <QMainWindow>
#include <QString>

#include <memory>

#include "presenter/ProjectPresenter.h"

#include "LanguageManager.h"

class GraphScene;
class NodeInspectorWidget;
class Project;
class QGraphicsView;
class QDockWidget;
class StoryNode;
class QMenu;
class QToolBar;
class QAction;
class QActionGroup;

class MainWindow : public QMainWindow, public gui::presenter::IMainWindowView
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setProject(Project *project);

protected:
    void changeEvent(QEvent *event) override;

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
    void onNodeDoubleClicked(const QString &nodeId);

private:
    void createMenus();
    void createToolbars();
    void setupScene();
    void retranslateUi();
    void updateLanguageMenuState();
    void openScriptEditorForNode(StoryNode *node);
    void setStatusMessage(const QString &key, int timeoutMs = 0);

    // gui::presenter::IMainWindowView overrides
    QString promptSaveFile(const QString &titleKey, const QString &filterKey) override;
    void showWarningMessage(const QString &titleKey, const QString &messageKey) override;
    void displayStatusMessage(const QString &key, int timeoutMs) override;
    void resetProjectFilePath() override;
    std::unique_ptr<gui::presenter::IExportProgressView> createExportProgressDialog(const QString &titleKey,
                                                                                    const QString &labelKey,
                                                                                    const QString &cancelKey) override;
    void processEvents() override;

    GraphScene *m_scene{nullptr};
    QGraphicsView *m_view{nullptr};
    NodeInspectorWidget *m_inspector{nullptr};
    QDockWidget *m_inspectorDock{nullptr};
    QWidget *m_previousCentralWidget{nullptr};
    Project *m_project{nullptr};
    QString m_currentProjectFile;
    bool m_isInspectorExpanded{false};

    QMenu *m_fileMenu{nullptr};
    QMenu *m_editMenu{nullptr};
    QMenu *m_exportMenu{nullptr};
    QMenu *m_settingsMenu{nullptr};
    QMenu *m_languageMenu{nullptr};
    QToolBar *m_mainToolbar{nullptr};

    QAction *m_newAction{nullptr};
    QAction *m_openAction{nullptr};
    QAction *m_saveAction{nullptr};
    QAction *m_exitAction{nullptr};
    QAction *m_addNodeAction{nullptr};
    QAction *m_deleteAction{nullptr};
    QAction *m_editScriptAction{nullptr};
    QAction *m_exportRenpyAction{nullptr};

    QActionGroup *m_languageGroup{nullptr};
    QAction *m_languageEnglishAction{nullptr};
    QAction *m_languageChineseAction{nullptr};

    QString m_lastStatusKey;
    int m_lastStatusTimeout{0};

    std::unique_ptr<gui::presenter::ProjectPresenter> m_presenter;
};

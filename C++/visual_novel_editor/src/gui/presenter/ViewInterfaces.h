#pragma once

#include <memory>

#include <QString>
#include <QStringList>

class Project;
class StoryNode;

namespace gui::presenter {

class IExportProgressView
{
public:
    virtual ~IExportProgressView() = default;
    virtual bool update(int current, int total) = 0;
    virtual void close() = 0;
};

class IMainWindowView
{
public:
    virtual ~IMainWindowView() = default;
    virtual QString promptSaveFile(const QString &titleKey, const QString &filterKey) = 0;
    virtual void showWarningMessage(const QString &titleKey, const QString &messageKey) = 0;
    virtual void displayStatusMessage(const QString &key, int timeoutMs) = 0;
    virtual void resetProjectFilePath() = 0;
    virtual std::unique_ptr<IExportProgressView> createExportProgressDialog(const QString &titleKey,
                                                                            const QString &labelKey,
                                                                            const QString &cancelKey) = 0;
    virtual void processEvents() = 0;
};

class IGraphSceneView
{
public:
    virtual ~IGraphSceneView() = default;
    virtual void setProject(Project *project) = 0;
    virtual QStringList selectedNodeIds() const = 0;
};

class INodeInspectorView
{
public:
    virtual ~INodeInspectorView() = default;
    virtual void setNode(StoryNode *node) = 0;
    virtual void setExpanded(bool expanded) = 0;
};

} // namespace gui::presenter


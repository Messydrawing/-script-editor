#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <functional>

class Project;

class ExporterRenpy
{
public:
    explicit ExporterRenpy(Project *project);

    [[nodiscard]] bool exportToFile(const QString &fileName);
    void setProgressCallback(std::function<bool(int, int)> callback);
    [[nodiscard]] bool wasCanceled() const { return m_wasCanceled; }
    void setSelectedNodeIds(const QStringList &nodeIds);

private:
    bool generateNode(const QString &nodeId, QTextStream &out, int indent = 0);
    [[nodiscard]] bool reportProgress() const;
    [[nodiscard]] int countReachableNodes(const QString &startId) const;
    [[nodiscard]] QStringList exportOrder() const;
    [[nodiscard]] bool hasSelection() const { return !m_selectedNodeIds.isEmpty(); }

    Project *m_project{nullptr};
    QSet<QString> m_visited;
    std::function<bool(int, int)> m_progressCallback;
    int m_totalNodes{0};
    int m_processedNodes{0};
    bool m_wasCanceled{false};
    QSet<QString> m_selectedNodeIds;
    QStringList m_selectionOrder;
};

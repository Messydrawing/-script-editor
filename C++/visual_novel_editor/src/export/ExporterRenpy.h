#pragma once

#include <QSet>
#include <QString>
#include <QTextStream>

class Project;

class ExporterRenpy
{
public:
    explicit ExporterRenpy(Project *project);

    [[nodiscard]] bool exportToFile(const QString &fileName);

private:
    void generateNode(const QString &nodeId, QTextStream &out, int indent = 0);

    Project *m_project{nullptr};
    QSet<QString> m_visited;
};

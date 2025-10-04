#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QPointF>
#include <QString>

class NodeItem;
class StoryNode;
class Project;

class GraphScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GraphScene(QObject *parent = nullptr);

    void setProject(Project *project);
    QString createNode(const QPointF &pos);
    void createEdge(const QString &sourceId, const QString &targetId);

signals:
    void nodeSelected(const QString &nodeId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void rebuild();

    Project *m_project{nullptr};
    QHash<QString, NodeItem *> m_nodeItems;
};

#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QPointF>
#include <QString>
#include <QStringList>

#include "presenter/ViewInterfaces.h"

class NodeItem;
class StoryNode;
class Project;
class EdgeItem;
class Choice;

class GraphScene : public QGraphicsScene, public gui::presenter::IGraphSceneView
{
    Q_OBJECT
public:
    explicit GraphScene(QObject *parent = nullptr);

    void setProject(Project *project) override;
    QString createNode(const QPointF &pos);
    void createEdge(const QString &sourceId, const QString &targetId);
    void refreshNode(const QString &nodeId);

    [[nodiscard]] QStringList selectedNodeIds() const override;

signals:
    void nodeSelected(const QString &nodeId);
    void nodeDoubleClicked(const QString &nodeId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    NodeItem *createNodeItem(StoryNode *node);
    void connectNodeItem(NodeItem *item);
    void rebuildEdges();
    void updateEdgesForNode(NodeItem *item);
    void clearEdges();
    void startBranch(NodeItem *source);
    void finalizeBranch(NodeItem *target);
    void copySelection();
    void deleteSelectionItems();
    void deleteEdges(const QList<EdgeItem *> &edges);
    void deleteNodes(const QList<NodeItem *> &nodes);
    Choice *findChoice(const QString &choiceId);
    StoryNode *nodeForEdge(const EdgeItem *edge) const;
    void updateChoiceText(const QString &choiceId, const QString &text);
    void updateBoundingForAllEdges();

    void rebuild();

    Project *m_project{nullptr};
    QHash<QString, QPointer<NodeItem>> m_nodeItems;
    QList<QPointer<EdgeItem>> m_edgeItems;
    QPointer<NodeItem> m_pendingBranchSource;
};

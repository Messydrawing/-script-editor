#include "GraphScene.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "EdgeItem.h"
#include "NodeItem.h"
#include "model/Choice.h"
#include "model/Project.h"
#include "model/StoryNode.h"

GraphScene::GraphScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-500, -500, 1000, 1000);
}

void GraphScene::setProject(Project *project)
{
    m_project = project;
    rebuild();
}

QString GraphScene::createNode(const QPointF &pos)
{
    if (!m_project) {
        return {};
    }
    StoryNode *node = m_project->addNode(StoryNode::Type::Dialogue);
    node->setPosition(pos);
    rebuild();
    return node->id();
}

void GraphScene::createEdge(const QString &sourceId, const QString &targetId)
{
    if (!m_project) {
        return;
    }
    StoryNode *source = m_project->getNode(sourceId);
    if (!source) {
        return;
    }
    Choice choice;
    choice.id = m_project->generateId();
    choice.text = QStringLiteral("Choice");
    choice.targetNodeId = targetId;
    source->choices().append(choice);
    rebuild();
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        if (NodeItem *nodeItem = qgraphicsitem_cast<NodeItem *>(itemAt(event->scenePos(), QTransform()))) {
            emit nodeSelected(nodeItem->storyNode()->id());
        }
    }
}

void GraphScene::rebuild()
{
    clear();
    m_nodeItems.clear();

    if (!m_project) {
        return;
    }

    for (const StoryNode *node : m_project->nodes()) {
        auto *item = new NodeItem(const_cast<StoryNode *>(node));
        item->setPos(node->position());
        addItem(item);
        m_nodeItems.insert(node->id(), item);
    }

    for (const StoryNode *node : m_project->nodes()) {
        NodeItem *sourceItem = m_nodeItems.value(node->id(), nullptr);
        if (!sourceItem) {
            continue;
        }
        for (const Choice &choice : node->choices()) {
            NodeItem *targetItem = m_nodeItems.value(choice.targetNodeId, nullptr);
            if (targetItem) {
                auto *edge = new EdgeItem(sourceItem, targetItem, nullptr);
                edge->updatePosition();
                addItem(edge);
            }
        }
    }
}

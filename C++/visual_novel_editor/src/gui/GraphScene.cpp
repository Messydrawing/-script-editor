#include "GraphScene.h"

#include <QAction>
#include <QGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHash>
#include <QMenu>
#include <QPair>
#include <QStringList>
#include <QSet>

#include "EdgeItem.h"
#include "NodeItem.h"
#include "model/Choice.h"
#include "model/Project.h"
#include "model/StoryNode.h"

namespace {
constexpr QPointF kDuplicateOffset(60.0, 40.0);
}

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

QStringList GraphScene::selectedNodeIds() const
{
    QStringList ids;
    const QList<QGraphicsItem *> selection = selectedItems();
    ids.reserve(selection.size());
    for (QGraphicsItem *item : selection) {
        if (const auto *nodeItem = qgraphicsitem_cast<NodeItem *>(item)) {
            if (const StoryNode *node = nodeItem->storyNode()) {
                ids.append(node->id());
            }
        }
    }
    return ids;
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
    rebuildEdges();
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pendingBranchSource) {
        if (NodeItem *targetItem = qgraphicsitem_cast<NodeItem *>(itemAt(event->scenePos(), QTransform()))) {
            if (targetItem != m_pendingBranchSource.data()) {
                finalizeBranch(targetItem);
            }
            m_pendingBranchSource = nullptr;
        }
    }

    QGraphicsScene::mousePressEvent(event);

    if (event->button() == Qt::LeftButton) {
        if (NodeItem *nodeItem = qgraphicsitem_cast<NodeItem *>(itemAt(event->scenePos(), QTransform()))) {
            emit nodeSelected(nodeItem->storyNode()->id());
        }
    }
}

void GraphScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    const QPointF scenePos = event->scenePos();
    QGraphicsItem *clickedItem = itemAt(scenePos, QTransform());
    NodeItem *nodeItem = qgraphicsitem_cast<NodeItem *>(clickedItem);
    EdgeItem *edgeItem = nullptr;
    if (!nodeItem) {
        edgeItem = qgraphicsitem_cast<EdgeItem *>(clickedItem);
        if (!edgeItem && clickedItem && clickedItem->parentItem()) {
            edgeItem = qgraphicsitem_cast<EdgeItem *>(clickedItem->parentItem());
        }
    }

    QMenu menu;
    QAction *addNodeAction = nullptr;
    if (nodeItem && !nodeItem->isSelected()) {
        clearSelection();
        nodeItem->setSelected(true);
    } else if (edgeItem && !edgeItem->isSelected()) {
        clearSelection();
        edgeItem->setSelected(true);
    }

    QAction *copyAction = nullptr;
    QAction *cutAction = nullptr;
    QAction *deleteAction = nullptr;
    QAction *branchAction = nullptr;

    const QList<QGraphicsItem *> selection = selectedItems();

    if (nodeItem) {
        copyAction = menu.addAction(tr("Copy"));
        cutAction = menu.addAction(tr("Cut"));
        deleteAction = menu.addAction(tr("Delete"));
        menu.addSeparator();
        branchAction = menu.addAction(tr("Create Branch"));
    } else if (!selection.isEmpty()) {
        copyAction = menu.addAction(tr("Copy"));
        cutAction = menu.addAction(tr("Cut"));
        deleteAction = menu.addAction(tr("Delete"));
    } else if (edgeItem) {
        deleteAction = menu.addAction(tr("Delete"));
    } else {
        addNodeAction = menu.addAction(tr("Add Node"));
    }

    if (menu.isEmpty()) {
        event->accept();
        return;
    }

    QAction *chosen = menu.exec(event->screenPos());
    if (!chosen) {
        event->accept();
        return;
    }

    if (chosen == addNodeAction) {
        const QString newId = createNode(scenePos);
        if (!newId.isEmpty()) {
            if (NodeItem *created = m_nodeItems.value(newId).data()) {
                clearSelection();
                created->setSelected(true);
            }
            emit nodeSelected(newId);
        }
    } else if (chosen == copyAction) {
        copySelection();
    } else if (chosen == cutAction) {
        deleteSelectionItems();
    } else if (chosen == deleteAction) {
        deleteSelectionItems();
    } else if (chosen == branchAction) {
        startBranch(nodeItem);
    }

    event->accept();
}

void GraphScene::rebuild()
{
    clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
    m_pendingBranchSource = nullptr;

    if (!m_project) {
        return;
    }

    for (auto it = m_project->nodes().cbegin(); it != m_project->nodes().cend(); ++it) {
        StoryNode *node = it.value().get();
        if (!node) {
            continue;
        }
        NodeItem *item = createNodeItem(node);
        addItem(item);
        item->setPos(node->position());
        m_nodeItems.insert(node->id(), item);
    }

    rebuildEdges();
}

void GraphScene::refreshNode(const QString &nodeId)
{
    if (NodeItem *item = m_nodeItems.value(nodeId).data()) {
        item->update();
        updateEdgesForNode(item);
    }
}

NodeItem *GraphScene::createNodeItem(StoryNode *node)
{
    auto *item = new NodeItem(node);
    item->setParent(this);
    connectNodeItem(item);
    return item;
}

void GraphScene::connectNodeItem(NodeItem *item)
{
    if (!item) {
        return;
    }
    connect(item, &NodeItem::positionChanged, this, [this](const QString &id, const QPointF &) {
        if (NodeItem *sourceItem = m_nodeItems.value(id).data()) {
            updateEdgesForNode(sourceItem);
        }
    });
    connect(item, &NodeItem::doubleClicked, this, [this](const QString &id) {
        if (!id.isEmpty()) {
            emit nodeDoubleClicked(id);
        }
    });
}

void GraphScene::rebuildEdges()
{
    clearEdges();
    if (!m_project) {
        return;
    }

    QHash<QPair<NodeItem *, NodeItem *>, QList<EdgeItem *>> groupedEdges;

    for (auto it = m_project->nodes().cbegin(); it != m_project->nodes().cend(); ++it) {
        StoryNode *node = it.value().get();
        if (!node) {
            continue;
        }
        NodeItem *sourceItem = m_nodeItems.value(node->id()).data();
        if (!sourceItem) {
            continue;
        }
        for (const Choice &choice : node->choices()) {
            NodeItem *targetItem = m_nodeItems.value(choice.targetNodeId).data();
            if (!targetItem) {
                continue;
            }
            auto *edge = new EdgeItem(sourceItem, targetItem, choice.id);
            edge->setParent(this);
            addItem(edge);
            edge->setLabelText(choice.text);
            connect(edge, &EdgeItem::labelEdited, this, &GraphScene::updateChoiceText);
            m_edgeItems.append(edge);
            groupedEdges[qMakePair(sourceItem, targetItem)].append(edge);
        }
    }

    for (auto it = groupedEdges.begin(); it != groupedEdges.end(); ++it) {
        const QList<EdgeItem *> edges = it.value();
        const int total = edges.size();
        for (int index = 0; index < edges.size(); ++index) {
            if (EdgeItem *edge = edges.at(index)) {
                edge->setParallelInfo(index, total);
            }
        }
    }

    updateBoundingForAllEdges();
}

void GraphScene::updateEdgesForNode(NodeItem *item)
{
    for (const QPointer<EdgeItem> &edgePtr : m_edgeItems) {
        if (EdgeItem *edge = edgePtr.data()) {
            if (edge->sourceItem() == item || edge->targetItem() == item) {
                edge->updatePosition();
            }
        }
    }
    updateBoundingForAllEdges();
}

void GraphScene::clearEdges()
{
    for (const QPointer<EdgeItem> &edgePtr : m_edgeItems) {
        if (EdgeItem *edge = edgePtr.data()) {
            removeItem(edge);
            edge->deleteLater();
        }
    }
    m_edgeItems.clear();
}

void GraphScene::startBranch(NodeItem *source)
{
    m_pendingBranchSource = source;
}

void GraphScene::finalizeBranch(NodeItem *target)
{
    NodeItem *sourceItem = m_pendingBranchSource.data();
    if (!m_project || !sourceItem || !target) {
        return;
    }
    StoryNode *sourceNode = sourceItem->storyNode();
    StoryNode *targetNode = target->storyNode();
    if (!sourceNode || !targetNode || sourceNode == targetNode) {
        return;
    }

    Choice choice;
    choice.id = m_project->generateId();
    choice.text = QStringLiteral("Choice");
    choice.targetNodeId = targetNode->id();
    sourceNode->choices().append(choice);

    rebuildEdges();
}

void GraphScene::copySelection()
{
    if (!m_project) {
        return;
    }

    QList<NodeItem *> selectedNodes;
    for (QGraphicsItem *item : selectedItems()) {
        if (auto *node = qgraphicsitem_cast<NodeItem *>(item)) {
            selectedNodes.append(node);
        }
    }

    if (selectedNodes.isEmpty()) {
        return;
    }

    QHash<QString, StoryNode *> clonedNodes;
    for (NodeItem *nodeItem : selectedNodes) {
        if (!nodeItem->storyNode()) {
            continue;
        }
        StoryNode *original = nodeItem->storyNode();
        StoryNode *clone = m_project->addNode(original->type());
        clone->setTitle(original->title());
        clone->setScript(original->script());
        clone->setPosition(original->position() + kDuplicateOffset);
        clonedNodes.insert(original->id(), clone);
    }

    for (NodeItem *nodeItem : selectedNodes) {
        StoryNode *original = nodeItem->storyNode();
        StoryNode *clone = clonedNodes.value(original->id(), nullptr);
        if (!clone || !original) {
            continue;
        }
        for (const Choice &choice : original->choices()) {
            if (!clonedNodes.contains(choice.targetNodeId)) {
                continue;
            }
            Choice newChoice;
            newChoice.id = m_project->generateId();
            newChoice.text = choice.text;
            newChoice.targetNodeId = clonedNodes.value(choice.targetNodeId)->id();
            newChoice.condition = choice.condition;
            clone->choices().append(newChoice);
        }
    }

    rebuild();
}

void GraphScene::deleteSelectionItems()
{
    QList<NodeItem *> nodes;
    QList<EdgeItem *> edges;
    for (QGraphicsItem *item : selectedItems()) {
        if (auto *node = qgraphicsitem_cast<NodeItem *>(item)) {
            nodes.append(node);
        } else if (auto *edge = qgraphicsitem_cast<EdgeItem *>(item)) {
            edges.append(edge);
        }
    }

    if (!edges.isEmpty()) {
        deleteEdges(edges);
    }
    if (!nodes.isEmpty()) {
        deleteNodes(nodes);
    }
    rebuild();
}

void GraphScene::deleteEdges(const QList<EdgeItem *> &edges)
{
    for (EdgeItem *edge : edges) {
        if (!edge) {
            continue;
        }
        StoryNode *node = nodeForEdge(edge);
        if (!node) {
            continue;
        }
        auto &choices = node->choices();
        for (int i = choices.size() - 1; i >= 0; --i) {
            if (choices[i].id == edge->choiceId()) {
                choices.removeAt(i);
            }
        }
    }
}

void GraphScene::deleteNodes(const QList<NodeItem *> &nodes)
{
    if (!m_project) {
        return;
    }

    QStringList ids;
    ids.reserve(nodes.size());
    for (NodeItem *item : nodes) {
        if (item && item->storyNode()) {
            ids.append(item->storyNode()->id());
        }
    }

    const QSet<QString> idSet(ids.cbegin(), ids.cend());

    for (auto it = m_project->nodes().cbegin(); it != m_project->nodes().cend(); ++it) {
        StoryNode *node = it.value().get();
        if (!node) {
            continue;
        }
        auto &choices = node->choices();
        for (int i = choices.size() - 1; i >= 0; --i) {
            if (idSet.contains(choices[i].targetNodeId)) {
                choices.removeAt(i);
            }
        }
    }

    for (const QString &id : ids) {
        m_project->removeNode(id);
    }
}

Choice *GraphScene::findChoice(const QString &choiceId)
{
    if (!m_project) {
        return nullptr;
    }
    for (auto it = m_project->nodes().cbegin(); it != m_project->nodes().cend(); ++it) {
        StoryNode *node = it.value().get();
        if (!node) {
            continue;
        }
        auto &choices = node->choices();
        for (Choice &choice : choices) {
            if (choice.id == choiceId) {
                return &choice;
            }
        }
    }
    return nullptr;
}

StoryNode *GraphScene::nodeForEdge(const EdgeItem *edge) const
{
    if (!edge || !m_project) {
        return nullptr;
    }
    NodeItem *sourceItem = edge->sourceItem();
    if (!sourceItem) {
        return nullptr;
    }
    return sourceItem->storyNode();
}

void GraphScene::updateChoiceText(const QString &choiceId, const QString &text)
{
    if (Choice *choice = findChoice(choiceId)) {
        choice->text = text;
    }
}

void GraphScene::updateBoundingForAllEdges()
{
    for (const QPointer<EdgeItem> &edgePtr : m_edgeItems) {
        if (EdgeItem *edge = edgePtr.data()) {
            edge->update();
        }
    }
}

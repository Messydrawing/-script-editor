#pragma once

#include <QGraphicsObject>
#include <QPointF>
#include <QString>

class StoryNode;

class NodeItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit NodeItem(StoryNode *node, QGraphicsItem *parent = nullptr);
    ~NodeItem() override = default;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    StoryNode *storyNode() const { return m_node; }

signals:
    void positionChanged(const QString &nodeId, const QPointF &newPos);
    void doubleClicked(const QString &nodeId);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    StoryNode *m_node{nullptr};
};

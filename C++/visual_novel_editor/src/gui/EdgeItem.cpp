#include "EdgeItem.h"

#include <QPainter>
#include <QPen>

#include "NodeItem.h"

EdgeItem::EdgeItem(NodeItem *source, NodeItem *target, Choice *choice, QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
    , m_source(source)
    , m_target(target)
    , m_choice(choice)
{
    setFlag(ItemIsSelectable, true);
    setPen(QPen(QColor(230, 230, 230), 2.0));
}

void EdgeItem::updatePosition()
{
    if (!m_source || !m_target) {
        return;
    }
    const QPointF startPoint = m_source->scenePos();
    const QPointF endPoint = m_target->scenePos();
    setLine(QLineF(startPoint, endPoint));
}

#include "NodeItem.h"

#include <QColor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>

#include "model/StoryNode.h"

namespace {
constexpr qreal kNodeWidth = 160.0;
constexpr qreal kNodeHeight = 80.0;
}

NodeItem::NodeItem(StoryNode *node, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_node(node)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setCacheMode(DeviceCoordinateCache);
}

QRectF NodeItem::boundingRect() const
{
    return QRectF(-kNodeWidth / 2.0, -kNodeHeight / 2.0, kNodeWidth, kNodeHeight);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRectF rect = boundingRect();
    QPen pen(Qt::darkGray, isSelected() ? 2.0 : 1.0);
    painter->setPen(pen);
    painter->setBrush(QColor(80, 120, 180, 200));
    painter->drawRoundedRect(rect, 8.0, 8.0);

    if (m_node) {
        painter->setPen(Qt::white);
        const QString title = m_node->title();
        painter->drawText(rect.adjusted(8.0, 8.0, -8.0, -8.0), Qt::AlignTop | Qt::AlignLeft, title);
    }
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && m_node) {
        m_node->setPosition(pos());
        emit positionChanged(m_node->id(), pos());
    }
    return QGraphicsObject::itemChange(change, value);
}

void NodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mouseDoubleClickEvent(event);
    if (m_node) {
        emit doubleClicked(m_node->id());
    }
}

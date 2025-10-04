#include "EdgeItem.h"

#include <QFocusEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QTextDocument>
#include <QtMath>
#include <cmath>

#include "NodeItem.h"

// 保持为 .cpp 内部私有类，不对外暴露
namespace {

class EditableLabelItem : public QGraphicsTextItem
{
public:
    explicit EditableLabelItem(QGraphicsItem *parent = nullptr)
        : QGraphicsTextItem(parent)
    {
        setDefaultTextColor(Qt::black);
        document()->setDocumentMargin(4.0);
    }

protected:
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect = boundingRect();
        painter->setPen(QPen(Qt::black, 1.0));
        painter->setBrush(Qt::white);
        painter->drawRoundedRect(rect, 4.0, 4.0);
        painter->setPen(Qt::black);
        QGraphicsTextItem::paint(painter, option, widget);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setFocus(Qt::MouseFocusReason);
        QGraphicsTextItem::mousePressEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        setTextInteractionFlags(Qt::NoTextInteraction);
        QGraphicsTextItem::focusOutEvent(event);
    }
};

QPointF anchorForRect(const QRectF &rect, const QPointF &towards)
{
    const QPointF center = rect.center();
    const QPointF delta  = towards - center;
    if (std::abs(delta.x()) >= std::abs(delta.y())) {
        return (delta.x() >= 0)
        ? QPointF(rect.right(),  center.y())
        : QPointF(rect.left(),   center.y());
    }
    return (delta.y() >= 0)
               ? QPointF(center.x(), rect.bottom())
               : QPointF(center.x(), rect.top());
}

QPointF orthogonalCorner(const QPointF &start, const QPointF &end)
{
    if (qFuzzyCompare(start.x(), end.x()) || qFuzzyCompare(start.y(), end.y())) {
        return QPointF((start.x() + end.x()) / 2.0,
                       (start.y() + end.y()) / 2.0);
    }
    if (std::abs(start.x() - end.x()) >= std::abs(start.y() - end.y())) {
        return QPointF(end.x(), start.y());
    }
    return QPointF(start.x(), end.y());
}

} // namespace

EdgeItem::EdgeItem(NodeItem *source, NodeItem *target,
                   const QString &choiceId, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_source(source)
    , m_target(target)
    , m_choiceId(choiceId)
    , m_label(new EditableLabelItem(this))   // ✅ 现在赋给基类指针，类型一致
{
    setFlag(ItemIsSelectable, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setZValue(-0.5);

    m_label->setPlainText(QStringLiteral("Choice"));
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    m_label->setZValue(1.0);

    // 文本变化 → 更新位置并发出 labelEdited
    connect(m_label->document(), &QTextDocument::contentsChanged,
            this, [this]() {
                if (m_ignoreLabelSignal) return;
                updateLabelPosition();
                emit labelEdited(m_choiceId, m_label->toPlainText());
            });
}

void EdgeItem::setLabelText(const QString &text)
{
    m_ignoreLabelSignal = true;
    m_label->setPlainText(text);
    m_ignoreLabelSignal = false;
    updateLabelPosition();
}

void EdgeItem::updatePosition()
{
    if (!m_source || !m_target) return;

    const QRectF sourceRect = m_source->sceneBoundingRect();
    const QRectF targetRect = m_target->sceneBoundingRect();

    const QPointF start  = anchorForRect(sourceRect, targetRect.center());
    const QPointF end    = anchorForRect(targetRect, sourceRect.center());
    const QPointF corner = orthogonalCorner(start, end);

    QPainterPath path(start);
    if (!qFuzzyCompare(start.x(), corner.x()) ||
        !qFuzzyCompare(start.y(), corner.y()))
        path.lineTo(corner);
    if (!qFuzzyCompare(corner.x(), end.x()) ||
        !qFuzzyCompare(corner.y(), end.y()))
        path.lineTo(end);

    prepareGeometryChange();
    m_path = path;
    m_boundingRect = m_path.boundingRect();
    updateLabelPosition();
}

void EdgeItem::updateLabelPosition()
{
    if (!m_label) return;

    const QPointF midPoint = m_path.isEmpty()
                                 ? QPointF()
                                 : m_path.pointAtPercent(0.5);

    const QRectF labelRect = m_label->boundingRect();
    m_label->setPos(midPoint.x() - labelRect.width()  / 2.0,
                    midPoint.y() - labelRect.height() / 2.0);

    const QRectF baseRect       = m_path.boundingRect();
    const QRectF labelSceneRect = m_label->sceneBoundingRect();
    const QRectF labelLocalRect = mapFromScene(labelSceneRect).boundingRect();
    m_boundingRect = baseRect.united(labelLocalRect).adjusted(-6.0, -6.0, 6.0, 6.0);

    update();
}

void EdgeItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_path.isEmpty()) return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(QColor(50, 50, 50), isSelected() ? 3.0 : 2.0);
    painter->setPen(pen);
    painter->drawPath(m_path);

    // 箭头
    const QPointF endPoint = m_path.pointAtPercent(1.0);
    const QPointF tangent  = m_path.pointAtPercent(0.99);
    const QPointF dir      = (endPoint - tangent);
    if (!dir.isNull()) {
        constexpr double arrowSize = 12.0;
        const double angle  = std::atan2(dir.y(), dir.x());
        const double offset = qDegreesToRadians(30.0);

        const QPointF a1 = endPoint -
                           QPointF(std::cos(angle - offset) * arrowSize,
                                   std::sin(angle - offset) * arrowSize);
        const QPointF a2 = endPoint -
                           QPointF(std::cos(angle + offset) * arrowSize,
                                   std::sin(angle + offset) * arrowSize);

        QPainterPath arrowPath(endPoint);
        arrowPath.lineTo(a1);
        arrowPath.lineTo(a2);
        arrowPath.closeSubpath();

        painter->setBrush(QColor(50, 50, 50));
        painter->drawPath(arrowPath);
    }
}

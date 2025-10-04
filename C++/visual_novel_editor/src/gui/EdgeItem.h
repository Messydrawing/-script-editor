#pragma once

#include <QGraphicsObject>
#include <QPainterPath>
#include <QString>

class NodeItem;
class Choice;

class EditableLabelItem;

class EdgeItem : public QGraphicsObject
{
    Q_OBJECT
public:
    EdgeItem(NodeItem *source, NodeItem *target, const QString &choiceId, QGraphicsItem *parent = nullptr);

    void updatePosition();
    QRectF boundingRect() const override { return m_boundingRect; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    NodeItem *sourceItem() const { return m_source; }
    NodeItem *targetItem() const { return m_target; }
    QString choiceId() const { return m_choiceId; }

    void setLabelText(const QString &text);

signals:
    void labelEdited(const QString &choiceId, const QString &text);

private:
    void updateLabelPosition();

    NodeItem *m_source{nullptr};
    NodeItem *m_target{nullptr};
    QString m_choiceId;
    QPainterPath m_path;
    QRectF m_boundingRect;
    EditableLabelItem *m_label{nullptr};
    bool m_ignoreLabelSignal{false};
};

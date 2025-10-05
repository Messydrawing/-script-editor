#pragma once

#include <QGraphicsObject>
#include <QGraphicsTextItem>   // ✅ 基类声明放到头里，成员用这个类型
#include <QPainterPath>
#include <QString>

class NodeItem;
// class Choice;               // 若未使用可删除

// ⚠️ 删掉对 EditableLabelItem 的前向声明，避免与 .cpp 内部类冲突
// class EditableLabelItem;

class EdgeItem : public QGraphicsObject
{
    Q_OBJECT
public:
    EdgeItem(NodeItem *source, NodeItem *target, const QString &choiceId,
             QGraphicsItem *parent = nullptr);

    void updatePosition();
    void setParallelInfo(int index, int total);

    QRectF boundingRect() const override { return m_boundingRect; }
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    NodeItem *sourceItem() const { return m_source; }
    NodeItem *targetItem() const { return m_target; }
    QString   choiceId()   const { return m_choiceId; }

    void setLabelText(const QString &text);

signals:
    void labelEdited(const QString &choiceId, const QString &text);

private:
    void updateLabelPosition();

    NodeItem *m_source{nullptr};
    NodeItem *m_target{nullptr};
    QString   m_choiceId;

    QPainterPath m_path;
    QRectF       m_boundingRect;

    QGraphicsTextItem *m_label{nullptr};  // ✅ 改为基类指针，消除不完全类型/二义性
    bool m_ignoreLabelSignal{false};
    int m_parallelIndex{0};
    int m_parallelCount{1};
};

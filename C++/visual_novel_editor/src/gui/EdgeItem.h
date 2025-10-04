#pragma once

#include <QGraphicsLineItem>

class NodeItem;
class Choice;

class EdgeItem : public QGraphicsLineItem
{
public:
    EdgeItem(NodeItem *source, NodeItem *target, Choice *choice = nullptr, QGraphicsItem *parent = nullptr);

    void updatePosition();
    Choice *choice() const { return m_choice; }

private:
    NodeItem *m_source{nullptr};
    NodeItem *m_target{nullptr};
    Choice *m_choice{nullptr};
};

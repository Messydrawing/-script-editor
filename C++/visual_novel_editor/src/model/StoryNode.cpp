#include "StoryNode.h"

#include <QJsonArray>
#include <QJsonValue>

namespace {
QString typeToString(StoryNode::Type type)
{
    switch (type) {
    case StoryNode::Type::Dialogue:
        return QStringLiteral("dialogue");
    case StoryNode::Type::Menu:
        return QStringLiteral("menu");
    case StoryNode::Type::Jump:
        return QStringLiteral("jump");
    case StoryNode::Type::End:
        return QStringLiteral("end");
    }
    return QStringLiteral("dialogue");
}

StoryNode::Type typeFromString(const QString &value)
{
    if (value == QStringLiteral("menu")) {
        return StoryNode::Type::Menu;
    }
    if (value == QStringLiteral("jump")) {
        return StoryNode::Type::Jump;
    }
    if (value == QStringLiteral("end")) {
        return StoryNode::Type::End;
    }
    return StoryNode::Type::Dialogue;
}
} // namespace

StoryNode::StoryNode(const QString &id)
    : m_id(id)
{
}

QJsonObject StoryNode::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = m_id;
    obj[QStringLiteral("title")] = m_title;
    obj[QStringLiteral("script")] = m_script;
    obj[QStringLiteral("type")] = typeToString(m_type);

    QJsonArray choicesArray;
    for (const Choice &choice : m_choices) {
        choicesArray.append(choice.toJson());
    }
    obj[QStringLiteral("choices")] = choicesArray;

    QJsonObject pos;
    pos[QStringLiteral("x")] = m_position.x();
    pos[QStringLiteral("y")] = m_position.y();
    obj[QStringLiteral("position")] = pos;

    return obj;
}

StoryNode StoryNode::fromJson(const QJsonObject &obj)
{
    StoryNode node(obj.value(QStringLiteral("id")).toString());
    node.m_title = obj.value(QStringLiteral("title")).toString();
    node.m_script = obj.value(QStringLiteral("script")).toString();
    node.m_type = typeFromString(obj.value(QStringLiteral("type")).toString());

    const QJsonArray choicesArray = obj.value(QStringLiteral("choices")).toArray();
    for (const QJsonValue &value : choicesArray) {
        node.m_choices.append(Choice::fromJson(value.toObject()));
    }

    const QJsonObject pos = obj.value(QStringLiteral("position")).toObject();
    node.m_position.setX(pos.value(QStringLiteral("x")).toDouble());
    node.m_position.setY(pos.value(QStringLiteral("y")).toDouble());

    return node;
}

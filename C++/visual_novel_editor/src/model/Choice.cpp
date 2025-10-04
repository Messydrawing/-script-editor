#include "Choice.h"

QJsonObject Choice::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("text")] = text;
    obj[QStringLiteral("target")] = targetNodeId;
    if (condition.has_value()) {
        obj[QStringLiteral("condition")] = *condition;
    }
    return obj;
}

Choice Choice::fromJson(const QJsonObject &obj)
{
    Choice choice;
    choice.id = obj.value(QStringLiteral("id")).toString();
    choice.text = obj.value(QStringLiteral("text")).toString();
    choice.targetNodeId = obj.value(QStringLiteral("target")).toString();
    if (obj.contains(QStringLiteral("condition"))) {
        choice.condition = obj.value(QStringLiteral("condition")).toString();
    }
    return choice;
}

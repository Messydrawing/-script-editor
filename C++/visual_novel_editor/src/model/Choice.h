#pragma once

#include <QJsonObject>
#include <QString>
#include <optional>

struct Choice {
    QString id;
    QString text;
    QString targetNodeId;
    std::optional<QString> condition;

    [[nodiscard]] QJsonObject toJson() const;
    static Choice fromJson(const QJsonObject &obj);
};

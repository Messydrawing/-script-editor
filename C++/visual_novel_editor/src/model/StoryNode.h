#pragma once

#include <QJsonObject>
#include <QList>
#include <QPointF>
#include <QString>

#include "Choice.h"

class StoryNode
{
public:
    enum class Type { Dialogue, Menu, Jump, End };

    explicit StoryNode(const QString &id = {});

    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }

    QString title() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }

    QString script() const { return m_script; }
    void setScript(const QString &script) { m_script = script; }

    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }

    QList<Choice> &choices() { return m_choices; }
    const QList<Choice> &choices() const { return m_choices; }

    QPointF position() const { return m_position; }
    void setPosition(const QPointF &pos) { m_position = pos; }

    [[nodiscard]] QJsonObject toJson() const;
    static StoryNode fromJson(const QJsonObject &obj);

private:
    QString m_id;
    QString m_title;
    QString m_script;
    Type m_type{Type::Dialogue};
    QList<Choice> m_choices;
    QPointF m_position{};
};

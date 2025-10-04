#pragma once

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

#include "StoryNode.h"

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);
    ~Project() override;

    StoryNode *addNode(StoryNode::Type type);
    void removeNode(const QString &nodeId);
    void clear();

    StoryNode *getNode(const QString &nodeId);
    const StoryNode *getNode(const QString &nodeId) const;
    const QMap<QString, StoryNode *> &nodes() const { return m_nodes; }

    [[nodiscard]] bool loadFromFile(const QString &fileName);
    [[nodiscard]] bool saveToFile(const QString &fileName) const;

    QString generateId();

signals:
    void changed();

private:
    QMap<QString, StoryNode *> m_nodes;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
};

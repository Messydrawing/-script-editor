#include "Project.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Utilities.h"

Project::Project(QObject *parent)
    : QObject(parent)
{
}

Project::~Project()
{
    qDeleteAll(m_nodes);
    m_nodes.clear();
}

StoryNode *Project::addNode(StoryNode::Type type)
{
    auto *node = new StoryNode(generateId());
    node->setType(type);
    node->setTitle(QStringLiteral("New Node"));
    m_nodes.insert(node->id(), node);
    emit changed();
    return node;
}

void Project::removeNode(const QString &nodeId)
{
    if (StoryNode *node = m_nodes.take(nodeId)) {
        delete node;
        emit changed();
    }
}

void Project::clear()
{
    qDeleteAll(m_nodes);
    m_nodes.clear();
    emit changed();
}

StoryNode *Project::getNode(const QString &nodeId)
{
    return m_nodes.value(nodeId, nullptr);
}

const StoryNode *Project::getNode(const QString &nodeId) const
{
    return m_nodes.value(nodeId, nullptr);
}

bool Project::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    const QJsonDocument document = QJsonDocument::fromJson(data);
    if (!document.isObject()) {
        return false;
    }

    fromJson(document.object());
    emit changed();
    return true;
}

bool Project::saveToFile(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QJsonDocument document(toJson());
    file.write(document.toJson(QJsonDocument::Indented));
    return true;
}

QString Project::generateId()
{
    return generateUuid();
}

QJsonObject Project::toJson() const
{
    QJsonObject root;
    QJsonArray nodesArray;
    for (const StoryNode *node : m_nodes) {
        nodesArray.append(node->toJson());
    }
    root[QStringLiteral("nodes")] = nodesArray;
    return root;
}

void Project::fromJson(const QJsonObject &json)
{
    clear();

    const QJsonArray nodesArray = json.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue &value : nodesArray) {
        const StoryNode node = StoryNode::fromJson(value.toObject());
        auto *nodePtr = new StoryNode(node);
        m_nodes.insert(nodePtr->id(), nodePtr);
    }
}

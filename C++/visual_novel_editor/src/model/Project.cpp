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

StoryNode *Project::addNode(StoryNode::Type type)
{
    auto node = std::make_shared<StoryNode>(generateId());
    node->setType(type);
    node->setTitle(QStringLiteral("New Node"));
    StoryNode *nodePtr = node.get();
    m_nodes.insert(node->id(), node);
    emit changed();
    return nodePtr;
}

void Project::removeNode(const QString &nodeId)
{
    m_nodes.remove(nodeId);

    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        StoryNode *otherNode = it.value().get();
        if (!otherNode) {
            continue;
        }
        auto &choices = otherNode->choices();
        for (int i = choices.size() - 1; i >= 0; --i) {
            if (choices[i].targetNodeId == nodeId) {
                choices.removeAt(i);
            }
        }
    }
    emit changed();
}

void Project::clear()
{
    m_nodes.clear();
    emit changed();
}

StoryNode *Project::getNode(const QString &nodeId)
{
    if (auto it = m_nodes.find(nodeId); it != m_nodes.end()) {
        return it.value().get();
    }
    return nullptr;
}

const StoryNode *Project::getNode(const QString &nodeId) const
{
    if (auto it = m_nodes.constFind(nodeId); it != m_nodes.cend()) {
        return it.value().get();
    }
    return nullptr;
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
    for (auto it = m_nodes.cbegin(); it != m_nodes.cend(); ++it) {
        const StoryNode *node = it.value().get();
        if (node) {
            nodesArray.append(node->toJson());
        }
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
        auto nodePtr = std::make_shared<StoryNode>(node);
        m_nodes.insert(nodePtr->id(), nodePtr);
    }
}

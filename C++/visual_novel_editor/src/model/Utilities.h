#pragma once

#include <QString>
#include <QUuid>

inline QString generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

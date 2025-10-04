#pragma once

#include <QString>

class ScriptFormatter
{
public:
    ScriptFormatter() = delete;

    static QString indent(int spaces);
};

#include "ScriptFormatter.h"

QString ScriptFormatter::indent(int spaces)
{
    if (spaces <= 0) {
        return {};
    }
    return QString(spaces, QLatin1Char(' '));
}

#include <QApplication>

#include "gui/LanguageManager.h"
#include "gui/MainWindow.h"
#include "model/Project.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LanguageManager::instance().initialize(&app);

    Project project;

    MainWindow mainWindow;
    mainWindow.setProject(&project);
    mainWindow.show();

    return app.exec();
}

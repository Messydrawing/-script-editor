#include <QApplication>
#include "gui/MainWindow.h"
#include "model/Project.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Project project;
    // TODO: load project if filename provided via argv

    MainWindow mainWindow;
    mainWindow.setProject(&project);
    mainWindow.show();

    return app.exec();
}

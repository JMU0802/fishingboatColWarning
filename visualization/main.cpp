#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AIS 碰撞预警可视化");
    app.setOrganizationName("FishingBoatColWarning");

    MainWindow w;
    w.show();

    return app.exec();
}

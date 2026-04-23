#include <QApplication>
#include <QWebEngineProfile>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("AIS Realtime Visualizer");
    app.setOrganizationName("FishingBoatColWarning");

    // 使用内存缓存避免磁盘缓存目录竞争，减少启动 cache 移动告警
    QString storageDir = QCoreApplication::applicationDirPath() + "/webstorage";
    QWebEngineProfile::defaultProfile()->setPersistentStoragePath(storageDir);
    QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);

    MainWindow w;
    w.show();

    return app.exec();
}

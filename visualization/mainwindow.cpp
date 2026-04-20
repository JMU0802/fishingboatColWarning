#include "mainwindow.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QWebEnginePage>
#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTime>

// --------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("AIS 碰撞预警可视化系统");
    resize(1280, 760);

    setupUi();
    setupConnections();
}

// --------------------------------------------------------------------------
void MainWindow::setupUi()
{
    // --- 地图视图 ---
    m_mapView = new QWebEngineView(this);

    QString mapPath = QCoreApplication::applicationDirPath()
                      + "/resources/map.html";
    m_mapView->load(QUrl::fromLocalFile(mapPath));

    // --- AIS 列表 ---
    m_model     = new AisListModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableView->setMinimumWidth(400);

    // --- 分割器（左图右表）---
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_mapView);
    splitter->addWidget(m_tableView);
    splitter->setStretchFactor(0, 3);   // 地图占 3/4
    splitter->setStretchFactor(1, 1);   // 列表占 1/4
    setCentralWidget(splitter);

    // --- 状态栏 ---
    m_statusLabel = new QLabel("就绪", this);
    statusBar()->addWidget(m_statusLabel);

    // --- AIS 数据读取器（读文件 / 串口均可）---
    m_reader = new AisReader(this);
}

// --------------------------------------------------------------------------
void MainWindow::setupConnections()
{
    connect(m_reader,    &AisReader::dataUpdated,
            this,        &MainWindow::onAisUpdated);

    connect(m_tableView, &QTableView::clicked,
            this,        &MainWindow::onTableRowClicked);

    connect(m_mapView,   &QWebEngineView::loadFinished,
            this,        &MainWindow::onMapReady);
}

// --------------------------------------------------------------------------
void MainWindow::onMapReady(bool ok)
{
    if (!ok) return;
    m_mapLoaded = true;
    // 地图就绪后推送已有数据
    pushMarkersToMap(m_model->targets());
}

// --------------------------------------------------------------------------
void MainWindow::onAisUpdated()
{
    const QVector<myAISData>& targets = m_reader->targets();
    m_model->setTargets(targets);
    if (m_mapLoaded)
        pushMarkersToMap(targets);
    m_statusLabel->setText(
        QString("目标数: %1  |  最后更新: %2")
            .arg(targets.size())
            .arg(QTime::currentTime().toString("hh:mm:ss")));
}

// --------------------------------------------------------------------------
void MainWindow::onTableRowClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    const myAISData& t = m_model->targets().at(index.row());
    if (t.Lat != NO_VALID_LATLON && t.Lon != NO_VALID_LATLON)
        panMapTo(t.Lat, t.Lon);
}

// --------------------------------------------------------------------------
// 将所有 AIS 目标序列化为 JSON，通过 JavaScript 推送到 Leaflet 地图
// --------------------------------------------------------------------------
void MainWindow::pushMarkersToMap(const QVector<myAISData>& targets)
{
    QJsonArray arr;
    for (const myAISData& t : targets)
    {
        if (t.Lat == NO_VALID_LATLON || t.Lon == NO_VALID_LATLON)
            continue;
        QJsonObject obj;
        obj["mmsi"]     = t.MMSI;
        obj["lat"]      = t.Lat;
        obj["lon"]      = t.Lon;
        obj["name"]     = QString::fromLatin1(t.ShipName).trimmed();
        obj["sog"]      = t.SOG;
        obj["cog"]      = t.COG;
        obj["hdg"]      = t.HDG;
        obj["navstatus"]= t.NavStatus;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QString js = QString("updateMarkers(%1);").arg(
        QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    m_mapView->page()->runJavaScript(js);
}

// --------------------------------------------------------------------------
void MainWindow::panMapTo(double lat, double lon)
{
    m_mapView->page()->runJavaScript(
        QString("map.setView([%1, %2], 14);").arg(lat).arg(lon));
}

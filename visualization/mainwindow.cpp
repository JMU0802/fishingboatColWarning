#include "mainwindow.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDockWidget>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTime>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineSettings>

#include "colcalc.h"

// ---------------------------------------------------------------
// 构造
// ---------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8("渔船避碰监控系统"));
    resize(1440, 860);

    setupUi();
    setupConnections();
}

// ---------------------------------------------------------------
// UI 初始化
// ---------------------------------------------------------------
void MainWindow::setupUi()
{
    // ── WebChannel / Bridge ──
    m_bridge  = new MapBridge(this);
    m_channel = new QWebChannel(this);
    m_channel->registerObject(QStringLiteral("mapBridge"), m_bridge);

    // ── 地图视图 ──
    m_mapView = new QWebEngineView(this);
    m_mapView->page()->setWebChannel(m_channel);
    m_mapView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_mapView->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);

    const QString mapPath = QCoreApplication::applicationDirPath() + "/resources/map.html";
    m_mapView->load(QUrl::fromLocalFile(mapPath));

    // ── AIS 原始目标表 ──
    m_aisModel = new AisListModel(this);
    m_aisTable = new QTableView(this);
    m_aisTable->setModel(m_aisModel);
    m_aisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_aisTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_aisTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // ── 碰撞信息表 ──
    m_colModel = new CollisionTableModel(this);
    m_colTable = new QTableView(this);
    m_colTable->setModel(m_colModel);
    m_colTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_colTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_colTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // ── 控制面板 ──
    m_controlPanel = new ControlPanel(this);

    // ── 渔船状态标签 ──
    m_boatLabel = new QLabel(QString::fromUtf8("渔船：未放置（点击地图添加）"), this);
    m_boatLabel->setStyleSheet("font-weight:bold; color:#006688;");

    // ── 右侧面板 ──
    QTabWidget* tabW = new QTabWidget(this);
    tabW->addTab(m_aisTable, QString::fromUtf8("AIS目标"));
    tabW->addTab(m_colTable, QString::fromUtf8("碰撞信息"));
    tabW->setMinimumWidth(460);

    QWidget*     rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(m_boatLabel);
    rightLayout->addWidget(m_controlPanel);
    rightLayout->addWidget(tabW, 1);
    rightWidget->setLayout(rightLayout);

    // ── 主分割 ──
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_mapView);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    // ── 状态栏 ──
    m_statusLabel = new QLabel(QString::fromUtf8("就绪"), this);
    statusBar()->addWidget(m_statusLabel);

    // ── 业务对象 ──
    m_reader = new AisReader(this);
    m_boat   = new FishingBoat(this);
    m_speechNotifier = new SpeechNotifier(this);
    m_boat->startSim();

    // ── 碰撞刷新定时器（200 ms）──
    m_simTimer.setInterval(200);
}

// ---------------------------------------------------------------
// 连接信号槽
// ---------------------------------------------------------------
void MainWindow::setupConnections()
{
    connect(m_reader,       &AisReader::dataUpdated,
            this,           &MainWindow::onAisUpdated);

    connect(m_boat,         &FishingBoat::stateChanged,
            this,           &MainWindow::onBoatStateChanged);

    connect(m_bridge,       &MapBridge::mapClicked,
            this,           &MainWindow::onMapClicked);

    connect(m_bridge,       &MapBridge::mapReady,
            this,           &MainWindow::onMapReady);

        connect(m_bridge,       &MapBridge::speechRequested,
            m_speechNotifier,&SpeechNotifier::speak);

    connect(m_mapView,      &QWebEngineView::loadFinished,
            this, [this](bool ok) { if (ok) onMapReady(); });

    connect(m_controlPanel, &ControlPanel::throttleChanged,
            m_boat,         &FishingBoat::setThrottle);

    connect(m_controlPanel, &ControlPanel::rudderChanged,
            m_boat,         &FishingBoat::setRudder);

        connect(m_controlPanel, &ControlPanel::aisSourceChanged,
            this,           &MainWindow::onAisSourceChanged);

    connect(m_aisTable,     &QTableView::clicked,
            this, [this](const QModelIndex& idx) {
                if (!idx.isValid()) return;
                const myAISData& t = m_aisModel->targets().at(idx.row());
                if (t.Lat != NO_VALID_LATLON && t.Lon != NO_VALID_LATLON)
                    panMapTo(t.Lat, t.Lon);
            });

    connect(&m_simTimer,    &QTimer::timeout,
            this,           &MainWindow::onSimTick);

    m_simTimer.start();
}

void MainWindow::onAisSourceChanged(const QString& mode, const QString& port, int baud)
{
    AisReader::Config cfg;
    cfg.updateMs = 1000;

    if (mode == QStringLiteral("serial")) {
        cfg.filePath.clear();
        cfg.serialPort = port.isEmpty() ? QStringLiteral("COM3") : port;
        cfg.baudRate = baud > 0 ? baud : 38400;
        m_statusLabel->setText(
            QString::fromUtf8("切换AIS源: 串口 %1 @ %2").arg(cfg.serialPort).arg(cfg.baudRate));
    } else {
        cfg.filePath.clear();
        cfg.serialPort.clear();
        cfg.baudRate = 38400;
        m_statusLabel->setText(QString::fromUtf8("切换AIS源: 离线文件回放"));
    }

    m_reader->start(cfg);
}

// ---------------------------------------------------------------
// AIS 数据更新
// ---------------------------------------------------------------
void MainWindow::onAisUpdated()
{
    const auto& targets = m_reader->targets();
    m_aisModel->setTargets(targets);

    if (m_mapLoaded) {
        pushMarkersToMap();
    }

    m_statusLabel->setText(
        QString::fromUtf8("[%1] AIS目标: %2 | 刷新: %3")
            .arg(m_reader->targets().isEmpty() ? "文件" : "在线")
            .arg(targets.size())
            .arg(QTime::currentTime().toString("hh:mm:ss")));
}

// ---------------------------------------------------------------
// 渔船状态变化
// ---------------------------------------------------------------
void MainWindow::onBoatStateChanged()
{
    m_boatLabel->setText(
        QString::fromUtf8("渔船  Lat:%1  Lon:%2  Hdg:%3°  Spd:%4kn  Rud:%5°")
            .arg(m_boat->lat(),         0, 'f', 5)
            .arg(m_boat->lon(),         0, 'f', 5)
            .arg(m_boat->heading(),     0, 'f', 1)
            .arg(m_boat->speedKnots(),  0, 'f', 1)
            .arg(m_boat->rudderDeg(),   0, 'f', 0));

    if (m_mapLoaded) {
        pushFishingBoatToMap();
        pushGuardRings();
    }
}

// ---------------------------------------------------------------
// 地图点击 → 放置渔船
// ---------------------------------------------------------------
void MainWindow::onMapClicked(double lat, double lon)
{
    m_boat->place(lat, lon);
    pushFishingBoatToMap();
    pushGuardRings();
    panMapTo(lat, lon);
}

// ---------------------------------------------------------------
// 地图加载完成
// ---------------------------------------------------------------
void MainWindow::onMapReady()
{
    if (m_mapLoaded) return;
    m_mapLoaded = true;
    pushMarkersToMap();
    if (m_boat->isPlaced()) {
        pushFishingBoatToMap();
        pushGuardRings();
    }
}

// ---------------------------------------------------------------
// 碰撞计算定时刷新（200 ms）
// ---------------------------------------------------------------
void MainWindow::onSimTick()
{
    if (!m_boat->isPlaced()) return;
    updateCollisionTable();
}

// ---------------------------------------------------------------
// 推送 AIS 目标到地图
// ---------------------------------------------------------------
void MainWindow::pushMarkersToMap()
{
    if (!m_mapLoaded) return;

    const QVector<myAISData> targets = m_reader->interpolatedTargets();

    QJsonArray arr;
    for (const myAISData& t : targets) {
        if (t.Lat == NO_VALID_LATLON || t.Lon == NO_VALID_LATLON) continue;

        QJsonObject obj;
        obj["mmsi"]   = t.MMSI;
        obj["lat"]    = t.Lat;
        obj["lon"]    = t.Lon;
        obj["name"]   = QString::fromLatin1(t.ShipName).trimmed();
        obj["sog"]    = t.SOG < 0  ? 0.0 : t.SOG;
        obj["cog"]    = t.COG < 0  ? 0.0 : t.COG;
        if (t.HDG >= 0.0 && t.HDG <= 360.0) {
            obj["hdg"] = t.HDG;
        } else {
            obj["hdg"] = QJsonValue::Null;
        }
        obj["status"] = t.NavStatus;
        arr.append(obj);
    }

    const QString js = QString("updateAisMarkers(%1);")
        .arg(QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    m_mapView->page()->runJavaScript(js);
}

// ---------------------------------------------------------------
// 推送渔船位置到地图
// ---------------------------------------------------------------
void MainWindow::pushFishingBoatToMap()
{
    if (!m_mapLoaded || !m_boat->isPlaced()) return;

    const QString js = QString("updateFishingBoat(%1,%2,%3,%4);")
        .arg(m_boat->lat(),        0, 'f', 7)
        .arg(m_boat->lon(),        0, 'f', 7)
        .arg(m_boat->heading(),    0, 'f', 2)
        .arg(m_boat->speedKnots(), 0, 'f', 2);
    m_mapView->page()->runJavaScript(js);
}

// ---------------------------------------------------------------
// 推送三层警戒圈
// ---------------------------------------------------------------
void MainWindow::pushGuardRings()
{
    if (!m_mapLoaded || !m_boat->isPlaced()) return;

    const QString js = QString("updateGuardRings(%1,%2);")
        .arg(m_boat->lat(), 0, 'f', 7)
        .arg(m_boat->lon(), 0, 'f', 7);
    m_mapView->page()->runJavaScript(js);
}

// ---------------------------------------------------------------
// 更新碰撞信息表
// ---------------------------------------------------------------
void MainWindow::updateCollisionTable()
{
    const QVector<myAISData> targets = m_reader->interpolatedTargets();

    QVector<CollisionRow> rows;
    rows.reserve(targets.size());

    for (const myAISData& t : targets) {
        if (t.Lat == NO_VALID_LATLON || t.Lon == NO_VALID_LATLON) continue;
        if (t.SOG < 0 || t.COG < 0) continue;

        const auto cpa = ColCalc::calculate(
            m_boat->lat(), m_boat->lon(), m_boat->heading(), m_boat->speedKnots(),
            t.Lat,         t.Lon,         t.COG,             t.SOG);

        // 只显示 1200m 以内的目标
        if (cpa.zone == 0) continue;

        CollisionRow row;
        row.mmsi       = t.MMSI;
        row.name       = QString::fromLatin1(t.ShipName).trimmed();
        row.lat        = t.Lat;
        row.lon        = t.Lon;
        row.distM      = cpa.distM;
        row.dcpaM      = cpa.dcpaM;
        row.tcpaMin    = cpa.tcpaMin;
        row.relBearDeg = cpa.relBearDeg;
        row.relCogDeg  = cpa.relCogDeg;
        row.relSogKn   = cpa.relSogKn;
        row.zone       = cpa.zone;
        row.isDanger   = cpa.isDanger;
        row.sector     = cpa.sector;
        rows.append(row);
    }

    // 按距离排序
    std::sort(rows.begin(), rows.end(),
              [](const CollisionRow& a, const CollisionRow& b) {
                  return a.distM < b.distM;
              });

    m_colModel->setRows(rows);

    // 同时发送碰撞警告到地图
    if (m_mapLoaded) {
        QJsonArray warn;
        QJsonArray motionTargets;
        for (const CollisionRow& r : rows) {
            QJsonObject w;
            w["mmsi"] = r.mmsi;
            w["zone"] = r.zone;
            w["isDanger"] = r.isDanger;
            w["sector"] = r.sector;
            w["distM"] = r.distM;
            w["dcpa"] = r.dcpaM;
            w["tcpa"] = r.tcpaMin;
            warn.append(w);

            QJsonObject m;
            m["mmsi"] = r.mmsi;
            m["lat"] = r.lat;
            m["lon"] = r.lon;
            m["relCog"] = r.relCogDeg;
            m["relSog"] = r.relSogKn;
            m["isDanger"] = r.isDanger;
            m["dcpa"] = r.dcpaM;
            m["tcpa"] = r.tcpaMin;
            m["distM"] = r.distM;
            m["relBearDeg"] = r.relBearDeg;
            m["zone"] = r.zone;
            m["sector"] = r.sector;
            motionTargets.append(m);
        }

        const QString warnJson = QString::fromUtf8(QJsonDocument(warn).toJson(QJsonDocument::Compact));
        m_mapView->page()->runJavaScript(QString("updateWarnings(%1);").arg(warnJson));

        QJsonObject motionPayload;
        motionPayload["ownLat"] = m_boat->lat();
        motionPayload["ownLon"] = m_boat->lon();
        motionPayload["targets"] = motionTargets;
        const QString motionJson = QString::fromUtf8(QJsonDocument(motionPayload).toJson(QJsonDocument::Compact));
        m_mapView->page()->runJavaScript(QString("updateMotionLines(%1);").arg(motionJson));
    }
}

// ---------------------------------------------------------------
// 地图定位
// ---------------------------------------------------------------
void MainWindow::panMapTo(double lat, double lon)
{
    if (!m_mapLoaded) return;
    m_mapView->page()->runJavaScript(
        QString("map.setView([%1,%2],14);").arg(lat, 0, 'f', 7).arg(lon, 0, 'f', 7));
}

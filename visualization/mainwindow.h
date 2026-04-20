#pragma once
/**
 * @file  mainwindow.h
 * @brief 主窗口：左侧地图（Leaflet.js via QWebEngineView）+ 右侧 AIS 目标列表
 *
 *  ┌──────────────────────────┬──────────────────────┐
 *  │   QWebEngineView         │  QTableView          │
 *  │   Leaflet.js 地图         │  AIS 目标列表         │
 *  │   • AIS 目标图标          │  • MMSI/船名/速度…    │
 *  │   • 选中高亮              │  • 点击行 → 地图定位   │
 *  └──────────────────────────┴──────────────────────┘
 *                   底部状态栏：目标数量 / 更新时间
 */

#include <QMainWindow>
#include <QTimer>
#include <QWebEngineView>
#include <QTableView>
#include <QSplitter>
#include <QStatusBar>
#include <QLabel>

#include "aislistmodel.h"
#include "aisreader.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // AIS 数据更新后刷新地图和列表
    void onAisUpdated();
    // 列表中点击行 → 地图中心移动到该目标
    void onTableRowClicked(const QModelIndex& index);
    // 地图加载完毕后注入初始数据
    void onMapReady(bool ok);

private:
    void setupUi();
    void setupConnections();

    // 将 targets 转换为 JSON 并通过 JavaScript 推送到 Leaflet 地图
    void pushMarkersToMap(const QVector<myAISData>& targets);
    // 移动地图中心到指定经纬度
    void panMapTo(double lat, double lon);

    QWebEngineView* m_mapView   {nullptr};
    QTableView*     m_tableView {nullptr};
    AisListModel*   m_model     {nullptr};
    AisReader*      m_reader    {nullptr};
    QLabel*         m_statusLabel {nullptr};

    bool m_mapLoaded {false};
};

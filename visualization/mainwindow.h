#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QTableView>
#include <QTimer>
#include <QWebChannel>
#include <QWebEngineView>

#include "aislistmodel.h"
#include "aisreader.h"
#include "collisiontablemodel.h"
#include "controlpanel.h"
#include "fishingboat.h"
#include "mapbridge.h"
#include "speechnotifier.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onAisUpdated();
    void onBoatStateChanged();
    void onMapClicked(double lat, double lon);
    void onMapReady();
    void onSimTick();
    void onAisSourceChanged(const QString& mode, const QString& port, int baud);

private:
    void setupUi();
    void setupConnections();
    void pushMarkersToMap();
    void pushFishingBoatToMap();
    void pushGuardRings();
    void updateCollisionTable();
    void panMapTo(double lat, double lon);

    // --- Widgets ---
    QWebEngineView*      m_mapView        {nullptr};
    QTableView*          m_aisTable       {nullptr};
    QTableView*          m_colTable       {nullptr};
    AisListModel*        m_aisModel       {nullptr};
    CollisionTableModel* m_colModel       {nullptr};
    ControlPanel*        m_controlPanel   {nullptr};
    QLabel*              m_statusLabel    {nullptr};
    QLabel*              m_boatLabel      {nullptr};

    // --- Logic ---
    AisReader*           m_reader         {nullptr};
    FishingBoat*         m_boat           {nullptr};
    QWebChannel*         m_channel        {nullptr};
    MapBridge*           m_bridge         {nullptr};
    SpeechNotifier*      m_speechNotifier {nullptr};

    // --- State ---
    bool                 m_mapLoaded      {false};
    QTimer               m_simTimer;   // 碰撞计算刷新（200 ms）
};

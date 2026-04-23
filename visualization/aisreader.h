#pragma once

#include <QDateTime>
#include <QFile>
#include <QMap>
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QVector>

#include "ais/myaisdecoder.h"
#include "serialportreader.h"

// ============================================================
//  AIS 记录（含时间戳，用于线性插值）
// ============================================================
struct AisRecord {
    myAISData data;
    qint64    timestampMs {0};
};

// ============================================================
//  配置
// ============================================================
struct AisReaderConfig {
    QString filePath;
    QString serialPort;
    int     baudRate  {38400};
    int     updateMs  {500};
};

// ============================================================
//  AisReader
// ============================================================
class AisReader : public QObject {
    Q_OBJECT

public:
    using Config = AisReaderConfig;

    explicit AisReader(QObject* parent = nullptr, Config cfg = Config());

    void start(const Config& cfg);
    void stop();

    const QVector<myAISData>&   targets()            const;
    QVector<myAISData>          interpolatedTargets(qint64 nowMs = 0) const;

signals:
    void dataUpdated();
    void errorOccurred(const QString& error);

private slots:
    void onTimer();
    void onSerialDataReceived(const QString& sentence);

private:
    void processLine(const QString& line);

    Config                      m_cfg;
    QTimer                      m_timer;
    QFile                       m_file;
    QTextStream                 m_stream;
    myAISDecoder                m_decoder;
    SerialPortReader*           m_serialReader{nullptr};

    QMap<int, AisRecord>        m_prev;   // 上一条有效记录
    QMap<int, AisRecord>        m_curr;   // 最新一条有效记录
    QVector<myAISData>          m_targetList;

    // 文件模式模拟时钟（1秒/tick，与渔船仿真同步）
    qint64 m_simClockMs {0};
    bool   m_simStarted {false};
};

#pragma once
/**
 * @file  aisreader.h
 * @brief AIS 数据源抽象层
 *
 * 支持两种数据源（通过构造参数选择）：
 *   1. 文件模式（FilePath 非空）：逐行读取 NMEA 文件并模拟实时播放
 *   2. 串口模式（SerialPort 非空）：接收实际 AIS 接收机的 NMEA 数据流
 *
 * 每当目标表更新时，发出 dataUpdated() 信号。
 */

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QMap>

#include "ais/myaisdecoder.h"

// Config 放在类外，避免 GCC 11 对内嵌类默认参数的限制
struct AisReaderConfig {
    QString filePath;       // 文件回放路径（为空则使用串口）
    QString serialPort;     // 串口设备，如 "/dev/ttyUSB0"
    int     baudRate   = 38400;
    int     updateMs   = 1000;  // 数据刷新周期（毫秒）
};

class AisReader : public QObject
{
    Q_OBJECT

public:
    using Config = AisReaderConfig;

    explicit AisReader(QObject* parent = nullptr,
                       Config    cfg   = Config());

    void start(const Config& cfg);
    void stop();

    const QVector<myAISData>& targets() const { return m_targetList; }

signals:
    void dataUpdated();

private slots:
    void onTimer();

private:
    void processLine(const QString& line);

    Config          m_cfg;
    QTimer          m_timer;
    QFile           m_file;
    QTextStream     m_stream;
    myAISDecoder    m_decoder;

    // MMSI → AIS 数据，key 保持唯一
    QMap<int, myAISData> m_targetMap;
    QVector<myAISData>   m_targetList;
};

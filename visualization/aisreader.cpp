#include "aisreader.h"
#include <QDebug>

// --------------------------------------------------------------------------
AisReader::AisReader(QObject* parent, Config cfg)
    : QObject(parent)
    , m_cfg(std::move(cfg))
{
    connect(&m_timer, &QTimer::timeout, this, &AisReader::onTimer);

    // 默认用 data/ 目录下的测试文件
    if (m_cfg.filePath.isEmpty() && m_cfg.serialPort.isEmpty())
        m_cfg.filePath = "data/AISMSG.txt";

    start(m_cfg);
}

// --------------------------------------------------------------------------
void AisReader::start(const Config& cfg)
{
    m_cfg = cfg;
    m_timer.stop();

    if (!m_cfg.filePath.isEmpty())
    {
        m_file.setFileName(m_cfg.filePath);
        if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qWarning() << "[AisReader] 无法打开文件:" << m_cfg.filePath;
            return;
        }
        m_stream.setDevice(&m_file);
    }
    // TODO: 串口模式后续扩展（QSerialPort）

    m_timer.start(m_cfg.updateMs);
}

// --------------------------------------------------------------------------
void AisReader::stop()
{
    m_timer.stop();
    if (m_file.isOpen())
        m_file.close();
}

// --------------------------------------------------------------------------
// 每个定时周期读取若干行并解码
// --------------------------------------------------------------------------
void AisReader::onTimer()
{
    if (m_cfg.filePath.isEmpty()) return;

    // 文件末尾则重置（循环回放）
    if (m_stream.atEnd())
    {
        m_stream.seek(0);
        m_targetMap.clear();
    }

    // 每个定时器周期读取最多 20 行
    int linesRead = 0;
    while (!m_stream.atEnd() && linesRead < 20)
    {
        QString line = m_stream.readLine().trimmed();
        if (!line.isEmpty())
            processLine(line);
        ++linesRead;
    }

    // 更新列表并通知 UI
    m_targetList = m_targetMap.values().toVector();
    emit dataUpdated();
}

// --------------------------------------------------------------------------
void AisReader::processLine(const QString& line)
{
    myAISData data;
    auto err = m_decoder.Decode(line.toLatin1().constData(), data);
    if (err == AIS_NOERROR || err == AIS_PARTIAL)
    {
        if (data.MMSI != NO_VALID_MMSI)
            m_targetMap[data.MMSI] = data;
    }
}

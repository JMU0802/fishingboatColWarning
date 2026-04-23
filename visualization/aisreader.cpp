#include "aisreader.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

const QVector<myAISData>& AisReader::targets() const
{
    return m_targetList;
}

AisReader::AisReader(QObject* parent, Config cfg)
    : QObject(parent), m_cfg(std::move(cfg))
{
    connect(&m_timer, &QTimer::timeout, this, &AisReader::onTimer);

    if (m_cfg.filePath.isEmpty()) {
        const QString appDir  = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            appDir + "/data/AISMSG.txt",
            appDir + "/../data/AISMSG.txt",
            appDir + "/../../data/AISMSG.txt",
            QDir::currentPath() + "/data/AISMSG.txt"
        };
        for (const QString& p : candidates) {
            if (QFileInfo::exists(p)) {
                m_cfg.filePath = p;
                qDebug() << "[AisReader] auto-detected file:" << p;
                break;
            }
        }
    }

    start(m_cfg);
}

void AisReader::start(const Config& cfg)
{
    m_cfg = cfg;
    m_timer.stop();
    m_simClockMs = 0;
    m_simStarted = false;

    if (m_cfg.filePath.isEmpty() && m_cfg.serialPort.isEmpty()) {
        const QString appDir  = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            appDir + "/data/AISMSG.txt",
            appDir + "/../data/AISMSG.txt",
            appDir + "/../../data/AISMSG.txt",
            QDir::currentPath() + "/data/AISMSG.txt"
        };
        for (const QString& p : candidates) {
            if (QFileInfo::exists(p)) {
                m_cfg.filePath = p;
                qDebug() << "[AisReader] auto-detected file:" << p;
                break;
            }
        }
    }

    if (m_serialReader) {
        m_serialReader->close();
        m_serialReader->deleteLater();
        m_serialReader = nullptr;
    }

    if (!m_cfg.filePath.isEmpty()) {
        m_file.setFileName(m_cfg.filePath);
        if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString err = QString("Failed to open file: %1").arg(m_cfg.filePath);
            qWarning() << "[AisReader]" << err;
            emit errorOccurred(err);
        } else {
            m_stream.setDevice(&m_file);
            m_timer.start(m_cfg.updateMs);
            qDebug() << "[AisReader] file mode started:" << m_cfg.filePath;
        }
        return;
    }

    if (!m_cfg.serialPort.isEmpty()) {
        m_serialReader = new SerialPortReader(this);
        connect(m_serialReader, &SerialPortReader::dataReceived,
                this, &AisReader::onSerialDataReceived);
        connect(m_serialReader, &SerialPortReader::errorOccurred,
                this, &AisReader::errorOccurred);

        SerialPortReader::Config sc;
        sc.portName = m_cfg.serialPort;
        sc.baudRate = m_cfg.baudRate;
        sc.dataBits = 8;
        sc.stopBits = 1;
        sc.parity   = "none";

        if (!m_serialReader->open(sc)) {
            const QString err = QString("Failed to open serial %1: %2")
                .arg(m_cfg.serialPort, m_serialReader->errorString());
            qWarning() << "[AisReader]" << err;
            emit errorOccurred(err);
        } else {
            qDebug() << "[AisReader] serial started:" << m_cfg.serialPort;
        }
    }
}

void AisReader::stop()
{
    m_timer.stop();
    if (m_file.isOpen()) m_file.close();
    if (m_serialReader)  m_serialReader->close();
}

void AisReader::onTimer()
{
    if (m_stream.atEnd()) {
        m_stream.seek(0);
    }

    // 文件模式：维护模拟时钟（1 秒/tick），保证与渔船仿真时间同步
    if (m_file.isOpen()) {
        if (!m_simStarted) {
            m_simClockMs = QDateTime::currentMSecsSinceEpoch();
            m_simStarted = true;
        } else {
            m_simClockMs += 1000;
        }
    }

    int linesRead = 0;
    while (!m_stream.atEnd() && linesRead < 20) {
        const QString line = m_stream.readLine().trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }
        ++linesRead;
    }

    m_targetList.clear();
    for (const AisRecord& r : m_curr.values()) {
        m_targetList.append(r.data);
    }
    emit dataUpdated();
}

void AisReader::onSerialDataReceived(const QString& sentence)
{
    if (sentence.isEmpty()) return;
    processLine(sentence);
    m_targetList.clear();
    for (const AisRecord& r : m_curr.values()) {
        m_targetList.append(r.data);
    }
    emit dataUpdated();
}

void AisReader::processLine(const QString& line)
{
    myAISData data;
    const auto err = m_decoder.Decode(line.toLatin1().constData(), data);
    if ((err == AIS_NOERROR || err == AIS_PARTIAL)
        && data.MMSI != NO_VALID_MMSI
        && data.Lat  != NO_VALID_LATLON
        && data.Lon  != NO_VALID_LATLON)
    {
        AisRecord rec;
        rec.data        = data;
        rec.timestampMs = m_file.isOpen() ? m_simClockMs : QDateTime::currentMSecsSinceEpoch();

        if (m_curr.contains(data.MMSI)) {
            m_prev[data.MMSI] = m_curr[data.MMSI];
        }
        m_curr[data.MMSI] = rec;
    }
}

QVector<myAISData> AisReader::interpolatedTargets(qint64 nowMs) const
{
    if (nowMs == 0) {
        nowMs = m_simStarted ? m_simClockMs : QDateTime::currentMSecsSinceEpoch();
    }

    QVector<myAISData> result;
    result.reserve(m_curr.size());

    for (auto it = m_curr.cbegin(); it != m_curr.cend(); ++it) {
        const int mmsi      = it.key();
        const AisRecord& cur = it.value();

        if (!m_prev.contains(mmsi)) {
            result.append(cur.data);
            continue;
        }

        const AisRecord& prv = m_prev[mmsi];
        const qint64 dt = cur.timestampMs - prv.timestampMs;
        if (dt <= 0) {
            result.append(cur.data);
            continue;
        }

        const double alpha = static_cast<double>(nowMs - prv.timestampMs) / static_cast<double>(dt);
        const double a     = std::min(std::max(alpha, 0.0), 2.0);

        myAISData interp   = cur.data;
        interp.Lat = prv.data.Lat + a * (cur.data.Lat - prv.data.Lat);
        interp.Lon = prv.data.Lon + a * (cur.data.Lon - prv.data.Lon);
        interp.SOG = prv.data.SOG + a * (cur.data.SOG - prv.data.SOG);

        double dCog = cur.data.COG - prv.data.COG;
        if (dCog >  180.0) dCog -= 360.0;
        if (dCog < -180.0) dCog += 360.0;
        interp.COG = std::fmod(prv.data.COG + a * dCog + 360.0, 360.0);

        result.append(interp);
    }

    return result;
}

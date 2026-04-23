#include "controlpanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QIntValidator>
#include <algorithm>
#include <cmath>

// Slider integer ranges (0.1 kn resolution for throttle)
// throttle slider: -60..+80  -> -6.0..+8.0 kn
// rudder  slider: -35..+35  -> -35..+35 deg
static const int TSLIDER_MIN = -60;
static const int TSLIDER_MAX =  80;
static const int RSLIDER_MIN = -35;
static const int RSLIDER_MAX =  35;

ControlPanel::ControlPanel(QWidget* parent)
    : QGroupBox(parent)
{
    setTitle(QString::fromUtf8("\xe6\xb8\x94\xe8\x88\xb9\xe8\xbf\x90\xe5\x8a\xa8\xe6\x8e\xa7\xe5\x88\xb6"));

    m_throttleLabel  = new QLabel("0.0 kn", this);
    m_throttleSlider = new QSlider(Qt::Horizontal, this);
    m_throttleSlider->setRange(TSLIDER_MIN, TSLIDER_MAX);
    m_throttleSlider->setValue(0);
    m_throttleSlider->setTickInterval(10);
    m_throttleSlider->setTickPosition(QSlider::TicksBelow);

    m_rudderLabel  = new QLabel("0 deg", this);
    m_rudderSlider = new QSlider(Qt::Horizontal, this);
    m_rudderSlider->setRange(RSLIDER_MIN, RSLIDER_MAX);
    m_rudderSlider->setValue(0);
    m_rudderSlider->setTickInterval(5);
    m_rudderSlider->setTickPosition(QSlider::TicksBelow);

    // Labels: use UTF-8 bytes for Chinese
    //   "车令" = \xe8\xbd\xa6\xe4\xbb\xa4
    //   "舵角" = \xe8\x88\xb5\xe8\xa7\x92
    auto* tLabel = new QLabel(this);
    tLabel->setText(QString::fromUtf8("\xe8\xbd\xa6\xe4\xbb\xa4"));
    auto* rLabel = new QLabel(this);
    rLabel->setText(QString::fromUtf8("\xe8\x88\xb5\xe8\xa7\x92"));

    QHBoxLayout* tRow = new QHBoxLayout;
    tRow->addWidget(tLabel);
    tRow->addWidget(m_throttleSlider, 1);
    tRow->addWidget(m_throttleLabel);

    QHBoxLayout* rRow = new QHBoxLayout;
    rRow->addWidget(rLabel);
    rRow->addWidget(m_rudderSlider, 1);
    rRow->addWidget(m_rudderLabel);

    // AIS 数据源设置
    auto* srcLabel = new QLabel(this);
    srcLabel->setText(QString::fromUtf8("AIS源"));
    m_sourceModeCombo = new QComboBox(this);
    m_sourceModeCombo->addItem("File", "file");
    m_sourceModeCombo->addItem("Serial", "serial");

    auto* portLabel = new QLabel("Port", this);
    m_serialPortEdit = new QLineEdit("COM3", this);
    m_serialPortEdit->setMaximumWidth(90);

    auto* baudLabel = new QLabel("Baud", this);
    m_baudRateEdit = new QLineEdit("38400", this);
    m_baudRateEdit->setValidator(new QIntValidator(1200, 921600, this));
    m_baudRateEdit->setMaximumWidth(110);

    m_applySourceBtn = new QPushButton(QString::fromUtf8("应用AIS源"), this);

    QHBoxLayout* srcRow = new QHBoxLayout;
    srcRow->addWidget(srcLabel);
    srcRow->addWidget(m_sourceModeCombo);
    srcRow->addWidget(portLabel);
    srcRow->addWidget(m_serialPortEdit);
    srcRow->addWidget(baudLabel);
    srcRow->addWidget(m_baudRateEdit);
    srcRow->addWidget(m_applySourceBtn);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(tRow);
    layout->addLayout(rRow);
    layout->addLayout(srcRow);
    setLayout(layout);

    connect(m_throttleSlider, &QSlider::valueChanged, this, [this](int val) {
        const double kn = val / 10.0;
        m_throttleLabel->setText(QString::number(kn, 'f', 1) + " kn");
        emit throttleChanged(kn);
    });

    connect(m_rudderSlider, &QSlider::valueChanged, this, [this](int val) {
        const double deg = static_cast<double>(val);
        // "L" / "R" / "C" direction
        const char* side = (deg < 0) ? "L" : (deg > 0 ? "R" : "C");
        m_rudderLabel->setText(
            QString("%1 %2deg").arg(side).arg(static_cast<int>(std::abs(deg))));
        emit rudderChanged(deg);
    });

    connect(m_applySourceBtn, &QPushButton::clicked, this, [this]() {
        const QString mode = m_sourceModeCombo->currentData().toString();
        const QString port = m_serialPortEdit->text().trimmed();
        const int baud = m_baudRateEdit->text().toInt();
        emit aisSourceChanged(mode, port.isEmpty() ? QStringLiteral("COM3") : port,
                              baud > 0 ? baud : 38400);
    });
}

double ControlPanel::throttleKnots() const
{
    return m_throttleSlider->value() / 10.0;
}

double ControlPanel::rudderDeg() const
{
    return static_cast<double>(m_rudderSlider->value());
}

void ControlPanel::setThrottle(double knots)
{
    m_throttleSlider->setValue(static_cast<int>(knots * 10.0));
}

void ControlPanel::setRudder(double deg)
{
    m_rudderSlider->setValue(static_cast<int>(deg));
}

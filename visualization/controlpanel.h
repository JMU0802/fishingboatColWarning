#pragma once

#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>

// ============================================================
//  ControlPanel - throttle / rudder control widget
//  throttle: -6.0 (rev) ~ +8.0 (fwd) knots
//  rudder:   -35.0 (port) ~ +35.0 (stbd) degrees
// ============================================================
class ControlPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit ControlPanel(QWidget* parent = nullptr);

    double throttleKnots() const;
    double rudderDeg()     const;

    void setThrottle(double knots);
    void setRudder  (double deg);

signals:
    void throttleChanged(double knots);
    void rudderChanged  (double deg);
    void aisSourceChanged(const QString& mode, const QString& port, int baud);

private:
    QSlider* m_throttleSlider {nullptr};
    QSlider* m_rudderSlider   {nullptr};
    QLabel*  m_throttleLabel  {nullptr};
    QLabel*  m_rudderLabel    {nullptr};
    QComboBox* m_sourceModeCombo {nullptr};
    QLineEdit* m_serialPortEdit  {nullptr};
    QLineEdit* m_baudRateEdit    {nullptr};
    QPushButton* m_applySourceBtn {nullptr};
};

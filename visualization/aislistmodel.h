#pragma once
/**
 * @file  aislistmodel.h
 * @brief Qt 模型：将 AIS 目标列表绑定到 QTableView
 */

#include <QAbstractTableModel>
#include <QVector>
#include <QTime>
#include "ais/myaisdecoder.h"

class AisListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // 列定义
    enum Column {
        COL_MMSI = 0,
        COL_NAME,
        COL_TYPE,
        COL_SOG,
        COL_COG,
        COL_LAT,
        COL_LON,
        COL_NAVSTATUS,
        COL_COUNT
    };

    explicit AisListModel(QObject* parent = nullptr);

    // QAbstractTableModel 接口
    int      rowCount   (const QModelIndex& = {}) const override;
    int      columnCount(const QModelIndex& = {}) const override;
    QVariant data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 数据更新
    void setTargets(const QVector<myAISData>& targets);
    const QVector<myAISData>& targets() const { return m_targets; }

private:
    QVector<myAISData> m_targets;
    myAISDecoder       m_decoder; // 用于 GetVesselType
};

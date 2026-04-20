#include "aislistmodel.h"

static const char* NAV_STATUS_STR[] = {
    "航行中(主机)", "锚泊", "失控", "操纵受限",
    "吃水受限",    "系泊", "搁浅", "捕鱼",
    "帆船航行",    "高速船","WIG", "保留11",
    "保留12",      "保留13","保留14","未定义"
};

// --------------------------------------------------------------------------
AisListModel::AisListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

// --------------------------------------------------------------------------
int AisListModel::rowCount(const QModelIndex&) const
{
    return m_targets.size();
}

int AisListModel::columnCount(const QModelIndex&) const
{
    return COL_COUNT;
}

// --------------------------------------------------------------------------
QVariant AisListModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section) {
    case COL_MMSI:      return "MMSI";
    case COL_NAME:      return "船名";
    case COL_TYPE:      return "船型";
    case COL_SOG:       return "SOG(kn)";
    case COL_COG:       return "COG(°)";
    case COL_LAT:       return "纬度";
    case COL_LON:       return "经度";
    case COL_NAVSTATUS: return "航行状态";
    default:            return {};
    }
}

// --------------------------------------------------------------------------
QVariant AisListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const myAISData& t = m_targets.at(index.row());

    switch (index.column()) {
    case COL_MMSI:
        return t.MMSI;
    case COL_NAME:
        return QString::fromLatin1(t.ShipName).trimmed();
    case COL_TYPE: {
        // GetVesselType 是非 const，通过 const_cast 调用（不修改状态）
        char* tp = const_cast<myAISDecoder&>(m_decoder)
                       .GetVesselType(true, t);
        return tp ? QString::fromLatin1(tp) : QString();
    }
    case COL_SOG:
        return t.SOG < 0 ? QVariant("--") : QVariant(QString::number(t.SOG, 'f', 1));
    case COL_COG:
        return t.COG < 0 ? QVariant("--") : QVariant(QString::number(t.COG, 'f', 1));
    case COL_LAT:
        return t.Lat == NO_VALID_LATLON ? QVariant("--") : QVariant(QString::number(t.Lat, 'f', 5));
    case COL_LON:
        return t.Lon == NO_VALID_LATLON ? QVariant("--") : QVariant(QString::number(t.Lon, 'f', 5));
    case COL_NAVSTATUS: {
        int ns = t.NavStatus;
        if (ns >= 0 && ns < 16)
            return QString::fromUtf8(NAV_STATUS_STR[ns]);
        return "未定义";
    }
    default:
        return {};
    }
}

// --------------------------------------------------------------------------
void AisListModel::setTargets(const QVector<myAISData>& targets)
{
    beginResetModel();
    m_targets = targets;
    endResetModel();
}

#include "colwarning.h"
#include <cmath>
#include <limits>

// --------------------------------------------------------------------------
ColWarning::ColWarning(double cpa_limit_nm, double tcpa_limit_min)
    : m_cpa_limit(cpa_limit_nm)
    , m_tcpa_limit(tcpa_limit_min)
{
}

// --------------------------------------------------------------------------
// 经纬度转换为以 origin 为原点的局部平面坐标 (单位: km)
// 使用等距柱投影（适用于小区域，海上避碰场景精度足够）
// --------------------------------------------------------------------------
ColWarning::Vec2 ColWarning::LatLonToXY(double lat, double lon,
                                         double origin_lat, double origin_lon)
{
    double dlat = (lat - origin_lat) * DEG2RAD;
    double dlon = (lon - origin_lon) * DEG2RAD;
    double cos_lat = std::cos(origin_lat * DEG2RAD);

    Vec2 v;
    v.x = EARTH_RADIUS_KM * dlon * cos_lat;  // East  (km)
    v.y = EARTH_RADIUS_KM * dlat;             // North (km)
    return v;
}

// --------------------------------------------------------------------------
// SOG (kn) + COG (deg, 真北顺时针) → 速度向量 (km/h)
// --------------------------------------------------------------------------
ColWarning::Vec2 ColWarning::SogCogToVelocity(double sog_kn, double cog_deg)
{
    double speed_kmh = sog_kn * KN_TO_KMH;
    double cog_rad   = cog_deg * DEG2RAD;
    Vec2 v;
    v.x = speed_kmh * std::sin(cog_rad);   // East
    v.y = speed_kmh * std::cos(cog_rad);   // North
    return v;
}

// --------------------------------------------------------------------------
// CPA / TCPA 核心计算
// 参考：向量法（相对运动线 RML）
// --------------------------------------------------------------------------
ColResult ColWarning::Compute(const myAISData& own,
                               const myAISData& target) const
{
    ColResult result{-1.0, -1.0, false};

    // 基本有效性校验
    if (own.Lat    == NO_VALID_LATLON || own.Lon    == NO_VALID_LATLON ||
        target.Lat == NO_VALID_LATLON || target.Lon == NO_VALID_LATLON ||
        own.SOG    == NO_VALID_SOG   || own.COG    == NO_VALID_COG   ||
        target.SOG == NO_VALID_SOG   || target.COG == NO_VALID_COG)
    {
        return result;
    }

    // 以本船位置为原点建立局部坐标系
    Vec2 pos_own    = {0.0, 0.0};
    Vec2 pos_target = LatLonToXY(target.Lat, target.Lon, own.Lat, own.Lon);

    Vec2 vel_own    = SogCogToVelocity(own.SOG,    own.COG);
    Vec2 vel_target = SogCogToVelocity(target.SOG, target.COG);

    // 相对位置向量 (目标相对本船)
    Vec2 dr = {pos_target.x - pos_own.x,
               pos_target.y - pos_own.y};

    // 相对速度向量 (目标相对本船)
    Vec2 dv = {vel_target.x - vel_own.x,
               vel_target.y - vel_own.y};

    // TCPA = - (dr · dv) / |dv|²  (小时)
    double dv2 = dv.x * dv.x + dv.y * dv.y;
    double tcpa_h = 0.0;

    if (dv2 < 1e-9)
    {
        // 相对速度近似为零，保持当前距离
        tcpa_h = 0.0;
    }
    else
    {
        tcpa_h = -(dr.x * dv.x + dr.y * dv.y) / dv2;
    }

    // CPA 位置 (km)
    Vec2 cpa_pos = {dr.x + tcpa_h * dv.x,
                    dr.y + tcpa_h * dv.y};

    double cpa_km = std::sqrt(cpa_pos.x * cpa_pos.x + cpa_pos.y * cpa_pos.y);
    double cpa_nm = cpa_km * NM_PER_KM;

    result.cpa_nm   = cpa_nm;
    result.tcpa_min = tcpa_h * 60.0;

    // 告警条件：CPA 小于门限 且 TCPA 在 (0, tcpa_limit] 之间
    result.is_warning = (cpa_nm   < m_cpa_limit) &&
                        (result.tcpa_min > 0.0)  &&
                        (result.tcpa_min <= m_tcpa_limit);

    return result;
}

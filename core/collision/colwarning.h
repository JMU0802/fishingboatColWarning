#pragma once
/**
 * @file  colwarning.h
 * @brief 碰撞避让算法模块 (CPA/TCPA)
 *
 * 纯计算模块，无 UI 依赖，可在 PC 端和树莓派端共用。
 *
 * 算法核心：
 *   - CPA  (Closest Point of Approach)  最近会遇距离，单位：海里
 *   - TCPA (Time to CPA)                到达最近会遇点的时间，单位：分钟
 *   - 当 CPA < cpa_limit 且 0 < TCPA < tcpa_limit 时触发碰撞警报
 */

#include <cmath>
#include "ais/myaisdecoder.h"

// 地球平均半径 (km)
static constexpr double EARTH_RADIUS_KM = 6371.0;
static constexpr double NM_PER_KM       = 0.539957;
static constexpr double DEG2RAD         = M_PI / 180.0;
static constexpr double KN_TO_KMH       = 1.852;

// 碰撞告警阈值（可根据实际场景调整）
static constexpr double DEFAULT_CPA_LIMIT_NM   = 0.5;   // 最近会遇距离门限 (NM)
static constexpr double DEFAULT_TCPA_LIMIT_MIN  = 6.0;   // 到达 CPA 时间门限 (分钟)

// --------------------------------------------------------------------------
// 碰撞计算结果
// --------------------------------------------------------------------------
struct ColResult
{
    double cpa_nm;      // 最近会遇距离 (NM)，< 0 表示无效
    double tcpa_min;    // 到达 CPA 的时间 (分钟)，< 0 表示已经过
    bool   is_warning;  // 是否触发碰撞警报
};

// --------------------------------------------------------------------------
// 碰撞避让计算器
// --------------------------------------------------------------------------
class ColWarning
{
public:
    /**
     * @param cpa_limit_nm   CPA 告警门限 (海里)
     * @param tcpa_limit_min TCPA 告警门限 (分钟)
     */
    explicit ColWarning(double cpa_limit_nm   = DEFAULT_CPA_LIMIT_NM,
                        double tcpa_limit_min = DEFAULT_TCPA_LIMIT_MIN);

    /**
     * @brief 计算本船与目标船之间的 CPA / TCPA
     *
     * @param own    本船 AIS 数据（需要有效的 Lat/Lon/SOG/COG）
     * @param target 目标船 AIS 数据
     * @return ColResult  计算结果，is_warning=true 时需要告警
     */
    ColResult Compute(const myAISData& own, const myAISData& target) const;

    void SetCpaLimit(double nm)   { m_cpa_limit   = nm;  }
    void SetTcpaLimit(double min) { m_tcpa_limit   = min; }

private:
    double m_cpa_limit;
    double m_tcpa_limit;

    // 经纬度 → 平面直角坐标 (km)，以 origin 为原点
    struct Vec2 { double x, y; };
    static Vec2  LatLonToXY(double lat, double lon,
                             double origin_lat, double origin_lon);
    static Vec2  SogCogToVelocity(double sog_kn, double cog_deg);
};

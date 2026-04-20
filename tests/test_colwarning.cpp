/**
 * @file  test_colwarning.cpp
 * @brief 碰撞避让算法 CPA/TCPA 单元测试
 */

#include <doctest/doctest.h>
#include "core/collision/colwarning.h"

// 构造一个有效 AIS 数据（位置 + 速度）
static myAISData makeShip(int mmsi, double lat, double lon,
                           double sog, double cog)
{
    myAISData d{};
    d.MMSI = mmsi;
    d.Lat  = lat;
    d.Lon  = lon;
    d.SOG  = sog;
    d.COG  = cog;
    d.NavStatus = UNDEFINED;
    return d;
}

// --------------------------------------------------------------------------
TEST_SUITE("ColWarning")
{
    // 两船正面相遇，应触发告警
    TEST_CASE("Head-on collision triggers warning")
    {
        ColWarning cw(0.5, 6.0);

        // 本船：纬度 30.0°N，经度 122.0°E，向北 (COG=0) 以 10 kn 行驶
        auto own    = makeShip(123456789, 30.0, 122.0, 10.0, 0.0);
        // 目标：在本船正前方约 2 NM 处，向南 (COG=180) 以 10 kn 行驶
        // 2 NM ≈ 0.0333° 纬度
        auto target = makeShip(987654321, 30.0333, 122.0, 10.0, 180.0);

        ColResult res = cw.Compute(own, target);

        CHECK(res.cpa_nm   >= 0.0);
        CHECK(res.tcpa_min >= 0.0);
        CHECK(res.is_warning == true);
    }

    // 两船平行行驶，不应触发告警
    TEST_CASE("Parallel ships no warning")
    {
        ColWarning cw(0.5, 6.0);

        auto own    = makeShip(111, 30.0, 122.0, 10.0, 0.0);   // 向北
        auto target = makeShip(222, 30.0, 122.1, 10.0, 0.0);   // 同向北，旁侧 ~5 NM

        ColResult res = cw.Compute(own, target);

        // CPA 约等于横向距离（~5 NM），不触发告警
        CHECK(res.cpa_nm > 0.5);
        CHECK(res.is_warning == false);
    }

    // TCPA 为负（已经过最近点）不应告警
    TEST_CASE("Past CPA no warning")
    {
        ColWarning cw(0.5, 6.0);

        // 目标在本船后方并向后驶离
        auto own    = makeShip(111, 30.0, 122.0, 10.0, 0.0);
        auto target = makeShip(222, 29.98, 122.0, 10.0, 180.0);  // 向南

        ColResult res = cw.Compute(own, target);
        CHECK(res.is_warning == false);
    }

    // 无效数据（坐标缺失）返回无效结果
    TEST_CASE("Invalid data returns no result")
    {
        ColWarning cw;
        myAISData own{};
        own.Lat = NO_VALID_LATLON;
        own.Lon = NO_VALID_LATLON;
        own.SOG = NO_VALID_SOG;
        own.COG = NO_VALID_COG;

        myAISData target = makeShip(222, 30.0, 122.0, 5.0, 90.0);

        ColResult res = cw.Compute(own, target);
        CHECK(res.cpa_nm < 0.0);     // 返回无效标志
        CHECK(res.is_warning == false);
    }
}

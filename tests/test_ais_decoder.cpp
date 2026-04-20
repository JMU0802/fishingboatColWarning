/**
 * @file  test_ais_decoder.cpp
 * @brief AIS 解码器单元测试
 *
 * 测试数据来自 data/AISMSG.txt
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "core/ais/myaisdecoder.h"

// --------------------------------------------------------------------------
TEST_SUITE("AISDecoder")
{
    // 单句 Class A 位置报告（Message Type 1/2/3）
    TEST_CASE("Decode single-sentence type1 message")
    {
        myAISDecoder decoder;
        myAISData    data{};

        // 来自 AISMSG.txt 的真实样本
        const char* sentence = "!AIVDM,1,1,,A,36:=nt5000`LFjL=wWd:e5c@0000,0*47";
        auto err = decoder.Decode(sentence, data);

        CHECK(err == AIS_NOERROR);
        CHECK(data.MID == 3);       // Message Type 3
        CHECK(data.MMSI != NO_VALID_MMSI);
        CHECK(data.Lat  != NO_VALID_LATLON);
        CHECK(data.Lon  != NO_VALID_LATLON);
    }

    // 多句拼接（Message Type 5，静态数据，需要 2 句）
    TEST_CASE("Decode two-sentence type5 message (static data)")
    {
        myAISDecoder decoder;
        myAISData    data{};

        const char* s1 = "!AIVDM,2,1,7,B,56:QjBP2@BST9H<oN20QD60P4pN3>22222222216=@H4>547N@E0BK@j,0*78";
        const char* s2 = "!AIVDM,2,2,7,B,5CQwOV2@H3AC`80,2*3A";

        auto err1 = decoder.Decode(s1, data);
        CHECK(err1 == AIS_PARTIAL);   // 第一句返回 PARTIAL（等待第二句）

        auto err2 = decoder.Decode(s2, data);
        CHECK(err2 == AIS_NOERROR);
        CHECK(data.MID == 5);
        CHECK(data.MMSI != NO_VALID_MMSI);
    }

    // 非 AIS 句子（TXT 报文）应返回错误
    TEST_CASE("Non-AIS sentence returns error")
    {
        myAISDecoder decoder;
        myAISData    data{};

        const char* sentence = "$AITXT,01,01,91,FREQ,2087,2088*57";
        auto err = decoder.Decode(sentence, data);

        CHECK(err != AIS_NOERROR);
    }

    // 校验和错误
    TEST_CASE("Bad checksum returns error")
    {
        myAISDecoder decoder;
        myAISData    data{};

        // 末尾校验和故意写错（FF → 00）
        const char* sentence = "!AIVDM,1,1,,A,36:=nt5000`LFjL=wWd:e5c@0000,0*00";
        auto err = decoder.Decode(sentence, data);

        CHECK(err == AIS_NMEAVDM_CHECKSUM_BAD);
    }
}

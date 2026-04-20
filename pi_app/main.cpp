/**
 * @file  main.cpp  (pi_app)
 * @brief 树莓派端主程序 —— 纯计算，无图形界面
 *
 * 功能：
 *   1. 从标准输入 或 串口 读取 NMEA/AIS 数据流
 *   2. 解码 AIS 报文，维护目标表
 *   3. 运行 CPA/TCPA 碰撞避让算法
 *   4. 碰撞告警时向标准输出 / GPIO / 网络 输出告警信息
 *
 * 使用方式：
 *   cat AISMSG.txt | ./ais_pi_app          # 文件输入（测试）
 *   ./ais_pi_app --serial /dev/ttyUSB0     # 串口实时接收
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <csignal>
#include <ctime>

#include "ais/myaisdecoder.h"
#include "collision/colwarning.h"

// --------------------------------------------------------------------------
// 全局控制
// --------------------------------------------------------------------------
static volatile bool g_running = true;

static void sigHandler(int) { g_running = false; }

// --------------------------------------------------------------------------
// 将 AIS 目标信息打印到控制台（可替换为日志/网络输出）
// --------------------------------------------------------------------------
static void printTarget(const myAISData& d)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "[TARGET] MMSI=%-10d Name=%-21s SOG=%5.1f COG=%6.1f "
        "Lat=%10.5f Lon=%11.5f",
        d.MMSI, d.ShipName, d.SOG, d.COG, d.Lat, d.Lon);
    std::cout << buf << "\n";
}

static void printWarning(const myAISData& own,
                          const myAISData& target,
                          const ColResult&  result)
{
    std::fprintf(stderr,
        "\033[1;31m[WARNING] CPA=%.2f NM  TCPA=%.1f min  "
        "OWN=%-10d -> TARGET=%d (%s)\033[0m\n",
        result.cpa_nm, result.tcpa_min,
        own.MMSI, target.MMSI, target.ShipName);
}

// --------------------------------------------------------------------------
// 主循环
// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    // --- 命令行参数 ---
    std::string serialPort;
    for (int i = 1; i < argc - 1; ++i)
    {
        if (std::strcmp(argv[i], "--serial") == 0)
            serialPort = argv[i + 1];
    }

    // --- 输入源：串口 / stdin ---
    std::istream* input = &std::cin;
    std::ifstream serialFile;

    if (!serialPort.empty())
    {
        // 简单文件打开（实际串口配置由系统层 stty 完成，或使用 termios）
        serialFile.open(serialPort);
        if (!serialFile.is_open())
        {
            std::cerr << "[ERROR] 无法打开串口: " << serialPort << "\n";
            return 1;
        }
        input = &serialFile;
        std::cout << "[INFO] 使用串口: " << serialPort << "\n";
    }
    else
    {
        std::cout << "[INFO] 从 stdin 读取 AIS 数据 (Ctrl+C 退出)\n";
    }

    // --- 解码器 & 碰撞计算器 ---
    myAISDecoder decoder;
    ColWarning   colWarning(/*CPA门限(NM)=*/0.5, /*TCPA门限(min)=*/6.0);

    // 目标表：MMSI → AIS 数据
    std::map<int, myAISData> targetMap;

    // 本船数据（MMSI=0 表示尚未初始化，可通过 GPS 或固定配置设置）
    myAISData ownShip{};
    ownShip.MMSI = 0;
    ownShip.OwnShip = true;

    std::string line;
    int lineCount = 0;

    while (g_running && std::getline(*input, line))
    {
        if (line.empty()) continue;

        myAISData data{};
        auto err = decoder.Decode(line.c_str(), data);

        if ((err == AIS_NOERROR || err == AIS_PARTIAL)
            && data.MMSI != NO_VALID_MMSI)
        {
            // 更新目标表
            targetMap[data.MMSI] = data;
            printTarget(data);
            ++lineCount;

            // --- 碰撞避让检测 ---
            // 若 ownShip 尚未初始化，以第一艘船作为"本船"（仅供测试）
            if (ownShip.MMSI == 0 && data.Lat != NO_VALID_LATLON)
            {
                ownShip = data;
                ownShip.OwnShip = true;
                std::cout << "[INFO] 本船设定为 MMSI=" << ownShip.MMSI << "\n";
                continue;
            }

            if (ownShip.MMSI != 0 && data.MMSI != ownShip.MMSI)
            {
                ColResult res = colWarning.Compute(ownShip, data);
                if (res.is_warning)
                    printWarning(ownShip, data, res);
            }
        }
    }

    std::cout << "[INFO] 处理完毕，共解码 " << lineCount
              << " 条有效 AIS 报文，目标数: " << targetMap.size() << "\n";
    return 0;
}

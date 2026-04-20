#pragma once
#include <stdio.h>
#include <time.h>
#include <string>

#define AIS_MAX_MESSAGE_LEN (10 * 82)    
#define NO_VALID_MID		0
#define NO_VALID_MMSI		0
#define NO_VALID_NAVSTATUS	UNDEFINED
#define NO_VALID_SOG		-1
#define NO_VALID_COG		-1
#define NO_VALID_HDG		511.
#define NO_VALID_LATLON		360.

// Describe NavStatus variable
enum myAISNavStatus
{
    UNDERWAY_USING_ENGINE = 0,
    AT_ANCHOR,
    NOT_UNDER_COMMAND,
    RESTRICTED_MANOEUVRABILITY,
    CONSTRAINED_BY_DRAFT,
    MOORED,
    AGROUND,
    FISHING,
    UNDERWAY_SAILING,
    HSC,
    WIG,
    RESERVED_11,
    RESERVED_12,
    RESERVED_13,
    RESERVED_14,
    UNDEFINED,
    ATON_VIRTUAL,
    ATON_VIRTUAL_ONPOSITION,
    ATON_VIRTUAL_OFFPOSITION,
    ATON_REAL,
    ATON_REAL_ONPOSITION,
    ATON_REAL_OFFPOSITION
};

//Ais容错
enum myAISDecodeError
{
	AIS_NOERROR = 0,			//解析无错
	AIS_PARTIAL,				//部分缺省
	AIS_NMEAVDM_TOO_LONG,		//信息过长
	AIS_NMEAVDM_CHECKSUM_BAD,	//校验错误
	AIS_NMEAVDM_BAD,			//解析错误
};

enum myAISClass
{
	// AIS_CLASS_A = 0,				//A类
	// AIS_CLASS_B,					//B类
	// AIS_NAVIGATION_AIDS,			//助航物标
	// AIS_BASE_STATION,				//AIS基站

    AIS_CLASS_A = 0,
    AIS_CLASS_B,
    AIS_ATON,        // Aid to Navigation   pjotrc 2010/02/01
    AIS_BASE,        // Base station
    AIS_GPSG_BUDDY,  // GpsGate Buddy object
    AIS_DSC,         // DSC target
    AIS_SART,        // SART
    AIS_ARPA,        // ARPA radar target
    AIS_APRS         // APRS position report
};


// Describe AIS Alarm state——AIS警报的状态
enum myAISAlarmType
{
	AIS_NO_ALARM = 0,
	AIS_ALARM_SET,
	AIS_ALARM_ACKNOWLEDGED
};

struct myAISData
{
  int                       MID;			// 电文ID
  myAISClass                Class;		// 转发指示器
  int                       MMSI;			// 用户ID
  int                       NavStatus;	// 航行状态
  int                       ROTAIS;       // 旋回速率ROT
  double                    SOG;			// 实际航速SOG
  double                    Lon;			// 经度
  double                    Lat;			// 纬度
  double                    COG;			// 实际航向COG
  double                    HDG;			// 真艏向


  int                       IMO;			// IMO号码
  char                      CallSign[8];	// 呼号             // includes terminator
  char                      ShipName[21];	// 船名
  unsigned char             ShipType;		// 船型与货物种类
  int	                      PosAccuracy;	//位置精确度
  int	                      EquipmentType;	//定位设备类型
  int                       ETA_Mo;		// ETA
  int                       ETA_Day;
  int                       ETA_Hr;
  int                       ETA_Min;
  double                    Draft;		// 当前最大静态吃水
  char                      Destination[21];	// 目的地

  int                       SyncState;
  int                       SlotTO;
  int                       DimA;
  int                       DimB;
  int                       DimC;
  int                       DimD;
  time_t                    ReportTicks;
  // time_t                    RecentPeriod;
  bool                      b_active;
  myAISAlarmType            n_alarm_state;
  bool                      b_suppress_audio;
  bool                      b_positionValid;
  bool                      b_nameValid;

  int                       m_utc_hour;
  int                       m_utc_min;
  int                       m_utc_sec;

  // double                    Range_NM;
  // double                    Brg;

  // Per target collision parameters
  // bool                      bCPA_Valid;
  // double                    TCPA;                     // Minutes
  // double                    CPA;                      // Nautical Miles
  bool                      OwnShip;
};

class myAISDecoder
{
public:
  myAISDecoder(void);
  ~myAISDecoder(void);

  // 解析AIS源码数据
  myAISDecodeError Decode(const char* str, myAISData& data);

  char* GetVesselType(bool b_short, const myAISData& data);

protected:
  bool NMEACheckSumOK(const char* str);

private:
  int	m_nSentences;				//AIS句子总数
  int	m_nIsentence;				//当前AIS序号
  char m_SentenceAccumulator[AIS_MAX_MESSAGE_LEN];	//已获取合并的句子
  void InitAisData(myAISData& data);
};

#include "myaisdecoder.h"
#include <cstring>
#include <cstdio>

char ais_type[][80] =
{
  "Vessel Fishing",             //30        0       渔船
  "Vessel Towing",              //31        1       拖船
  "Vessel Towing, Long",        //32        2       
  "Vessel Dredging",            //33        3       挖泥船
  "Vessel Diving",              //34        4       潜水作业船
  "Military Vessel",            //35        5       军用船
  "Sailing Vessel",             //36        6       帆船
  "Pleasure craft",             //37        7       游艇
  "High Speed Craft",           //4x        8       高速船
  "Pilot Vessel",               //50        9       引航船
  "Search and Rescue Vessel",   //51        10      搜救船
  "Tug",                        //52        11      拖船
  "Port Tender",                //53        12      港口补给船
  "Pollution Control Vessel",   //54        13      污染控制船
  "Law Enforcement Vessel",     //55        14      执法船
  "Medical Transport",          //58        15      医疗运输船
  "Passenger Ship",             //6x        16      客船
  "Cargo Ship",                 //7x        17      货船
  "Tanker",                     //8x        18      油轮
  "Unknown",                    //          19      未知

  "Aid to Navigation",		//type 0	20        助航设备
  "Reference Point",			//01		21        参考点
  "RACON",     	            //02        22        雷达信标
  "Fixed Structure",            //03        23      固定建筑物/固定结构
  "Spare",                      //04        24      备件
  "Light",                      //05        25      灯标
  "Light w/Sectors",            //06        26      灯标扇形区
  "Leading Light Front",        //07        27      向前导航灯
  "Leading Light Rear",         //08        28      向后导航灯
  "Cardinal N Beacon",          //09        29
  "Cardinal E Beacon",          //10        30
  "Cardinal S Beacon",          //11        31
  "Cardinal W Beacon",          //12        32
  "Beacon, Port Hand",          //13        33
  "Beacon, Starboard Hand",     //14        34
  "Beacon, Preferred Channel Port Hand",         //15        35
  "Beacon, Preferred Channel Starboard Hand",    //16        36
  "Beacon, Isolated Danger",    //17        37
  "Beacon, Safe Water",         //18        38
  "Beacon, Special Mark",       //19        39
  "Cardinal Mark N",            //20        40
  "Cardinal Mark E",            //21        41
  "Cardinal Mark S",            //22        42
  "Cardinal Mark W",            //23        43
  "Port Hand Mark",             //24        44
  "Starboard Hand Mark",        //25        45
  "Preferred Channel Port Hand",      //26        46
  "Preferred Channel Starboard Hand", //27        47
  "Isolated Danger",            //28        48
  "Safe Water",                 //29        49
  "Special Mark",               //30        50
  "Light Vessel/Rig"            //31        51
};

char short_ais_type[][80] =
{
  "F/V",                  //30        0
  "Tow",                  //31        1
  "Long Tow",             //32        2
  "Dredge",               //33        3
  "D/V",                  //34        4
  "Mil/V",                //35        5
  "S/V",                  //36        6
  "Yat",                  //37        7
  "HSC",                  //4x        8
  "P/V",                  //50        9
  "SAR/V",                //51        10
  "Tug",                  //52        11
  "Tender",               //53        12
  "PC/V",                 //54        13
  "LE/V",                 //55        14
  "Med/V",                //58        15
  "Pass/V",               //6x        16
  "M/V",                  //7x        17
  "M/T",                  //8x        18
  "",                     //          19

  "AtoN",			//00		20  
  "Ref. Pt",		      //01		21
  "RACON",     	      //02        22
  "Fix.Struct.",          //03        23
  "",                     //04        24
  "Lt",                   //05        25
  "Lt sect.",             //06        26
  "Ldg Lt Front",         //07        27
  "Ldg Lt Rear",          //08        28
  "Card. N",              //09        29
  "Card. E",              //10        30
  "Card. S",              //11        31
  "Card. W",              //12        32
  "Port",                 //13        33
  "Stbd",                 //14        34
  "Pref. Chnl",           //15        35
  "Pref. Chnl",           //16        36
  "Isol. Dngr",           //17        37
  "Safe Water",           //18        38
  "Special",              //19        39
  "Card. N",              //20        40
  "Card. E",              //21        41
  "Card. S",              //22        42
  "Card. W",              //23        43
  "Port Hand",            //24        44
  "Stbd Hand",            //25        45
  "Pref. Chnl",           //26        46
  "Pref. Chnl",           //27        47
  "Isol. Dngr",           //28        48
  "Safe Water",           //29        49
  "Special",              //30        50
  "LtV/Rig"               //31        51
};

class AISBit
{
public:
  AISBit(const char *str);
  unsigned char to_6bit(const char c);
  int GetInt(int sp, int len);
  bool GetStr(int sp, int len, char *dest, int max_len);
private:
  unsigned char bitbytes[AIS_MAX_MESSAGE_LEN];
  int byte_length;
};

//---------------------------------------------------------------------------------
//  AIS Helpers
//---------------------------------------------------------------------------------
// AISBit类 对AIS的信息进行提取...

AISBit::AISBit(const char *str)
{
  byte_length = strlen(str);

  for (int i = 0; i < byte_length; i++)
  {
    bitbytes[i] = to_6bit(str[i]);
  }
}

//  Convert printable characters to IEC 6 bit representation
//  according to rules in IEC AIS Specification
unsigned char AISBit::to_6bit(const char c)
{
  if (c < 0x30)
    return (unsigned char)-1;
  if (c > 0x77)
    return (unsigned char)-1;
  if ((0x57 < c) && (c < 0x60))
    return (unsigned char)-1;

  unsigned char cp = c;
  cp += 0x28;

  if (cp > 0x80)
    cp += 0x20;
  else
    cp += 0x28;

  return (unsigned char)(cp & 0x3f);
}

int AISBit::GetInt(int sp, int len)
{
  int acc = 0;
  int s0p = sp - 1;                          // to zero base
  int cp, cx, c0, cs;
  for (int i = 0; i < len; i++)
  {
    acc = acc << 1;
    cp = (s0p + i) / 6;
    cx = bitbytes[cp];		// what if cp >= byte_length?
    cs = 5 - ((s0p + i) % 6);
    c0 = (cx >> (5 - ((s0p + i) % 6))) & 1;
    acc |= c0;
  }
  return acc;
}

bool AISBit::GetStr(int sp, int len, char *dest, int max_len)
{
  char temp_str[85];
  char acc = 0;
  int s0p = sp - 1;                          // to zero base
  int k = 0;
  int cp, cx, c0, cs;

  int i = 0;
  while (i < len)
  {
    acc = 0;
    for (int j = 0; j < 6; j++)
    {
      acc = acc << 1;
      cp = (s0p + i) / 6;
      cx = bitbytes[cp];		// what if cp >= byte_length?
      cs = 5 - ((s0p + i) % 6);
      c0 = (cx >> (5 - ((s0p + i) % 6))) & 1;
      acc |= c0;

      i++;
    }
    temp_str[k] = (char)(acc & 0x3f);

    if (acc < 32 && temp_str[k] != 0)
      temp_str[k] += 0x40;
    k++;
  }
  temp_str[k] = 0;

  int copy_len = strlen(temp_str);
  if (copy_len > max_len)
    copy_len = max_len;
  strncpy(dest, temp_str, copy_len);

  return true;
}

bool ParseVDMBitstring(AISBit *bstr, myAISData *ptd);

bool ParseVDMBitstring(AISBit *bstr, myAISData *ptd)
{
  bool parse_result = false;

  time_t now = time(NULL);
  ptd->ReportTicks = now;       // Default is my idea of NOW
  // which may disagee with target...

  int message_ID = bstr->GetInt(1, 6);        // Parse on message ID
  ptd->MID = message_ID;
  ptd->MMSI = bstr->GetInt(9, 30);            // MMSI is always in the same spot in the bitstream

  switch (message_ID)
  {
  case 1:                                 // Position Report
  case 2:
  case 3:
  {
    ptd->NavStatus = bstr->GetInt(39, 4);
    ptd->SOG = 0.1 * (bstr->GetInt(51, 10));
    ptd->PosAccuracy = bstr->GetInt(61, 1);
    int lon = bstr->GetInt(62, 28);
    if (lon & 0x08000000)                    // negative?
      lon |= 0xf0000000;
    ptd->Lon = lon / 600000.;

    int lat = bstr->GetInt(90, 27);
    if (lat & 0x04000000)                    // negative?
      lat |= 0xf8000000;
    ptd->Lat = lat / 600000.;

    ptd->COG = 0.1 * (bstr->GetInt(117, 12));
    ptd->HDG = 1.0 * (bstr->GetInt(129, 9));

    ptd->b_positionValid = true;

    ptd->ROTAIS = bstr->GetInt(43, 8);
    if (ptd->ROTAIS == 128)
      ptd->ROTAIS = 0;                      // not available codes as zero
    if ((ptd->ROTAIS & 0x80) == 0x80)
      ptd->ROTAIS = ptd->ROTAIS - 256;       // convert to twos complement

    ptd->m_utc_sec = bstr->GetInt(138, 6);

    if ((1 == message_ID) || (2 == message_ID))      // decode SOTDMA per 7.6.7.2.2
    {
      ptd->SyncState = bstr->GetInt(151, 2);
      ptd->SlotTO = bstr->GetInt(153, 2);
      if ((ptd->SlotTO == 1) && (ptd->SyncState == 0)) // UTCDirect follows
      {
        ptd->m_utc_hour = bstr->GetInt(155, 5);
        ptd->m_utc_min = bstr->GetInt(160, 7);
      }
    }
    parse_result = true;                // so far so good
    ptd->Class = AIS_CLASS_A;
    break;
  }

  case 18:
  {
    ptd->SOG = 0.1 * (bstr->GetInt(47, 10));
    ptd->PosAccuracy = bstr->GetInt(57, 1);

    int lon = bstr->GetInt(58, 28);

    if (lon & 0x08000000)                    // negative?
      lon |= 0xf0000000;
    ptd->Lon = lon / 600000.;

    int lat = bstr->GetInt(86, 27);

    if (lat & 0x04000000)                    // negative?
      lat |= 0xf8000000;
    ptd->Lat = lat / 600000.;

    ptd->COG = 0.1 * (bstr->GetInt(113, 12));
    ptd->HDG = 1.0 * (bstr->GetInt(125, 9));

    ptd->b_positionValid = true;

    ptd->m_utc_sec = bstr->GetInt(134, 6);

    parse_result = true;                // so far so good

    ptd->Class = AIS_CLASS_B;

    break;
  }
  case 19:
  {
    ptd->Class = AIS_CLASS_B;
    ptd->SOG = 0.1 * (bstr->GetInt(47, 10));
    ptd->PosAccuracy = bstr->GetInt(57, 1);

    int lon = bstr->GetInt(58, 28);

    if (lon & 0x08000000)                    // negative?
      lon |= 0xf0000000;
    ptd->Lon = lon / 600000.;

    int lat = bstr->GetInt(86, 27);

    if (lat & 0x04000000)                    // negative?
      lat |= 0xf8000000;
    ptd->Lat = lat / 600000.;

    ptd->COG = 0.1 * (bstr->GetInt(113, 12));
    ptd->HDG = 1.0 * (bstr->GetInt(125, 9));
    ptd->m_utc_sec = bstr->GetInt(134, 6);

    bstr->GetStr(144, 120, &ptd->ShipName[0], 20);
    ptd->b_nameValid = true;

    ptd->ShipType = (unsigned char)bstr->GetInt(264, 8);

    ptd->DimA = bstr->GetInt(272, 9);
    ptd->DimB = bstr->GetInt(281, 9);
    ptd->DimC = bstr->GetInt(290, 6);
    ptd->DimD = bstr->GetInt(296, 6);

    ptd->EquipmentType = bstr->GetInt(302, 4);
    parse_result = true;
  }
  break;
  case 5:
  {
    ptd->Class = AIS_CLASS_A;
      //          Get the AIS Version indicator
      //          0 = station compliant with Recommendation ITU-R M.1371-1
      //          1 = station compliant with Recommendation ITU-R M.1371-3
      //          2-3 = station compliant with future editions
    int AIS_version_indicator = bstr->GetInt(39, 2);
    if (AIS_version_indicator < 4)
    {
      ptd->IMO = bstr->GetInt(41, 30);

      bstr->GetStr(71, 42, &ptd->CallSign[0], 7);
      bstr->GetStr(113, 120, &ptd->ShipName[0], 20);
      ptd->b_nameValid = true;
      ptd->ShipType = (unsigned char)bstr->GetInt(233, 8);
      ptd->DimA = bstr->GetInt(241, 9);
      ptd->DimB = bstr->GetInt(250, 9);
      ptd->DimC = bstr->GetInt(259, 6);
      ptd->DimD = bstr->GetInt(265, 6);
      ptd->EquipmentType = bstr->GetInt(271, 4);
      ptd->ETA_Mo = bstr->GetInt(275, 4);
      ptd->ETA_Day = bstr->GetInt(279, 5);
      ptd->ETA_Hr = bstr->GetInt(284, 5);
      ptd->ETA_Min = bstr->GetInt(289, 6);
      ptd->Draft = (double)(bstr->GetInt(295, 8)) * 0.1;
      bstr->GetStr(303, 120, &ptd->Destination[0], 20);
      parse_result = true;
    }
    break;
  }
  case 24:
  {
    ptd->Class = AIS_CLASS_B;

    int part_number = bstr->GetInt(39, 2);
    if (0 == part_number)
    {
      bstr->GetStr(41, 120, &ptd->ShipName[0], 20);
      ptd->b_nameValid = true;
      parse_result = true;
    }
    else if (1 == part_number)
    {
      ptd->ShipType = (unsigned char)bstr->GetInt(41, 8);
      bstr->GetStr(91, 42, &ptd->CallSign[0], 7);

      ptd->DimA = bstr->GetInt(133, 9);
      ptd->DimB = bstr->GetInt(142, 9);
      ptd->DimC = bstr->GetInt(151, 6);
      ptd->DimD = bstr->GetInt(157, 6);
      parse_result = true;
    }


    break;
  }
  case 4:                                    // base station
  {
    ptd->Class = AIS_BASE;
    /*
    ptd->utcYear = bstr->GetInt(39,14);
    ptd->utcMonth = bstr->GetInt(53,4);
    ptd->utcDay = bstr->GetInt(57,5);*/

    ptd->m_utc_hour = bstr->GetInt(62, 5);
    ptd->m_utc_min = bstr->GetInt(67, 6);
    ptd->m_utc_sec = bstr->GetInt(73, 6);

    if ((ptd->m_utc_hour < 24)
      && (ptd->m_utc_min < 60)
      && (ptd->m_utc_sec < 60))
    {
      //strtime.Format(_T("%d-%02d-%2d %02d:%02d:%02d"),
      //m_utc_year,m_utc_month,m_utc_day,m_utc_hour,m_utc_min,m_utc_sec);
    }

    ptd->PosAccuracy = bstr->GetInt(79, 1);

    int lon = bstr->GetInt(80, 28);

    if (lon & 0x08000000)                    // negative?
      lon |= 0xf0000000;
    ptd->Lon = lon / 600000.;

    int lat = bstr->GetInt(108, 27);

    if (lat & 0x04000000)                    // negative?
      lat |= 0xf8000000;
    ptd->Lat = lat / 600000.;

    ptd->COG = -1.;
    ptd->HDG = 511;
    ptd->SOG = -1.;

    ptd->EquipmentType = bstr->GetInt(135, 4);
    parse_result = true;
    break;
  }
  case 9:                                    // Special Position Report (Standard SAR Aircraft Position Report)
  {
      ptd->Class = AIS_SART;

      ptd->SOG = bstr->GetInt(51, 10);

      int lon = bstr->GetInt(62, 28);
      if (lon & 0x08000000)  // negative?
          lon |= 0xf0000000;
      double lon_tentative = lon / 600000.;

      int lat = bstr->GetInt(90, 27);
      if (lat & 0x04000000)  // negative?
          lat |= 0xf8000000;
      double lat_tentative = lat / 600000.;

      if ((lon_tentative <= 180.) &&
          (lat_tentative <= 90.))  // Ship does not report Lat or Lon "unavailable"
      {
          ptd->Lon = lon_tentative;
          ptd->Lat = lat_tentative;
          ptd->b_positionValid = false;
      }
      else
          ptd->b_positionValid = true;

      //    decode balance of message....
      ptd->COG = 0.1 * (bstr->GetInt(117, 12));

      int alt_tent = bstr->GetInt(39, 12);

      parse_result = true;

    break;
  }
  case 21:                                    // Test Message (Aid to Navigation)   pjotrc 2010.02.01
  {
    int lon = bstr->GetInt(165, 28);

    if (lon & 0x08000000)                    // negative?
      lon |= 0xf0000000;
    ptd->Lon = lon / 600000.;

    int lat = bstr->GetInt(193, 27);

    if (lat & 0x04000000)                    // negative?
      lat |= 0xf8000000;
    ptd->Lat = lat / 600000.;

    ptd->b_positionValid = true;
    //
    // The following is a patch to impersonate an AtoN as Ship
    //
    ptd->NavStatus = MOORED;
    ptd->ShipType = (unsigned char)bstr->GetInt(39, 5);
    ptd->IMO = 0;
    ptd->SOG = 0;
    ptd->HDG = 0;
    ptd->COG = 0;
    ptd->ROTAIS = 0;			// i.e. not available
    ptd->DimA = bstr->GetInt(220, 9);
    ptd->DimB = bstr->GetInt(229, 9);
    ptd->DimC = bstr->GetInt(238, 6);
    ptd->DimD = bstr->GetInt(244, 6);
    ptd->Draft = 0;
    bstr->GetStr(44, 120, &ptd->ShipName[0], 20); // short name only, extension wont fit in Ship structure
    ptd->b_nameValid = true;
    ptd->m_utc_sec = bstr->GetInt(254, 6);
    parse_result = true;                // so far so good
    ptd->Class = AIS_ATON;

    break;
  }
  case 8:                                    // Binary Broadcast
  {
    break;
  }
  case 6:                                    // Addressed Binary Message
  {
    break;
  }
  case 7:                                    // Binary Ack
  {
    break;
  }
  case 27:
  {
      // Long-range automatic identification system broadcast message
// This message is used for long-range detection of AIS Class A and Class
// B vessels (typically by satellite).

// Define the constant to do the covertion from the internal encoded
// position in message 27. The position is less accuate :  1/10 minute
// position resolution.
      int bitCorrection = 10;
      int resolution = 10;

      // Default aout of bounce values.
      double lon_tentative = 181.;
      double lat_tentative = 91.;

      // It can be both a CLASS A and a CLASS B vessel - We have decided for
      // CLASS A
      // TODO: Lookup to see if we have seen it as a CLASS B, and adjust.
      ptd->Class = AIS_CLASS_A;

      ptd->NavStatus = bstr->GetInt(39, 4);

      int lon = bstr->GetInt(45, 18);
      int lat = bstr->GetInt(63, 17);

      lat_tentative = lat;
      lon_tentative = lon;

      // Negative latitude?
      if (lat >= (0x4000000 >> bitCorrection)) {
          lat_tentative = (0x8000000 >> bitCorrection) - lat;
          lat_tentative *= -1;
      }

      // Negative longitude?
      if (lon >= (0x8000000 >> bitCorrection)) {
          lon_tentative = (0x10000000 >> bitCorrection) - lon;
          lon_tentative *= -1;
      }

      // Decode the internal position format.
      lat_tentative = lat_tentative / resolution / 60.0;
      lon_tentative = lon_tentative / resolution / 60.0;

      // Get the latency of the position report.
      int positionLatency = bstr->GetInt(95, 1);

      if ((lon_tentative <= 180.) &&
          (lat_tentative <= 90.))  // Ship does not report Lat or Lon "unavailable"
      {
          ptd->Lon = lon_tentative;
          ptd->Lat = lat_tentative;
          ptd->b_positionValid = true;
          if (positionLatency == 0) {
              // The position is less than 5 seconds old.
          }
      }
      else
          ptd->b_positionValid = false;

      ptd->SOG = 1.0 * (bstr->GetInt(80, 6));
      ptd->COG = 1.0 * (bstr->GetInt(85, 9));

      parse_result = true;

      break;
  }
  default:
  {
    break;
  }

  }

  if (true == parse_result)
    ptd->b_active = true;

  return parse_result;
}

/************************************************************************/
/* AIS DECODER                                                          */
/************************************************************************/

myAISDecoder::myAISDecoder(void)
{
  memset(m_SentenceAccumulator, 0, AIS_MAX_MESSAGE_LEN);
}


myAISDecoder::~myAISDecoder(void)
{
}

myAISDecodeError myAISDecoder::Decode(const char* str, myAISData& data)
{
  InitAisData(data);

  myAISDecodeError ret = AIS_NOERROR;
  //  Make some simple tests for validity
  //    int nlen = str.Len();

  if (strlen(str) > 100)
    return AIS_NMEAVDM_TOO_LONG;

  if (!NMEACheckSumOK(str))
  {
    return AIS_NMEAVDM_CHECKSUM_BAD;
  }

  if (strncmp(str + 3, "VDM", 3) != 0 && strncmp(str + 3, "VDO", 3) != 0)
  {
    return AIS_NMEAVDM_BAD;
  }

  if (strncmp(str + 3, "VDM", 3) == 0)
  {
    data.OwnShip = false;
  }

  if (strncmp(str + 3, "VDO", 3) == 0)
  {
    data.OwnShip = true;
  }

  //  OK, looks like the sentence is OK
  //  Use a tokenizer to pull out the first 4 fields
  char* input = strdup(str);
  char* token = NULL;
  char control_ch = ',';
  int found = 0;
  int last_found = -1;
  int len = strlen(input);
  int lsequence_id = 0;
  int lchannel = 0;

#define __FIND_CH( _str, _len, _i, _ch )   \
	while ( _i <= _len ) \
	{   \
	if ( _str[_i] == _ch ) \
	break;  \
	_i++;   \
	}

#define __GET_TOKEN( _str, _token,  _from, _to, _len )  \
	if ( _to != _len )   \
	{   \
	_token = _str+ _from + 1;  \
	_str[_to] = '\0'; \
	_from = _to; \
	}   \
	else \
	{   \
	_token = NULL;   \
	}

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);     // !xxVDM

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);
  if (token != NULL)
    m_nSentences = atoi(token);

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);
  if (token != NULL)
    m_nIsentence = atoi(token);

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);
  if (token != NULL)
    lsequence_id = atoi(token);

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);
  if (token != NULL)
    lchannel = atoi(token);

  __FIND_CH(input, len, found, control_ch);
  __GET_TOKEN(input, token, last_found, found, len);

  //  Now, some decisions

  char* string_to_parse = NULL;

  //  Simple case first
  //  First and only part of a one-part sentence
  if ((1 == m_nSentences) && (1 == m_nIsentence))
  {
    string_to_parse = token;         // the encapsulated data
  }
  else if (m_nSentences > 1)
  {
    if (token != NULL)
    {
      if (1 == m_nIsentence)
      {
        strncpy(m_SentenceAccumulator, token, AIS_MAX_MESSAGE_LEN - 1);
        m_SentenceAccumulator[AIS_MAX_MESSAGE_LEN - 1] = '\0';  // the encapsulated data
      }

      else
      {
        if (strlen(m_SentenceAccumulator) == 0)
        {
          return AIS_PARTIAL;
        }
        strncat(m_SentenceAccumulator, token, AIS_MAX_MESSAGE_LEN - strlen(m_SentenceAccumulator) - 1);
      }

      if (m_nIsentence == m_nSentences)
      {
        string_to_parse = m_SentenceAccumulator;
      }
    }
  }
  if (string_to_parse != NULL && strlen(string_to_parse) < AIS_MAX_MESSAGE_LEN)
  {

    //  Create the bit accessible string
    AISBit strbit(string_to_parse);

    //  Extract the MMSI
    int mmsi = strbit.GetInt(9, 30);

    //        int message_ID = strbit.GetInt(1, 6);

    //AISData *pStaleTarget = &m_data;
    bool bnewtarget = false;

    //  Grab the stale targets's last report time
    time_t last_report_ticks = time(NULL);
    // if (&data)
    last_report_ticks = data.ReportTicks;

    bool bhad_name = false;
    // if (&data)
    bhad_name = data.b_nameValid;


    bool bdecode_result = ParseVDMBitstring(&strbit, &data);            // Parse the new data

    //  If the message was decoded correctly, Update the AIS Target in the Selectable list, and update the TargetData
    if (bdecode_result)
    {
      //     Update the most recent report period
      // data.RecentPeriod = data.ReportTicks - last_report_ticks;
    }

    ret = AIS_NOERROR;

  }
  else
    ret = AIS_PARTIAL;                // accumulating parts of a multi-sentence message

  free(input);

  if (m_nIsentence == m_nSentences)
  {
    memset(m_SentenceAccumulator, 0, AIS_MAX_MESSAGE_LEN);
  }
  return ret;
}

bool myAISDecoder::NMEACheckSumOK(const char* str)
{
  unsigned char checksum_value = 0;

  int string_length = strlen(str);

  int payload_length = 0;
  while ((payload_length < string_length) && (str[payload_length] != '*')) // look for '*'
    payload_length++;

  if (payload_length == string_length) return false; // '*' not found at all, no checksum

  int index = 1; // Skip over the $ at the begining of the sentence

  while (index < payload_length)
  {
    checksum_value ^= str[index];
    index++;
  }

  if (string_length > 4)
  {
    char scanstr[3];
    scanstr[0] = str[payload_length + 1];
    scanstr[1] = str[payload_length + 2];
    scanstr[2] = 0;
    unsigned int sentence_hex_sum_u = 0;
    sscanf(scanstr, "%2x", &sentence_hex_sum_u);
    int sentence_hex_sum = static_cast<int>(sentence_hex_sum_u);

    if (sentence_hex_sum == checksum_value)
      return true;
  }

  return false;
}

void myAISDecoder::InitAisData(myAISData& data)
{
  memset(&data, 0, sizeof(data));
  data.Class = AIS_CLASS_A;	//类型

  data.SOG = NO_VALID_SOG;		//对地航速
  data.COG = NO_VALID_COG;		//对地航向
  data.HDG = NO_VALID_HDG;		//船首向
  data.Lon = NO_VALID_LATLON;	//经度
  data.Lat = NO_VALID_LATLON;	//纬度

  data.MMSI = NO_VALID_MMSI;		//船舶识别号
  data.MID = NO_VALID_MID;		//信息ID码
  data.NavStatus = NO_VALID_NAVSTATUS;//航行状态
  data.ROTAIS = 0;	//转向率

  data.ETA_Mo = 0;		//预计时间
  data.ETA_Day = 0;
  data.ETA_Hr = 0;
  data.ETA_Min = 0;
}

// 获得AIS船的类型 从ais_type和short_ais_type数组中获取...
char* myAISDecoder::GetVesselType(bool b_short, const myAISData& data)
{
  int i = 19;
  if (data.Class == AIS_ATON)
  {
    i = data.ShipType + 20;
  }
  else
    switch (data.ShipType)
    {
    case 30:
      i = 0; break;
    case 31:
      i = 1; break;
    case 32:
      i = 2; break;
    case 33:
      i = 3; break;
    case 34:
      i = 4; break;
    case 35:
      i = 5; break;
    case 36:
      i = 6; break;
    case 37:
      i = 7; break;
    case 50:
      i = 9; break;
    case 51:
      i = 10; break;
    case 52:
      i = 11; break;
    case 53:
      i = 12; break;
    case 54:
      i = 13; break;
    case 55:
      i = 14; break;
    case 58:
      i = 15; break;
    default:
      i = 19; break;
    }

  if ((data.Class == AIS_CLASS_B) || (data.Class == AIS_CLASS_A))
  {
    if ((data.ShipType >= 40) && (data.ShipType < 50))
      i = 8;

    if ((data.ShipType >= 60) && (data.ShipType < 70))
      i = 16;

    if ((data.ShipType >= 70) && (data.ShipType < 80))
      i = 17;

    if ((data.ShipType >= 80) && (data.ShipType < 90))
      i = 18;
  }

  if (!b_short)
    return &ais_type[i][0];
  else
    return &short_ais_type[i][0];
}

#pragma once
#include <atomic>
#include <algorithm>
#include <memory>
#include <map>
#include <mutex>
#include <iostream>
#include <condition_variable>

#include "MbimService.h"
#include "MbimToSar.h"
#pragma pack(push, 1)
using namespace std;

/**
 * PC基于该信令下发查询/设置SAR Enable/Disable、SetSarBackoffIndex。
 */

// DATA
typedef struct
{
	uint32_t SARAntennaIndex; // 指定天线号
	uint32_t SARBAckOffIndex; // 功率回退等级(0-8)
} MBIM_SAR_STATE_BACKOFF_DATA;

// REQ
typedef struct
{
	uint32_t SARMode;					 // SAR工作模式(目前支持0 Device 1 OS)
	uint32_t SARBackOffState;			 // SAR工作状态(目前支持Enable/Disable)
	uint32_t ElementCount;				 // 支持的组数(目前仅支持1)
	MBIM_SAR_STATE_BACKOFF_DATA data[0]; // 保存指定天线号及该天线的功率回退等级
} MBIM_SAR_STATE_BACKOFF_REQ;

// RESP
typedef struct
{
	uint32_t SARMode;					 // SAR工作模式(目前支持0 Device 1 OS)
	uint32_t SARBackOffStatus;			 // SAR工作状态(目前支持Enable/Disable)
	uint32_t SARWifiIntegration;		 // WIFI信息(当前不支持)
	uint32_t ElementCount;				 // 发射天线号(目前仅支持1)
	MBIM_SAR_STATE_BACKOFF_DATA data[0]; // 保存指定天线号及该天线的功率回退等级
} MBIM_SAR_STATE_BACKOFF_RESP;

/**
 * PC基于该信令下发查询/设置Quectel定义的SarMode(GPIO Or SW Control)、
 * SarProfile(sar table)、SarPowerNv(SarIndex Tx Power)。 
 */
// BodySarMode数据结构成员值域
#define QUEC_SVC_MSSAR_TRIGGER_DEFAULT_MODE 0 // 未定义状态
#define QUEC_SVC_MSSAR_TRIGGER_AT_MODE 1	  // AT命令软件控制
#define QUEC_SVC_MSSAR_TRIGGER_PIN_MODE 2	  // GPIO控制
#define QUEC_SVC_MSSAR_TRIGGER_MBIM_MODE 3	  // MBIM协议控制

// BodySarProfile数据结构成员值域
#define QUEC_MBIM_MS_SAR_PROFILE0 0 // Table 0 选中配置
#define QUEC_MBIM_MS_SAR_PROFILE1 1 // Table 1 选中配置

// BodySarTech1数据结构成员值域
#define LTE 0	// LTE
#define WCDMA 1 // WCDMA

typedef struct
{
	uint8_t BodySarMode;	  // SAR工作模式(0====default, 1==AT,2== HW control by DPR pin----3 mbim control)
	uint8_t BodySarProfile;	  // SAR 表格号
	uint8_t BodySarTech0[24]; // 指定BodySarStruct的数据长度(目前固定24字节)
	uint8_t BodySarTech1;	  // 请求制式(LTE/WCDMA)
	uint16_t BodySarPower[8]; // 当前服役的SAR Table
	uint8_t BodySarBand;	  // 请求频段(band等)
	uint8_t BodySarOnTable;	  // 请求生校表格
} MBIM_SAR_QUEC_DATA;

// 请求模式(SET/QUERY)
#define QUEC_MBIM_MS_SAR_BACKOFF_SET 0x92F456E8	  // QuecMode SET
#define QUEC_MBIM_MS_SAR_BACKOFF_QUERY 0x8204F6F9 // QuecMode QUERY

// 请求模块配置状态
#define QUEC_SVC_MSSAR_NULL_STATE 0					 // 保留
#define QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_SET 1	 // 设置 SAR工作模式
#define QUEC_SVC_MSSAR_BODY_SAR_MODE_STATE_GET 2	 // 请求 SAR工作模式
#define QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_SET 3	 // 设置 SAR Table 表号
#define QUEC_SVC_MSSAR_BODY_SAR_PROFILE_VALUE_GET 4	 // 请求 SAR Table 表号
#define QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_SET 5		 // 设置 SAR TX Power NV
#define QUEC_SVC_MSSAR_BODY_SAR_CONFIG_NV_GET 6		 // 请求 SAR TX Power NV
#define QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_SET 7 // 设置 SAR Table 生校表格
#define QUEC_SVC_MSSAR_BODY_SAR_ON_TABLE_VALUE_GET 8 // 请求 SAR Table 生校表格
#define QUEC_SVC_MSSAR_BODY_SAR_CLEAR_STATE_SET 9	 // 恢复出厂设置
typedef struct
{
	uint32_t QuecMode;		  // 请求模式(SET/QUERY)
	uint32_t QuecGetSetState; // 请求模块配置状态
	uint32_t QuecSetCount;	  // 请求配置的数据长度
	MBIM_SAR_QUEC_DATA data[0];
} MBIM_SAR_QUEC_REQ;

// RESP
typedef struct
{
	uint32_t QuecMode;			// 请求模式(SET/QUERY)
	uint32_t QuecGetSetState;	// 请求模块配置状态
	uint32_t QuecWifiNull;		// WIFI信息(当前不支持)
	uint32_t QuecSetCount;		// 数据长度
	MBIM_SAR_QUEC_DATA data[0]; // 数据
} MBIM_SAR_QUEC_RESP;

#pragma pack(pop)
/******************* end ******************/

class CMbimManager : public IObserver
{
private:
	static std::mutex mGlobalLock;
	static MbimService mService;

private:
	std::mutex mReqLock;
	std::condition_variable mReqCond;
	uint32_t mRequestId;

private:
	bool mSarEnable;
	bool mSarMode;

private:
	CMbimManager(const CMbimManager &) = delete;
	CMbimManager &operator=(const CMbimManager &) = delete;

public:
	CMbimManager();
	~CMbimManager();

	static int Init(const char *d, bool use_sock = false);
	static void UnInit();
	static CMbimManager &instance();

	static int openCommandSession(CMbimManager *mm, const char *dev);
	static int closeCommandSession(CMbimManager *mm, uint32_t *rid);

public:
	uint32_t getRequestId();

	int GetIsMbimReady(bool *);

	int SetSarEnable(bool bEnable);

	int GetSarEnable(bool *bEnable);

	int SetSarMode(int nMode);

	int GetSarMode(int *nMode);

	int SetSarProfile(int nMode, int nTable);

	int GetSarProfile(int nMode, int *nTable);

	int SetSarTableOn(int nMode, int nTable);

	int GetSarTableOn(int nMode, int *nTable);

	int GetSarState(int *sartable);

	int SetSarState(int sartable);

	int SetSarValue(int nMode, int nProfile, int nTech, MBIM_SAR_BAND_POWER sarbandpower);

	int GetSarValue(int nMode, int nProfile, int nTech, int nBand, MBIM_SAR_BAND_POWER *sarbandpower);

	int SetSarClear();

	void OpenCmdSessionCb();

	void update(uint8_t *, int);
};

typedef CMbimManager MBIMMANAGER;

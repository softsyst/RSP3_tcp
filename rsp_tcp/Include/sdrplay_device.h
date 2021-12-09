/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017 Clem Schmidt, softsyst GmbH, http://www.softsyst.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
**/

#pragma once
#include "rsp_tcp.h"
#include "common.h"
#include "IPAddress.h"
#include "rsp_cmdLineArgs.h"
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#ifdef _WIN32
#define sleep(n) Sleep(n*1000)
#define usleep(n) Sleep(n/1000)
#else
#include <string.h> //memset etc.
#include <unistd.h>
#endif
using namespace std;

enum eCommState
{
	ST_IDLE = 0
	, ST_SERIALS_REQUESTED    
	, ST_DEVICE_CREATED    
	, ST_WELCOME_SENT 	  
	, ST_DEVICE_RELEASED 	  
};


static bool VERBOSE = false;
const int MAX_TUNERS = 2;

void* receive(void* md);
void streamACallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
	unsigned int numSamples, unsigned int reset, void *cbContext);

void streamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
	unsigned int numSamples, unsigned int reset, void *cbContext);

void gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void* cbContext);

struct samplingConfiguration
{
	const int samplingRateHz;
	const int deviceSamplingRateHz;
	const sdrplay_api_Bw_MHzT bandwidth;
	const int decimationFactor;
	const bool doDecimation;

	samplingConfiguration(int srHz, int devSrHz, sdrplay_api_Bw_MHzT bw, int decimFact, bool doDecim)
	  : samplingRateHz(srHz), 
		deviceSamplingRateHz(devSrHz),
		bandwidth(bw),
		decimationFactor(decimFact),
		doDecimation(doDecim)
	{
	}
};

typedef struct
{
	void *dev;
	int port;
	int wait;
	const char *addr;
	bool* pDoExit;
}
ctrl_thread_data_t;
void *ctrl_thread_fn(void *arg);
class crc32;

class sdrplay_device
{
public:
	sdrplay_device(rsp_cmdLineArgs* args);
	virtual ~sdrplay_device();
	sdrplay_api_CallbackFnsT cbFns;

private:
	sdrplay_device() {}
	crc32* _crc32;

	int getSamplingConfigurationTableIndex(int requestedSrHz);
	void writeWelcomeString() const;
	void cleanup();
	sdrplay_api_DeviceT* pDevice;
	rsp_cmdLineArgs* pargs;

	friend void* receive(void* p);
	//friend void streamACallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
	//	unsigned int numSamples, unsigned int reset, void *cbContext);

	//friend void streamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
	//	unsigned int numSamples, unsigned int reset, void *cbContext);

	friend void streamCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params,
		unsigned int numSamples, unsigned int reset, void *cbContext);

	friend void eventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,
		sdrplay_api_EventParamsT *params, void *cbContext);


public:
	eCommState CommState = ST_IDLE;
	pthread_mutex_t mutex_rxThreadStarted;
	pthread_cond_t started_cond = PTHREAD_COND_INITIALIZER;
	pthread_t* thrdRx;
	pthread_t* thrdCtrl;
	ctrl_thread_data_t ctrlThreadData;
	bool ctrlThreadExitFlag = false;

	/// <summary>
	/// Current values, to be sent to the host
	/// </summary>
	sdrplay_api_GainValuesT GainValues;

	// HW version
	HANDLE hdl;         // Handle of the device
	sdrplay_api_DeviceParamsT *deviceParams = NULL;
	bool isStreaming;
	bool flatGr;		// true: dont use the gain reduction tables
	int LNAstate;		// Calculated from the RequestedGain

	bool overloaded_A = false;
	bool overloaded_B = false;

	bool AGC_A = false;
	bool AGC_B = false;

	int RequestedGain; // the gain requested from the user, NOT the gain reduction used by the RSP
	bool started = false;
	//The socket of the remote app
	SOCKET remoteClient;
	eRxType rxType;

	bool DeviceSelected = false;
	bool Initialized = false;
	const int c_welcomeMessageLength = 100;

private:
	sdrplay_api_DeviceT sdrplayDevices[MAX_DEVICES];
	uint32_t serialCRCs[MAX_DEVICES];
	int numDevices;

	bool RSPGainValuesFromRequestedGain(int flatValue, int rxtype, int& LNAstate, int& gr);
	//bool selectDevice(rsp_cmdLineArgs* args);
	sdrplay_api_ErrT  selectDevice(uint32_t crc);
	void selectChannel(sdrplay_api_TunerSelectT tunerId);

	BYTE* mergeIQ(const short* idata, const short* qdata, int samplesPerPacket, int& buflen);
	sdrplay_api_ErrT createChannels();
	sdrplay_api_ErrT setFrequency(int valueHz);
	sdrplay_api_ErrT setFrequencyCorrection(int value);
	sdrplay_api_ErrT setFrequencyCorrection100(int value);
	sdrplay_api_ErrT setBiasT(int value);
	sdrplay_api_ErrT setAntenna(int value);
	sdrplay_api_ErrT setLNAState(int value);
	sdrplay_api_ErrT setAGC(bool on);
	sdrplay_api_ErrT setGain(int value);
	sdrplay_api_ErrT setSamplingRate(int requestedSrHz);
	sdrplay_api_ErrT stream_Uninit();
	sdrplay_api_ErrT setAdsbMode();

	//Reference: rtl_tcp.c fct command_worker, line 277
	//Copied from QIRX
	enum eRTLCommands
	{
		CMD_SET_FREQUENCY = 1
		, CMD_SET_SAMPLINGRATE = 2
		, CMD_SET_GAIN_MODE = 3               //int manual
		, CMD_SET_GAIN = 4                    //int gain, tenth dB; fct tuner_r82xx.c fct r82xx_set_gain
		, CMD_SET_FREQUENCYCORRECTION = 5     //int ppm
		, CMD_SET_IF_GAIN = 6                 //int stage, int gain
		, CMD_SET_AGC_MODE = 8                //int on
		, CMD_SET_DIRECT_SAMPLING = 9         //int on
		, CMD_SET_OFFSET_TUNING = 10          //int on
		, CMD_SET_TUNER_GAIN_BY_INDEX = 13
		, CMD_SET_BIAS_T = 14				  //int on
		, CMD_SET_RSP2_ANTENNA_CONTROL = 33   //int Antenna Select
		, CMD_SET_FREQUENCYCORRECTION_PPM100 = 0x4a            // int ppm*100
		, CMD_SET_RSP_LNA_STATE = 0x4b        // 0: most sensitive, 8: least sensitive
		, CMD_SET_RSP_REQUEST_ALL_SERIALS = 0x80      // request for all serials to be transmitted via back channel
		, CMD_SET_RSP_SELECT_SERIAL = 0x81    // value is four bytes CRC-32 of the requested serial number
	};

	// This server is able to stream native 16-bit data (of "short" type)
	// or - for comaptibility with some apps, 8-bit data, 
	// where ( 8-bit Byte) =  ( 16-bit short /64) + 127
	eBitWidth bitWidth = BITS_16;

	// Reasonable number of possible bandwidth/sampling rate combinations
	//samplingConfiguration(int srHz, int devSrHz, sdrplay_api_Bw_MHzT bw, int decimFact, bool doDecim)
		const int c_numSamplingConfigs = 10;
		samplingConfiguration samplingConfigs[10] = {
		samplingConfiguration(512000, 2048000,  sdrplay_api_BW_0_300, 4, true),
		samplingConfiguration(1024000, 2048000, sdrplay_api_BW_0_600, 2, true),
		samplingConfiguration(2048000, 2048000, sdrplay_api_BW_1_536, 1, false),
		samplingConfiguration(4096000, 4096000, sdrplay_api_BW_5_000, 1, false),
		samplingConfiguration(8192000, 8192000, sdrplay_api_BW_8_000, 1, false),
		samplingConfiguration(3000000, 3000000, sdrplay_api_BW_1_536, 1, false),
		samplingConfiguration(4000000, 4000000, sdrplay_api_BW_1_536, 1, false),
		samplingConfiguration(2400000, 2400000, sdrplay_api_BW_1_536, 1, false),
		samplingConfiguration(2500000, 2500000, sdrplay_api_BW_1_536, 1, false),
		samplingConfiguration(2000000, 2000000, sdrplay_api_BW_1_536, 1, false)
		//samplingConfiguration(2000000, 8000000, sdrplay_api_BW_8_000, 4, true)
	};


	//Generic API error type
	sdrplay_api_ErrT err;

	int sys = 40;
	int agcPoint_dBfs = -25;

	// currently commanded values
	int currentFrequencyHz;
	int gainReduction;				// Calculated from the RequestedGain
	double currentSamplingRateHz;
	int antenna = 5;
	
	//Callbacks per second, for a "timer" to discard samples 
	//to prevent overrun in the device in the error case
	int cbksPerSecond = 0;
	bool cbkTimerStarted = false;

	sdrplay_api_RxChannelParamsT* pCurCh;

public:
	sdrplay_api_DeviceT* getDevice()
	{
		return pDevice;
	}
	void setDevice(sdrplay_api_DeviceT* dev)
	{
		pDevice = dev;
	}
	bool collectDevices();	// called from the controlThread, on clients request
	int prepareSerialsList(BYTE* buf);
	void init(rsp_cmdLineArgs* pargs);
	void start(SOCKET client);
	void stop();
	void createCtrlThread(const char* addr, int port);
	sdrplay_api_GainValuesT* getGainValues();
	int getLNAState();
	int getRxString(char* s ) const;
	int getExportedRxType() const { return rxType + 7 ; }
	int getBitWidth() const { return bitWidth; }
	int deviceCount() const { return numDevices; }
	bool releaseDevice()
	{
		if (pDevice != 0 && sdrplay_api_ReleaseDevice(pDevice) == sdrplay_api_Success)
			return true;
		return false;
	}

	/// <summary>
	/// API: Device Enumeration Structure
	/// </summary>
	string serno() { if (pDevice == 0) return "";  return pDevice->SerNo; }		// serial number
	BYTE hwVer() { if (pDevice == 0) return 0; return pDevice->hwVer; }

	void getOverload(bool& overload_a, bool& overload_b) {
		overload_a = overloaded_A;
		overload_b = overloaded_B;
	}

};


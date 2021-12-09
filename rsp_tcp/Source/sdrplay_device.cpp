/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017-2021 Clem Schmidt, softsyst, http://qirx.softsyst.com
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
//#define TIME_MEAS
#include "devices.h"
#include "sdrplay_device.h"
#include "sdrGainTable.h"
//#include "MeasTimeDiff.h"
#include <string.h>
#include <iostream>
using namespace std;

static LARGE_INTEGER Count1, Count2;
extern bool exitRequest;


sdrplay_device::~sdrplay_device()
{
	pthread_mutex_destroy(&mutex_rxThreadStarted);
	pthread_cond_destroy(&started_cond);
}

sdrplay_device::sdrplay_device(rsp_cmdLineArgs* args) 
{
	devices::instance().collectDevices();
	numDevices = devices::instance().getNumDevices();
	serialCRCs = devices::instance().getSerialCRCs();
	sdrplayDevices = devices::instance().getSdrplayDevices();

	CommState = ST_IDLE;

	pargs = args;
	init(pargs);

	thrdRx = 0;
	pthread_mutex_init(&mutex_rxThreadStarted, NULL);
	pthread_cond_init(&started_cond, NULL);

#ifdef TIME_MEAS
	Count1.LowPart = Count2.LowPart = 0;
	Count1.HighPart = Count2.HighPart = 0;
#endif
}

// called in the constructor
void sdrplay_device::init(rsp_cmdLineArgs* pargs)
{
	//From the command line
	currentFrequencyHz = pargs->Frequency;
	RequestedGain = pargs->Gain;
	currentSamplingRateHz = pargs->SamplingRate;
	bitWidth = (eBitWidth)pargs->BitWidth;
	//antenna = pargs->Antenna;
}


/// <summary>
/// Prepares the serials and hwVersion into a list, to be transmitted to the host
/// Devices must have been collected in advance.
/// </summary>
/// <param name="buf">Must be large enough</param>
/// <returns>Length of the information in the buffer</returns>
int sdrplay_device::prepareSerialsList(BYTE* buf)
{
	devices::instance().collectDevices();
	numDevices = devices::instance().getNumDevices();
	serialCRCs = devices::instance().getSerialCRCs();
	sdrplayDevices = devices::instance().getSdrplayDevices();

	cout << "Preparing device serials list." << endl;
	BYTE* p = buf;
	const int SERLEN = 64;
	for (int i = 0; i < numDevices; i++)
	{
		for (int k = 0; k < SERLEN; k++)
			*p++ = sdrplayDevices[i].SerNo[k];
		*p++ = ',';
		*p++ = sdrplayDevices[i].hwVer;
		*p++ = ';';
	}
	int len = p - buf;
	return len;
}

sdrplay_api_ErrT  sdrplay_device::selectDevice(uint32_t crc)
{
	DeviceSelected = false;
	pDevice = 0;
	sdrplay_api_DeviceT* pd = 0;
	// Lock API while device selection is performed
	sdrplay_api_LockDeviceApi();
	//collectDevices();
	////if (pargs == 0)
	//	pargs = args;
	//sdrplay_api_TunerSelectT tun = (sdrplay_api_TunerSelectT)args->Tuner;

	//if ((tun == sdrplay_api_Tuner_B) || (tun == sdrplay_api_Tuner_Both) ||
	//	(args->Master == true)) // requires RSPduo
	//{
	//	// Choose device: Algo adapted from sdrplay's example
	//	for (int i = 0; i < numDevices; i++)
	//	{
	//		// Pick first RSPduo
	//		sdrplay_api_DeviceT* pd = &sdrplayDevices[i];
	//		string devserno = string(pd->SerNo);
	//		//if (common::hasEnding(devserno, pargs->Serial) &&
	//		if (crc == serialCRCs[i] &&
	//			pd->hwVer == SDRPLAY_RSPduo_ID)
	//		{
	//			setDevice(pd);
	//			pd->rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
	//			break;
	//		}
	//	}
	//}
	//else
	{
		bool master_slave = false;

		for (int i = 0; i < numDevices; i++)
		{
			// Pick first if crc == 0
			pd = &sdrplayDevices[i];
			string devserno = string(pd->SerNo);
			if (crc == serialCRCs[i] || crc== 0 )
			{
				cout << "Device with Serial " << sdrplayDevices[i].SerNo << " selected." << endl;
				//pd->tuner = sdrplay_api_Tuner_A;
				setDevice(pd); //pDevice
				if (pd->hwVer == SDRPLAY_RSPduo_ID)
				{
					// If master device is available, select device as master
					if (pd->rspDuoMode & sdrplay_api_RspDuoMode_Master)
					{
						// Select tuner based on user input (or default to TunerA)
						pd->tuner = sdrplay_api_Tuner_A;
						//if (reqTuner == 1)
						//	chosenDevice->tuner = sdrplay_api_Tuner_B;
						// Set operating mode
						if (!master_slave) // Single tuner mode
						{
							pd->rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
						}
						else
						{
							pd->rspDuoMode = sdrplay_api_RspDuoMode_Master;
							// Need to specify sample frequency in master/slave mode
							pd->rspDuoSampleFreq = 6000000.0;
						}
					}
					else // Only slave device available
					{
						// Shouldn't change any parameters for slave device
					}
				}
				// Select chosen device
				if ((err = sdrplay_api_SelectDevice(pd)) != sdrplay_api_Success)
				{
					printf("sdrplay_api_SelectDevice failed %s\n", sdrplay_api_GetErrorString(err));
					break;
				}
				DeviceSelected = true;
				break;
			}
		}
	}
	if (pDevice == 0)
		return sdrplay_api_Fail;

	pd = pDevice;
	sdrplay_api_UnlockDeviceApi();

	if (pd->hwVer == SDRPLAY_RSP1_ID)
		rxType = RSP1;
	else if (pd->hwVer == SDRPLAY_RSP1A_ID)
		rxType = RSP1A;
	else if (pd->hwVer == SDRPLAY_RSP2_ID)
		rxType = RSP2;
	else if (pd->hwVer == SDRPLAY_RSPduo_ID)
		rxType = RSPduo;
	else if (pd->hwVer == SDRPLAY_RSPdx_ID)
		rxType = RSPdx;
	else
		rxType = UNKNOWN;

	flatGr = false;


	sdrplay_api_ErrT err;

	// Retrieve device parameters so they can be changed if wanted
	if ((err = sdrplay_api_GetDeviceParams(pd->dev, &deviceParams)) != sdrplay_api_Success)
	{
		printf("sdrplay_api_GetDeviceParams failed %s\n", sdrplay_api_GetErrorString(err));
		throw msg_exception("Error in tuner initialisation.");
	}

	cout << "Tuner " << pd->tuner << " selected";
	return sdrplay_api_Success;
}

void sdrplay_device::createCtrlThread(const char* addr, int port)
{
	std::cout << endl << "Creating ctrl thread..." << endl;
	if (thrdCtrl != 0) // just in case..
	{
		pthread_cancel(*thrdCtrl);
		delete thrdCtrl;
		thrdCtrl = 0;
	}
	ctrlThreadExitFlag = false;
	ctrlThreadData.addr = addr;
	ctrlThreadData.port = port;
	ctrlThreadData.pDoExit = &ctrlThreadExitFlag;
	ctrlThreadData.wait = 500000; //0.5s
	ctrlThreadData.dev = this;

	thrdCtrl = new pthread_t();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int res = pthread_create(thrdCtrl, &attr, &ctrl_thread_fn, &ctrlThreadData);
	pthread_attr_destroy(&attr);
}

void sdrplay_device::start(SOCKET client)
{
	remoteClient = client;

	cout << endl << "Starting..." << endl;
	// create the control thread and its socket communication
	createCtrlThread(pargs->Address.sIPAddress.c_str(), pargs->Port + 1);



	if (thrdRx != 0) // just in case..
	{
		pthread_cancel(*thrdRx);
		delete thrdRx;
		thrdRx = 0;
	}
	thrdRx = new pthread_t();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int res = pthread_create(thrdRx, &attr, &receive, this);
	pthread_attr_destroy(&attr);

	started = true;
}

void sdrplay_device::selectChannel(sdrplay_api_TunerSelectT tunerId)
{
	if (tunerId == sdrplay_api_Tuner_A)
	{
		pDevice->tuner = sdrplay_api_Tuner_A;
		pCurCh = deviceParams->rxChannelA;
	}
	else if (tunerId == sdrplay_api_Tuner_B)
	{
		pDevice->tuner = sdrplay_api_Tuner_B;
		pCurCh = deviceParams->rxChannelB;
	}
}


void sdrplay_device::cleanup()
{
	if (thrdRx != 0) // just in case..
	{
		pthread_cancel(*thrdRx);
		delete thrdRx;
		thrdRx = 0;
	}
}


void sdrplay_device::stop()
{
	sdrplay_api_ErrT err;

	if (!started)
	{
		cout << "Already Stopped. Nothing to do here.";
		return;
	}
	ctrlThreadExitFlag = true;
	cleanup();

}

void sdrplay_device::writeWelcomeString() const
{
	BYTE buf0[] = "RTL0";
	BYTE* buf = new BYTE[c_welcomeMessageLength];
	memset(buf, 0, c_welcomeMessageLength);
	memcpy(buf, buf0, 4);
	buf[6] = bitWidth;
	buf[7] = rxType+7;	//7:RSP1, 8: RSP1A, 9: RSP2, 10:RSPduo
	buf[11] = 0;// gainConfiguration::GAIN_STEPS;
	buf[15] = 0x52; buf[16] = 0x53; buf[17] = 0x50; buf[18] = (int)rxType + 0x30; //"RSP2", interpreted e.g. by qirx
	send(remoteClient, (const char*)buf, c_welcomeMessageLength, 0);
	delete[] buf;
}

int sdrplay_device::getRxString(char* s) const
{
	s[3] = (BYTE)rxType + 0x30;
	return 4;
}

/// <summary>
/// Total gain
/// </summary>
/// <returns>current gain</returns>
/// <remark>running in the context of the controlThread</remark>
sdrplay_api_GainValuesT* sdrplay_device::getGainValues()
{
	if (!started)
		return 0;
	//sdrplay_api_GainValuesT gainVals;
	if (getDevice()->tuner == sdrplay_api_Tuner_A || getDevice()->tuner == sdrplay_api_Tuner_B)
	{
		memcpy(&GainValues, &pCurCh->tunerParams.gain.gainVals, sizeof(GainValues));
	}
	else //TODO sdrplay_api_Tuner_Both
		return 0;

	return &GainValues;
}

BYTE* sdrplay_device::mergeIQ(const short* idata, const short* qdata, int samplesPerPacket, int& buflen)
{
	BYTE* buf = 0;
	buflen = 0;

	if (bitWidth == BITS_16)
	{
		buflen = samplesPerPacket *4;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			buf[j++] = (BYTE)(idata[i] & 0xff);			
			buf[j++] = (BYTE)((idata[i] & 0xff00) >> 8);  

			buf[j++] = (BYTE)(qdata[i] & 0xff);			
			buf[j++] = (BYTE)((qdata[i] & 0xff00) >> 8); 
		}
	}
	else if (bitWidth == BITS_8)
	{
		// assume the 12 Bit ADC values are mapped onto signed 16-Bit values covering the whole range
		buflen = samplesPerPacket * 2;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			//restore the unsigned 12-Bit signal
			int tmpi = (idata[i] >> 4) + 2048;
			int tmpq = (qdata[i] >> 4) + 2048;

			// cut the four low order bits
			tmpi >>= 4;
			tmpq >>= 4;

			buf[j++] = (BYTE)tmpi;
			buf[j++] = (BYTE)tmpq;
			//buf[j++] = (BYTE)(idata[i] / 64 + 127);
			//buf[j++] = (BYTE)(qdata[i] / 64 + 127);
		}
	}
	else if (bitWidth == BITS_4)
	{
		buflen = samplesPerPacket * 1;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			//I-Byte
			byte b = (BYTE)(idata[i] / 64 + 127);
			// Low order nibble
			byte b2 = b >> 4;
			b2 &= 0x0f;
				
			//Q-Byte
			b = (BYTE)(qdata[i] / 64 + 127);
			// High order nibble
			byte b3 = b & 0xf0;

			byte b4 = b2 | b3;
			buf[j++] = b4;
		}
	}
	return buf;
}
void eventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, 
	sdrplay_api_EventParamsT *params, void *cbContext)
{
	if ( exitRequest)
		return;

	sdrplay_device* md = (sdrplay_device*)cbContext;

	int masterInitialised = 0;
	int slaveUninitialised = 0;

	switch (eventId)
	{
	case sdrplay_api_GainChange:
		//printf("sdrplay_api_EventCb: %s, tuner=%s gRdB=%d lnaGRdB=%d systemGain=%.2f\n",
		//	"sdrplay_api_GainChange", (tuner == sdrplay_api_Tuner_A) ? "sdrplay_api_Tuner_A" :
		//	"sdrplay_api_Tuner_B", params->gainParams.gRdB, params->gainParams.lnaGRdB,
		//	params->gainParams.currGain);
		break;

	case sdrplay_api_PowerOverloadChange:

		if (tuner == sdrplay_api_Tuner_A && params->powerOverloadParams.powerOverloadChangeType ==
			sdrplay_api_Overload_Detected)
		{
			md->overloaded_A = true;
			int gr = md->pCurCh->tunerParams.gain.gRdB;
			int lnastate = md->pCurCh->tunerParams.gain.LNAstate;
			cout << "Overload detected on tuner A with lnastate " << lnastate << " and grdB: " << gr << endl;
			//md->increaseLNAstate_A(2);
		}
		else if (tuner == sdrplay_api_Tuner_A && params->powerOverloadParams.powerOverloadChangeType ==
			sdrplay_api_Overload_Corrected)
		{
			md->overloaded_A = false;
			cout << "Overload corrected on tuner A" << endl;
			//md->deviceParams->rxChannelB->tunerParams.gain.gRdB +=1;
		}
		else if (tuner == sdrplay_api_Tuner_B && params->powerOverloadParams.powerOverloadChangeType ==
			sdrplay_api_Overload_Detected)
		{
			md->overloaded_B = true;
			cout << "Overload detected on tuner B" << endl;
		}

		else if (tuner == sdrplay_api_Tuner_B && params->powerOverloadParams.powerOverloadChangeType ==
			sdrplay_api_Overload_Corrected)
		{
			md->overloaded_B = false;
			cout << "Overload corrected on tuner B" << endl;
		}

		// Send update message to acknowledge power overload message received
		sdrplay_api_Update(md->pDevice->dev, tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck,
			sdrplay_api_Update_Ext1_None);
		break;

	case sdrplay_api_RspDuoModeChange:
		printf("sdrplay_api_EventCb: %s, tuner=%s modeChangeType=%s\n",
			"sdrplay_api_RspDuoModeChange", (tuner == sdrplay_api_Tuner_A) ?
			"sdrplay_api_Tuner_A" : "sdrplay_api_Tuner_B",
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised) ?
			"sdrplay_api_MasterInitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveAttached) ?
			"sdrplay_api_SlaveAttached" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDetached) ?
			"sdrplay_api_SlaveDetached" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveInitialised) ?
			"sdrplay_api_SlaveInitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised) ?
			"sdrplay_api_SlaveUninitialised" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterDllDisappeared) ?
			"sdrplay_api_MasterDllDisappeared" :
			(params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDllDisappeared) ?
			"sdrplay_api_SlaveDllDisappeared" : "unknown type");

		if (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised)
			masterInitialised = 1;
		if (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised)
			slaveUninitialised = 1;
		break;
	case sdrplay_api_DeviceRemoved:
		printf("sdrplay_api_EventCb: %s\n", "sdrplay_api_DeviceRemoved");
		devices::instance().CloseClient();
		break;
	default:
		printf("sdrplay_api_EventCb: %d, unknown event\n", eventId);
		break;
	}
}

void streamACallback(short* xi, short* xq, sdrplay_api_StreamCbParamsT* params,
	unsigned int numSamples, unsigned int reset, void* cbContext)
{
	if (exitRequest)
		return;
	if (reset)
		printf("sdrplay_api_StreamACallback: numSamples=%d\n", numSamples);
	
	streamCallback(xi, xq, params, numSamples, reset, cbContext);
}

void streamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
	unsigned int numSamples, unsigned int reset, void *cbContext)
{
	if (exitRequest)
		return;
	if (reset)
		printf("sdrplay_api_StreamBCallback: numSamples=%d\n", numSamples);
	// Process stream callback data here - this callback will only be used in dual tuner mode
		streamCallback(xi, xq, params, numSamples, reset, cbContext);
	return;
}

void streamCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, 
	unsigned int numSamples, unsigned int reset, void *cbContext)
{

	static int count = 0;

	sdrplay_device* md = (sdrplay_device*)cbContext;
	try
	{
		//In case of a socket error, don't process the data for one second
		if (md->cbksPerSecond-- > 0)
		{
			if (!md->cbkTimerStarted)
			{
				md->cbkTimerStarted = true;
				cout << "Discarding samples for one second\n";
			}
			return;
		}
		else if (md->cbkTimerStarted)
		{
			md->cbkTimerStarted = false;
		} 
		if (md->remoteClient == INVALID_SOCKET)
			return;


		if (params->grChanged)
			;

#ifdef TIME_MEAS
		count++;
		if (count % 1000 == 0)
		{
			QueryPerformanceCounter(&Count2);
			CMeasTimeDiff::formattedTimeOutput ("1000 sdrplay buffers (ms)", CMeasTimeDiff::calcTimeDiff_in_ms (Count2, Count1));
			QueryPerformanceCounter(&Count1);
		}
#endif
		int buflen = 0;
		BYTE* buf = md->mergeIQ(xi, xq, numSamples, buflen);
		int remaining = buflen;
		int sent = 0;
		while (remaining > 0)
		{
			fd_set writefds;
			struct timeval tv= {1,0};
			FD_ZERO(&writefds);
			FD_SET(md->remoteClient, &writefds);
			int res  = select(md->remoteClient+1, NULL, &writefds, NULL, &tv);
			if (res > 0)
			{
				sent = send(md->remoteClient, (const char*)buf + (buflen - remaining), remaining, 0);
				remaining -= sent;
			}
			else
			{
				md->cbksPerSecond = md->currentSamplingRateHz / numSamples; //1 sec "timer" in the error case. assumed this does not change frequently
				delete[] buf;
				throw msg_exception("socket error " + to_string(errno));
			}
		}
		delete[] buf;
	}
	catch (exception& e)
	{
		cout << "Error in streaming callback :" << e.what() <<  endl;
	}
}

sdrplay_api_ErrT sdrplay_device::createChannels()
{
	Initialized = false;
	sdrplay_api_DeviceT* pd = pDevice;
	//pd->rspDuoSampleFreq = currentSamplingRateHz;
	selectChannel(sdrplay_api_Tuner_A);

	if (!RSPGainValuesFromRequestedGain(RequestedGain, rxType, LNAstate, gainReduction))
	{
		cout << "\nCannot retrieve LNA state and Gain Reduction from requested gain value " << RequestedGain << endl;
		cout << "Program cannot continue" << endl;
		return sdrplay_api_Fail;
	}
	cout << "\n Using LNA State: " << LNAstate << endl;
	cout << " Using Gain Reduction: " << gainReduction << endl;

	// next doesn't work in master/slave mode, 
	pCurCh->tunerParams.ifType = sdrplay_api_IF_Zero;
	//pCurCh->tunerParams.ifType = sdrplay_api_IF_0_450;

	deviceParams->devParams->fsFreq.fsHz = currentSamplingRateHz; // initially set in init
	int ix = getSamplingConfigurationTableIndex(currentSamplingRateHz);
	if (ix < 0)
	{
		cout << "Invalid sampling rate: " << currentSamplingRateHz << endl;
		return sdrplay_api_InvalidParam;
	}

	//// next doesn't work in master/slave mode, 
	pCurCh->tunerParams.bwType = samplingConfigs[ix].bandwidth;
	byte decimationFactor = samplingConfigs[ix].decimationFactor;
	pCurCh->ctrlParams.decimation.decimationFactor = decimationFactor;
	pCurCh->ctrlParams.decimation.enable = decimationFactor == 1 ? 0 : 1;
	//pCurCh->ctrlParams.decimation.wideBandSignal = currentSamplingRateHz == 2000000 ? 1 : 0;
	pCurCh->tunerParams.rfFreq.rfHz = 222064000;

	pCurCh->ctrlParams.agc.setPoint_dBfs = -60;
	pCurCh->ctrlParams.agc.enable = sdrplay_api_AGC_5HZ;
	pCurCh->tunerParams.gain.gRdB = gainReduction;
	pCurCh->tunerParams.gain.LNAstate = LNAstate;

	// next doesn't work in master/slave mode, 
	//deviceParams->rxChannelB->tunerParams.ifType = sdrplay_api_IF_Zero;
	//deviceParams->rxChannelB->tunerParams.bwType= samplingConfigs[2].bandwidth ;
	//deviceParams->rxChannelB->tunerParams.rfFreq.rfHz = 222064000;
	//				   
	//deviceParams->rxChannelB->ctrlParams.agc.setPoint_dBfs = -60;
	//deviceParams->rxChannelB->ctrlParams.agc.enable = sdrplay_api_AGC_5HZ;
	//deviceParams->rxChannelB->tunerParams.gain.gRdB = gainReduction;
	//deviceParams->rxChannelB->tunerParams.gain.LNAstate = LNAstate;
	//deviceParams->rxChannelA->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;

	cbFns.StreamACbFn = streamACallback;
	cbFns.StreamBCbFn = streamBCallback;
	cbFns.EventCbFn = eventCallback;

	sdrplay_api_ErrT errInit = sdrplay_api_Init(pDevice->dev, &cbFns, this);
	cout << "\nsdrplay_api_StreamInit returned with: " << errInit << endl;
	if (errInit == sdrplay_api_Success)
		Initialized = true;
	//double fs = deviceParams->devParams->fsFreq.fsHz;
	return errInit;
}

void gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void* cbContext)
{
	// do nothing...
}


sdrplay_api_ErrT sdrplay_device::setFrequency(int valueHz)
{
	pCurCh->tunerParams.rfFreq.rfHz = valueHz;

	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
		sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);

	cout << "\nsdrplay_api_Update_Tuner_Frf returned with: " << err << endl;
	if (err != sdrplay_api_Success)
	{
		cout << "*** Error on Frequency setting: " << sdrplay_api_GetErrorString(err) << endl;
		cout << "Requested Frequency was: " << +valueHz << endl;
	}
	else
	{
		currentFrequencyHz = valueHz;
		cout << "Frequency set to (Hz): " << valueHz << endl;
	}
	return err;
}
sdrplay_api_ErrT sdrplay_device::setGain(int value)
{
	err = sdrplay_api_Success;

	//if (AGC_A == true)
	//{
	//	int curGainValue = (int)(getGainValues() + 0.5f);
	//	byte lnastate;
	//	if (value > curGainValue)
	//	{
	//		// decrease lna state
	//		lnastate = deviceParams->rxChannelA->tunerParams.gain.LNAstate;
	//		LNAstate = lnastate - 1;
	//		if (VERBOSE)
	//			cout << "Requested gain of " << value << " set, GR: " << value << " , LNAState: " << LNAstate << endl;
	//		deviceParams->rxChannelA->tunerParams.gain.LNAstate = (lnastate - 1) > 0 ? lnastate - 1 : 0;
	//	}
	//	else if (value < curGainValue)
	//	{
	//		// increase lna state
	//		lnastate = deviceParams->rxChannelA->tunerParams.gain.LNAstate;
	//		LNAstate = lnastate + 1;
	//		if (VERBOSE)
	//			cout << "Requested gain of " << value << " set, GR: " << value << " , LNAState: " << (int)lnastate << endl;
	//		deviceParams->rxChannelA->tunerParams.gain.LNAstate = (lnastate - 1) < 7 ? lnastate + 1 : 7;
	//	}
	//	else // value didn't change
	//		return err;
	//	curGainValue = value;
	//	LNAstate = lnastate;

	//	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//		sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);

	//	cout << "\nsdrplay_api_Update_Tuner_GainReduction returned with: " << err << endl;
	//	if (err != sdrplay_api_Success)
	//	{
	//		cout << "*** Error on Gain Reduction setting: " << sdrplay_api_GetErrorString(err) << endl;
	//		cout << "Requested gain was: " << value << /*" , GR: " << gainReduction <<*/  " , lnastate: " << LNAstate << endl;
	//	}
	//	else
	//	{
	//		LNAstate = deviceParams->rxChannelA->tunerParams.gain.LNAstate;
	//		int gr = deviceParams->rxChannelA->tunerParams.gain.gRdB;
	//		cout << "Result was GR: " << gr <<" , LNAState: " << LNAstate << endl;
	//	}
	//}
	if (AGC_A == false)
	{
		sdrplay_api_ErrT err;
		//cout << " Gain value requested: "<< value<< endl;

		int lnastate = 0;
		int gr = 0;

		// map GAIN_STEPS into 20 .. 59 dB
		double steps = (double)gainConfiguration::GAIN_STEPS;
		double grMax = 59.0;
		double grMin = 20.0;
		double grunit = (grMax - grMin) / steps;
		double gred = (steps - value); // value is max gain, NOT gr
		double dgr = grMin + gred * grunit;
		gr = gainReduction = (int)(dgr + 0.5);

		pCurCh->tunerParams.gain.gRdB = gainReduction;
		err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
			sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
		if (VERBOSE)
		{
			cout << "Requested Gain of " << value << "   resulted in " << gr << " dB gain reduction." << endl;
		}
		return err;
	}

	//	if (RSPGainValuesFromRequestedGain(value, rxType, lnastate, gr))
	//	{
	//		if (gr < 20)
	//			gr = 20;
	//		if (gr > 59)
	//			gr = 59;
	//		gainReduction = gr;
	//		LNAstate = lnastate;
	//		deviceParams->rxChannelA->tunerParams.gain.LNAstate = lnastate;
	//		if (lnastate != (int(deviceParams->rxChannelA->tunerParams.gain.LNAstate)))
	//		{
	//			err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//				sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
	//			if (VERBOSE)
	//			{
	//				cout << "Set Lnastate returned " << err << " with requested value: " << (int)(deviceParams->rxChannelA->tunerParams.gain.LNAstate) << endl;
	//				cout << "LNA State: " << lnastate << endl;
	//			}
	//		}

	//		deviceParams->rxChannelA->tunerParams.gain.gRdB = gainReduction;
	//		err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//			sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);

	//		if (VERBOSE)
	//		{
	//			cout << "Set Gain reduction returned " << err << " with requested value: " << gainReduction << endl;
	//			cout << "Gr value: " << gr << endl;
	//		}
	//		return err;
	//	}
	//	else
	//	{
	//		cout << "RSPGainValuesFromRequestedGain FAILED for the requested value: " << gainConfiguration::GAIN_STEPS - value << endl;
	//		return (sdrplay_api_ErrT)-1;
	//	}

	//	if (VERBOSE)
	//		cout << "\Set Gr and LNA state returned with: " << err << endl;

	//	if (err != sdrplay_api_Success)
	//	{
	//		cout << "Set Gr and LNA state  FAILED with requested value: " << gainConfiguration::GAIN_STEPS - value << endl;
	//	}
	//	else if (VERBOSE)
	//		cout << "Set Gr and LNA state  succeeded with requested value: " << gainConfiguration::GAIN_STEPS - value << endl;

	//	return err;
	//}

	//return err;

	//if (overloaded_A)
	//{
	//	//if (deviceParams->rxChannelA->tunerParams.gain.gRdB < 45)
	//	//	deviceParams->rxChannelA->tunerParams.gain.gRdB += 10;
	//	//else
	//	//	deviceParams->rxChannelA->tunerParams.gain.LNAstate += 1;
	//	//err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//	//	sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);

	//	return err;
	//}
	//else
	//	return sdrplay_api_Success;

	//lnastate = 0;
	//int gr = 0;

	//bool res = RSPGainValuesFromRequestedGain(value, rxType, lnastate, gr);
	//if (!res)
	//{
	//	cout << "*** Error: RSPGainValuesFromRequestedGain for " << value << " failed";
	//	return err;
	//}

	//LNAstate = lnastate;
	//gainReduction = gr;

	//deviceParams->rxChannelA->tunerParams.gain.gRdB = gr;
	//deviceParams->rxChannelA->tunerParams.gain.LNAstate = LNAstate;

	//err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//	sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);

	//cout << "\nsdrplay_api_Update_Tuner_GainReduction returned with: " << err << endl;
	//if (err != sdrplay_api_Success)
	//{
	//	cout << "*** Error on Gain Reduction setting: " << sdrplay_api_GetErrorString(err) << endl;
	//	cout << "Requested gain was: " << value << " , GR: " << gainReduction << /* " , LNAState: " << LNAstate <<*/ endl;
	//}
	//else
	//{
	//	LNAstate = deviceParams->rxChannelA->tunerParams.gain.LNAstate;
	//	cout << "Requested gain of " << value << " set, GR: " << gainReduction << " , LNAState: " << LNAstate << endl;
	//}

	//return err;
}
sdrplay_api_ErrT sdrplay_device::setAGC(bool on)
{
	sdrplay_api_ErrT err = sdrplay_api_Success;
	if (on == false)
	{
		AGC_A = false;
		pCurCh->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
		cout << "\nAGC OFF returned with: " << err << endl;
	}
	else
	{
		AGC_A = true;
		// enable AGC with a setPoint of -15dBfs //optimum for DAB
		pCurCh->ctrlParams.agc.setPoint_dBfs = -15;// -60;
		pCurCh->ctrlParams.agc.enable = sdrplay_api_AGC_5HZ;
		cout << "\nsdrplay_api_AgcControl 50Hz, " << agcPoint_dBfs << " dBfs returned with: " << err << endl;
	}
	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
		sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);

	if (err != sdrplay_api_Success)
	{
		cout << "SetAGC failed." << endl;
	}

	return err;
}

sdrplay_api_ErrT sdrplay_device::setLNAState(int value)
{
	t_freqBand band = gainConfiguration::BandIndexFromHz(currentFrequencyHz);
	if (band == Band_Invalid)
		return sdrplay_api_OutOfRange;
	gainConfiguration gcfg(band);

	int lnaStates = gcfg.LNAstates[rxType][band];

	if (value < 0 || value >= lnaStates)
	{
		cout << "***Error in setLNAState. state #" << value << " requested, but "<< lnaStates << " available." << endl;
		return sdrplay_api_OutOfRange;
	}
	if ( gcfg.IsGrInvalid(rxType, value, band))
	{
		cout << "***Error in setLNAState. state #" << value << " requested, but "<< lnaStates << " is invalid." << endl;
		return sdrplay_api_OutOfRange;
	}

	int lnastate = pCurCh->tunerParams.gain.LNAstate;
	if (value == lnastate)
		return sdrplay_api_Success;

	pCurCh->tunerParams.gain.LNAstate = (byte)value;
	sdrplay_api_ErrT err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
		sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
	lnastate = pCurCh->tunerParams.gain.LNAstate;

	cout << "New lnastate returned with " << err << " and new lnastate " << lnastate << endl;
	return err;
}

int sdrplay_device::getLNAState()
{
	if (pCurCh == 0)
		return -1;
	return pCurCh->tunerParams.gain.LNAstate;
}

sdrplay_api_ErrT sdrplay_device::setSamplingRate(int requestedSrHz)
{
	cout << "New Samplingrate requested: " << requestedSrHz << endl;

	if (Initialized)
	{
		sdrplay_api_ErrT err = sdrplay_api_Uninit(pDevice->dev);
		if (err != sdrplay_api_Success)
		{
			cout << "***Uninitialize failed!" << endl;
			return err;
		}
		else
			cout << "Uninitialize successful!" << endl;
	}
	int ix = getSamplingConfigurationTableIndex(requestedSrHz);
	if (ix >= 0)
	{
		currentSamplingRateHz = requestedSrHz;
		err = createChannels();
		return err;
	}
	else
	{

	}
	return err;
}

/// <summary>
/// Gets the config table index for a requested sampling rate
/// </summary>
/// <param name="requestedSrHz">Requested sampling rate in Hz</param>
/// <returns>Index into the samplingConfigs table, -1 if not found</returns>
int sdrplay_device::getSamplingConfigurationTableIndex(int requestedSrHz)
{
	for (int i = 0; i < c_numSamplingConfigs; i++)
	{
		samplingConfiguration sc = samplingConfigs[i];
		if (requestedSrHz == sc.samplingRateHz)
		{
			return i;
		}
	}
	printf("Sampling Rate: %d; Should be %d or %d or %d or %d or %d\n", requestedSrHz, samplingConfigs[0].samplingRateHz,
		samplingConfigs[1].samplingRateHz, samplingConfigs[2].samplingRateHz, samplingConfigs[3].samplingRateHz, samplingConfigs[4].samplingRateHz);
	printf("Sampling Rate: %d Hz will be tried\n", requestedSrHz);

	return -1;
}

//value is correction in ppm
sdrplay_api_ErrT sdrplay_device::setFrequencyCorrection(int value)
{
	deviceParams->devParams->ppm = value;

	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
		sdrplay_api_Update_Dev_Ppm, sdrplay_api_Update_Ext1_None);

	if (err != sdrplay_api_Success)
		cout << "PPM setting error: " << err << endl;
	else
	cout << "\nsdrplay_api_SetPpm returned with: " << err << endl;
		cout << "PPM correction: " << value << endl;
	return err;
}


//value is correction in ppm*100
sdrplay_api_ErrT sdrplay_device::setFrequencyCorrection100(int value)
{
	double valPpm = (double)(value / 100.0);
	deviceParams->devParams->ppm = valPpm;

	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
		sdrplay_api_Update_Dev_Ppm, sdrplay_api_Update_Ext1_None);

	cout << "\nsdrplay_api_SetPpm returned with: " << err << endl;
	if (err != sdrplay_api_Success)
	{
		cout << "PPM setting error: " << err << endl;
		return err;
	}
	else
		cout << "PPM correction: " << valPpm << endl;


	//// first undo the old samplingrate delta
	//if (oldDeltaSrHz != 0)
	//{
	//	deviceParams->devParams->fsFreq..fsHz = currentSamplingRateHz; // initially set in init

	//	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
	//		sdrplay_api_Update_Tuner_Frf, sdrplay_api_Update_Ext1_None);

	//	err = sdrplay_api_SetFs(-oldDeltaSrHz, 0, 0, 0);
	//	cout << "\nsdrplay_api_SetFs returned with: " << err << endl;
	//	if (err != sdrplay_api_Success)
	//	{
	//		cout << "Undo old Sampling rate delta setting error: " << err << endl;
	//	}
	//	else
	//		cout << "old Sampling rate correction undone: " << -oldDeltaSrHz << "Hz" << endl << endl;
	//}
}



sdrplay_api_ErrT sdrplay_device::setBiasT(int value)
{
	sdrplay_api_ErrT err = sdrplay_api_Success;
	switch (rxType)
	{
		case RSP1A:
			pCurCh->rsp1aTunerParams.biasTEnable = value;
			err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
				sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);
			break;
		case RSP2:
			pCurCh->rsp2TunerParams.biasTEnable = value;
			err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
				sdrplay_api_Update_Rsp2_BiasTControl, sdrplay_api_Update_Ext1_None);
			break;
		case RSPduo:
			pCurCh->rspDuoTunerParams.biasTEnable = value;
			err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
				sdrplay_api_Update_RspDuo_BiasTControl, sdrplay_api_Update_Ext1_None);
			break;
		default:
			break;
	}

	cout << "\nsdrplay_api_xxx_BiasT returned with: " << err << endl;
	if (err != sdrplay_api_Success)
		cout << "BiasT setting error: " << err << endl;
	else
		cout << "BiasT setting: " << value << endl;
	return err;
}
sdrplay_api_ErrT sdrplay_device::setAdsbMode()
{
	pCurCh->ctrlParams.adsbMode = sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ;
	err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
			sdrplay_api_Update_Ctrl_AdsbMode, sdrplay_api_Update_Ext1_None);

	cout << "\nSet ADSB mode returned with: " << err << endl;
	if (err != sdrplay_api_Success)
		cout << "ADSB mode  error: " << err << endl;
	return err;
}
sdrplay_api_ErrT sdrplay_device::setAntenna(int value)
{
	switch (rxType)
	{
		case RSP1A:
			err = sdrplay_api_InvalidParam;
			break;
		case RSP2:
			pCurCh->rsp2TunerParams.antennaSel = (sdrplay_api_Rsp2_AntennaSelectT)value;
			err = sdrplay_api_Update(pDevice->dev, pDevice->tuner,
				sdrplay_api_Update_Rsp2_AntennaControl, sdrplay_api_Update_Ext1_None);
			break;
		case RSPduo:
			if (value == 5) //Tuner A requested
			{
				if (pDevice->tuner != sdrplay_api_Tuner_A)
				{
					err = sdrplay_api_SwapRspDuoActiveTuner(pDevice->dev, &pDevice->tuner, sdrplay_api_RspDuo_AMPORT_2);// , sdrplay_api_RspDuo_AMPORT_1);
					if (err == sdrplay_api_Success)
						selectChannel(sdrplay_api_Tuner_A);
				}
			}
			else if (value == 6) //Tuner B requested
			{
				if (pDevice->tuner != sdrplay_api_Tuner_B)
					err = sdrplay_api_SwapRspDuoActiveTuner(pDevice->dev, &pDevice->tuner, sdrplay_api_RspDuo_AMPORT_1);// , sdrplay_api_RspDuo_AMPORT_1);
					if (err == sdrplay_api_Success)
						selectChannel(sdrplay_api_Tuner_B);
			}
			break;
		default:
			err = sdrplay_api_InvalidParam;
			break;
	}

	cout << "\nAntenna control  with: " << err << endl;
	if (err != sdrplay_api_Success)
	{
		cout << "Antenna control setting error: " << err << endl;
		return err;
	}
	else
		cout <<" Antenna control succeeded: " << value << endl;
	return err;
}


bool sdrplay_device::RSPGainValuesFromRequestedGain(int flatValue, int rxtype, int& LNAstate, int& gr)
{
	if (flatGr)
		return false;

	t_freqBand band = gainConfiguration::BandIndexFromHz(currentFrequencyHz);
	if (band == Band_Invalid)
		return false;


	gainConfiguration gcfg(band);

	if (gcfg.calculateGrValues(flatValue, rxType, LNAstate, gr))
	{
		return true;
	}
	return false;
}



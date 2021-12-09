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
#include "sdrplay_device.h"
#include "sdrGainTable.h"
//#include "MeasTimeDiff.h"
#include <iostream>
using namespace std;

uint8_t getCommandAndValue(char* rxBuf, int& value)
{
	BYTE valbuf[4];
	int cmd = rxBuf[0];
	memcpy(valbuf, rxBuf + 1, 4);

	if (common::isLittleEndian())
		value = valbuf[3] + valbuf[2] * 0x100 + valbuf[1] * 0x10000 + valbuf[0] * 0x1000000;
	else
		value = valbuf[0] + valbuf[1] * 0x100 + valbuf[2] * 0x10000 + valbuf[3] * 0x1000000;
	return cmd;
}


/// <summary>
/// Receive thread, to process commands
/// </summary>
void* receive(void* p)
{
	sdrplay_api_ErrT err;
	sdrplay_device* md = (sdrplay_device*)p;
	cout << "**** receive thread entered.   *****" << endl;

	try
	{
		// preliminary
		md->writeWelcomeString();

		char rxBuf[16];
		for (;;)
		{
			const int cmd_length = 5;
			memset(rxBuf, 0, 16);
			int remaining = cmd_length;

			while (remaining > 0)
			{
				int rcvd = recv(md->remoteClient, rxBuf + (cmd_length - remaining), remaining, 0); //read 5 bytes (cmd + value)
				remaining -= rcvd;

				if (rcvd == 0)
				{
					err = sdrplay_api_Uninit(md->pDevice->dev);
					cout << "sdrplay_api_Uninit returned with: " << err << endl;
					throw msg_exception("Socket closed");
				}
				if (rcvd == SOCKET_ERROR)
				{
					err = sdrplay_api_Uninit(md->pDevice->dev);
					cout << "sdrplay_api_Uninit returned with: " << err << endl;
					throw msg_exception("Socket error");
				}
			}

			int value = 0; // out parameter
			uint8_t cmd = getCommandAndValue(rxBuf, value);

			// The ids of the commands are defined in rtl_tcp, the names had been inserted here
			// for better readability
			//int gain = md->RequestedGain;
			switch (cmd)
			{
				// First comman
			case sdrplay_device::CMD_SET_RSP_REQUEST_ALL_SERIALS: //select hardware, 1st command to receive
				md->CommState = ST_SERIALS_REQUESTED;
				break;

				// Second command
			case sdrplay_device::CMD_SET_RSP_SELECT_SERIAL: //select hardware, 1st command to receive
				md->selectDevice(value);
				md->createChannels();
				if (md->Initialized)
					md->CommState = ST_DEVICE_CREATED;
				break;

			case sdrplay_device::CMD_SET_FREQUENCY: //set frequency
													//value is freq in Hz
				err = md->setFrequency(value);
				break;

			case (int)sdrplay_device::CMD_SET_SAMPLINGRATE:
				err = md->setSamplingRate(value);//value is sr in Hz
				break;

			case (int)sdrplay_device::CMD_SET_FREQUENCYCORRECTION: //value is ppm correction
				md->setFrequencyCorrection(value);
				break;

			case (int)sdrplay_device::CMD_SET_FREQUENCYCORRECTION_PPM100: //value is ppm*100 correction
				md->setFrequencyCorrection100(value);
				break;

			case (int)sdrplay_device::CMD_SET_TUNER_GAIN_BY_INDEX:
				//value is gain value between 0 and 100
				err = md->setGain(value);
				break;

			case (int)sdrplay_device::CMD_SET_AGC_MODE:
				err = md->setAGC(value != 0);
				break;

			case (int)sdrplay_device::CMD_SET_BIAS_T:
				//err = md->setBiasT(value != 0);
				err = md->setAdsbMode();
				break;

			case (int)sdrplay_device::CMD_SET_RSP2_ANTENNA_CONTROL:
				md->setAntenna(value);
				break;

			case (int)sdrplay_device::CMD_SET_RSP_LNA_STATE:
				md->setLNAState(value);
				break;
			default:
				printf("Unknown Command; 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4]);
				break;
			}
		}
	}
	catch (exception& e)
	{
		cout << "*** Error in receive :" << e.what() << endl;
		//err = sdrplay_api_StreamUninit();
		//cout << "sdrplay_api_StreamUninit returned with: " << err << endl;
		//if (err == sdrplay_api_Success)
		//	cout << "StreamUnInit successful(0)" << endl;
		//else
		//	cout << "StreamUnInit failed (1) with " << err << endl;
	}
	err = sdrplay_api_ReleaseDevice(md->pDevice);
	if (err == sdrplay_api_Success)
	{
		cout << "Device " << md->rxType << " released" << endl;
		md->CommState = ST_DEVICE_RELEASED;
		usleep(1000000);
	}
	else
		cout << "*** Error on releasing device: " << sdrplay_api_GetErrorString(err) << endl;

	cout << "**** Rx thread terminating. ****" << endl;
	return 0;
}


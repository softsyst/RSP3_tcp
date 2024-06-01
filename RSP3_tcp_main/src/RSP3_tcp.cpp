/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017-2023 Clem Schmidt, softsyst GmbH, http://www.softsyst.com
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

/**
** The interface and the sdrplay API Library is Copyright SDRplay Ltd.
** For the sdrplay Copyright, see the file 

       SDRplay_RSP_API_Release_Notes_V3.07.pdf

**/

#include <iostream>
#include <stdio.h>
#include "rsp_tcp.h"
#include "rsp_cmdLineArgs.h"
#include "devices.h"
#include "sdrGainTable.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include <stdio.h>
//#include <conio.h>
#include <signal.h>

using namespace std;

// From RSP2_tcp
// V0.9.8	Sampling rate 2.000 for ADS-B
// V0.9.9	Bitwidth 8 Bit corrected
// V0.3.4	API 2.10
// V0.3.5	Timeout rx thread reduced from 1s to 50ms
// V0.3.6	Linux mode (QIRX only, with device selection)
// V0.3.7   Commandline -B 1 for basic mode compatible with SDR# and DAB player
// V0.3.8   Commandline -L <x> for LNA state 0 <= x <= 15
// V0.3.9   Notch filter for DAB, WFM, AM
// V0.3.10  Notch filter and antenna states sent back to host
// V0.3.11  sdrplay API 3.15
// V0.3.12  Corrections for RSP1B, RSPdxR2
// V0.3.13  RSPdxR2 tested
string Version = "0.3.13";

bool exitRequest = false;
pthread_mutex_t stateLock;

map<eErrors, string> returnErrorStrings =
{
	 { E_OK, "OK"}
    ,{ E_PARAMETER, "Starting Parameter Error"}
	,{ E_WRONG_API_VERSION, "Wrong or incompatible sdrplay API Version. Must be at least 3.07."}
	,{ E_NO_DEVICE, "No sdrplay device found."}
	,{ E_DEVICE_INDEX, "Requested Device Index not present: "}
	,{ E_WIN_WSA_STARTUP, "WSAStartup failed with error:  "}
};

map<int, string> deviceNameByType =
{
	 { SDRPLAY_RSP1_ID, "RSP1"}
    ,{ SDRPLAY_RSP1A_ID, "RSP1A"}
	,{ SDRPLAY_RSP2_ID, "RSP2"}
	,{ SDRPLAY_RSPduo_ID, "RSPduo"}
	,{ SDRPLAY_RSPdx_ID, "RSPdx"}
	,{ SDRPLAY_RSP1B_ID, "RSP1B"}
	,{ SDRPLAY_RSPdxR2_ID, "RSPdxR2"}
};

#ifdef _WIN32
BOOL WINAPI CtrlHandler(int n_signal)
{
	if (n_signal == CTRL_C_EVENT || n_signal == CTRL_CLOSE_EVENT)
	{
		printf("Exit request\n");
		exitRequest = true;
		devices::instance().Stop();
		return TRUE;
	}
	return FALSE;
}
#else
static void sighandler(int signum)
{
	printf("Signal (%d) caught, ask for exit!\n", signum);
	exitRequest = true;
	devices::instance().Stop();
}
#endif

int main(int argc, char* argv[])
{
	sdrplay_api_ErrT err;

	rsp_cmdLineArgs* pargs = 0;
	eErrors retCode = E_OK;
	string sError;
	bool res = false;
	float apiVersion = 0.0f;
	double epsilon = 0.0001;

#ifdef _WIN32
	WSADATA wsd;
	int result = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (result != 0)
	{
		retCode = E_WIN_WSA_STARTUP;
		sError = returnErrorStrings[retCode] + to_string(result);
		goto exitapp;
	}
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
		std::cout << "Error setting ctrl-c handler." << endl;
#else
	struct sigaction sigact, sigign;
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigign.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigign, NULL);
#endif

	std::cout << "\nRSP_tcp V" + Version << std::endl;
	std::cout << "Copyright (c) Clem Schmidt, qirx.softsyst.com. All rights reserved" << endl;
	std::cout << endl;

	pargs = new rsp_cmdLineArgs(argc, argv);
	if (pargs->parse() != 0)
	{
		pargs->displayUsage();
		retCode = E_PARAMETER;
		sError = returnErrorStrings[retCode];
		goto exitapp;
	}
	std::cout << "IP Address = " + pargs->Address.sIPAddress << endl;
	std::cout << "Port Number = " + to_string(pargs->Port) << endl;
	std::cout << "Sampling Rate = " + to_string(pargs->SamplingRate) << endl;
	std::cout << "Frequency = " + to_string(pargs->Frequency) << endl;
	std::cout << "LNA State = " + to_string(pargs->LNAstate) << ". Might be changed by the Gain value setting."<< endl;
	std::cout << "Gain = " + to_string(pargs->Gain) << endl;
	std::cout << "BitWidth = " + to_string(pargs->BitWidth) << endl;
	std::cout << "Antenna = " + to_string(pargs->Antenna) << endl;
	std::cout << "Tuner = " + to_string(pargs->Tuner) << endl;
	std::cout << "Master = " + to_string(pargs->Master) << endl;
	std::cout << "Basic Mode (rtl_tcp compatible) = " + to_string(pargs->BasicMode) << endl;

	std::cout << "\nStarting sdrplay...\n";

	pthread_mutex_init(&stateLock, NULL);

	gainConfiguration::createGainConfigTables();
	gainConfiguration::createGainConfigTable_RSP1B();

	// Open API
	if ((err = sdrplay_api_Open()) == sdrplay_api_Success)
	{
		printf("sdrplay_api_Open successful\n");
	}
	else
	{
		printf("*** Error on sdrplay_api_Open: %s\n", sdrplay_api_GetErrorString(err));
		return -1;
	}
	////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	if ((err = sdrplay_api_DisableHeartbeat()) != sdrplay_api_Success)
	{
		printf("sdrplay_api_DisableHeartbeat failed %s\n", sdrplay_api_GetErrorString(err));
	}
	else
		cout << " *** Heartbeat disabled *** " << endl;
	////////////////////////////////////////////////////////////////
#endif
	//// Lock API while device selection is performed
	//sdrplay_api_LockDeviceApi();

	// Check API versions match
	if ((err = sdrplay_api_ApiVersion(&apiVersion)) != sdrplay_api_Success)
	{
		printf("*** Error on sdrplay_api_ApiVersion: %s\n", sdrplay_api_GetErrorString(err));
		goto exitapp;
	}
	//std::cout << "sdrplay API Version " << apiVersion << endl << endl;
		printf("sdrplay API dll Version %.2f\n", apiVersion);
		
	if ((apiVersion < SDRPLAY_API_VERSION -epsilon) || (apiVersion > SDRPLAY_API_VERSION + epsilon))
	{
		printf("*** Warning : API version doesn't match (local=%.2f dll=%.2f)\n", SDRPLAY_API_VERSION, apiVersion);
		//retCode = E_WRONG_API_VERSION;
		//sError = returnErrorStrings[retCode];
		//goto exitapp;
	}

	res = devices::instance().collectDevices();
	if (res)
	{
		std::cout << devices::instance().numDevices << " Device(s) found\n";
		for (int i=0; i < devices::instance().numDevices; i++)
		{
			sdrplay_api_DeviceT* pd = &devices::instance().sdrplayDevices[i];
			int hwVer = (int)pd->hwVer;
			//!! test uncomment next two lines
			//if (hwVer == 4)
			//	hwVer = 7;
			//!! test end
			if ((hwVer < SDRPLAY_RSP1_ID || hwVer > SDRPLAY_RSPdxR2_ID) &&
				hwVer != SDRPLAY_RSP1A_ID)
			{
				cout << "\tUnknown Hardware version: " << hwVer << endl;
				continue;
			}

			cout << "\tSerial: " << pd->SerNo << endl;
			cout << "\tHardware Version: " << (int)hwVer << endl;
			//cout << "\tHardware Version: " << (int)pd->hwVer << endl;
			cout << "\tHardware Type: " << deviceNameByType[hwVer] << endl;
			cout << "\n";
		}
		devices::instance().Start(pargs);
		goto Close;
	}
	else
	{
		retCode = E_NO_DEVICE;
		goto exitapp;
	}

exitapp:
	if (retCode != 0)
	{
		delete pargs;
		cout << returnErrorStrings[retCode] << endl;
	Close:		
		cout << "Application closing. \n" << endl;
		sdrplay_api_Close();
	}
#ifdef _WIN32
	WSACleanup();
#endif
	pthread_mutex_destroy(&stateLock);

	return retCode;

}
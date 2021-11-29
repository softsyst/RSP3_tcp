/**
** RSP3_tcp - TCP/IP I/Q Data Server for the sdrplay RSPs
** Copyright (C) 2017 Clem Schmidt, softsyst GmbH, https://qirx.softsyst.com
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
#include "common.h"
#include "rsp_tcp.h"
#include "rsp_cmdLineArgs.h"
#include "devices.h"
#include "sdrGainTable.h"
#include "syTwoDimArray.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include <stdio.h>
#include <conio.h>
#include <signal.h>

using namespace std;
int masterInitialised = 0;
int slaveUninitialised = 0;
sdrplay_api_DeviceT *chosenDevice = NULL;

// V0.9.8	Sampling rate 2.000 for ADS-B
// V0.9.9	Bitwidth 8 Bit corrected
string Version = "0.3.1";

map<eErrors, string> returnErrorStrings =
{
	 { E_OK, "OK"}
    ,{ E_PARAMETER, "Starting Parameter Error"}
	,{ E_WRONG_API_VERSION, "Wrong or incompatible sdrplay API Version. Must be at least 3.07."}
	,{ E_NO_DEVICE, "No sdrplay device found."}
	,{ E_DEVICE_INDEX, "Requested Device Index not present: "}
	,{ E_WIN_WSA_STARTUP, "WSAStartup failed with error:  "}
};


void SigInt_Handler(int n_signal)
{
	//printf("interrupted\n");
	//sdrplay_api_Close();
	//#ifdef _WIN32
	//WSACleanup();
	//#endif
	//exit(1);
}

void SigBreak_Handler(int n_signal)
{
	sdrplay_api_Close();
	printf("sdrplay_api closed\n");
	#ifdef _WIN32
	WSACleanup();
	#endif
	exit(2);
}

int main(int argc, char* argv[])
{
	sdrplay_api_ErrT err;

	rsp_cmdLineArgs* pargs = 0;
	eErrors retCode = E_OK;
	string sError;
#ifdef _WIN32
	WSADATA wsd;
	int result = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (result != 0)
	{
		retCode = E_WIN_WSA_STARTUP;
		sError = returnErrorStrings[retCode] + to_string(result);
		goto exit;
	}
	signal(SIGINT, &SigInt_Handler);
	signal(SIGBREAK, &SigBreak_Handler);
	//if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
	//	std::cout << "Error setting ctrl-c handler." << endl;
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
		goto exit;
	}
	std::cout << "IP Address = " + pargs->Address.sIPAddress << endl;
	std::cout << "Port Number = " + to_string(pargs->Port) << endl;
	std::cout << "Sampling Rate = " + to_string(pargs->SamplingRate) << endl;
	std::cout << "Frequency = " + to_string(pargs->Frequency) << endl;
	std::cout << "Gain = " + to_string(pargs->Gain) << endl;
	std::cout << "BitWidth = " + to_string(pargs->BitWidth) << endl;
	//std::cout << "Antenna = " + to_string(pargs->Antenna) << endl;
	std::cout << "Tuner = " + to_string(pargs->Tuner) << endl;
	std::cout << "Master = " + to_string(pargs->Master) << endl;

	std::cout << "\nStarting sdrplay...\n";

	gainConfiguration::createGainConfigTables();

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
	//if ((err = sdrplay_api_DisableHeartbeat()) != sdrplay_api_Success)
	//{
	//	printf("sdrplay_api_DisableHeartbeat failed %s\n", sdrplay_api_GetErrorString(err));
	//}
	////////////////////////////////////////////////////////////////
#endif
	// Lock API while device selection is performed
	sdrplay_api_LockDeviceApi();

	float apiVersion = 0.0f;
	// Check API versions match
	if ((err = sdrplay_api_ApiVersion(&apiVersion)) != sdrplay_api_Success)
	{
		printf("*** Error on sdrplay_api_ApiVersion: %s\n", sdrplay_api_GetErrorString(err));
		goto exit;
	}
	std::cout << "sdrplay API Version " << apiVersion << endl << endl;
		
	double epsilon = 0.0001;

	if ((apiVersion < SDRPLAY_API_VERSION -epsilon) || (apiVersion > SDRPLAY_API_VERSION + epsilon))
	{
		printf("*** Error : API version don't match (local=%.2f dll=%.2f)\n", SDRPLAY_API_VERSION, apiVersion);
		retCode = E_WRONG_API_VERSION;
		sError = returnErrorStrings[retCode];
		goto exit;
	}

	unsigned int numDevices = 0;
	bool res = devices::instance().getDevices();
	if (res)
	{
		std::cout << devices::instance().numDevices << " Device(s) found\n";
		for (int i=0; i < devices::instance().numDevices; i++)
		{
			sdrplay_api_DeviceT* pd = &devices::instance().sdrplayDevices[i];
			cout << "\tSerial: " << pd->SerNo << endl;
			cout << "\tHardware Version: " << (int)pd->hwVer << endl;
			cout << "\n";
		}
		//if (devices::instance().SelectDevice(pargs))
		//{
		//	std::cout << "Device with Serial " << devices::instance().SelectedDevice->serno() << " selected." << endl;
		//}
		//else
		//{
		//	std::cout << "No matching device found." << endl;
		//	goto exit;
		//}
		devices::instance().Start(pargs);
		goto close;
	}
	else
	{
		retCode = E_NO_DEVICE;
		sError = returnErrorStrings[retCode];
		goto exit;
	}

exit:
	if (retCode != 0)
	{
		delete pargs;
		std::cout << sError << endl;
		std::cout << "Application cannot continue. \n" << endl;
		sdrplay_api_UnlockDeviceApi();
close:		sdrplay_api_Close();
		//cout << "Please press any character to exit here..." << endl;
		//getchar();
	}
#ifdef _WIN32
	WSACleanup();
#endif
	return retCode;

}
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
#ifdef _WIN32
#pragma warning(disable:4996)
#endif
#include <map>
#include "sdrplay_device.h"
#include "sdrplay_api.h"
#include "rsp_cmdLineArgs.h"

class crc32;

class devices
{

	// Singleton pattern
private:
	devices() 
	{
		numDevices = 0;
		SelectedDevice = 0;

		for (int i = 0; i < MAX_DEVICES; i++)
			sdrplayDevices[i].dev = 0;
	}
	devices(devices const&);				// Don't Implement
	void operator=(devices const&);			// Don't implement
public:
	static devices& instance()
	{
		static devices instance; // Guaranteed to be destroyed.
									// Instantiated on first use.
		return instance;
	}
	sdrplay_api_DeviceT sdrplayDevices[MAX_DEVICES];
	int numDevices;

	sdrplay_device* SelectedDevice;

	void Start(rsp_cmdLineArgs*  pargs);
	void Stop();
	void CloseClient();
	bool collectDevices();

	int getNumDevices() { return numDevices; }
	uint32_t* getSerialCRCs() { return &serialCRCs[0]; }
	sdrplay_api_DeviceT* getSdrplayDevices() { return &sdrplayDevices[0]; }
private:
	void initListener();
	void doListen();

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;
	sdrplay_device* currentDevice;
	sockaddr_in local;
	sockaddr_in remote;
	struct addrinfo *result = NULL;
	//struct addrinfo hints;

	// command line arguments
	//static rsp_cmdLineArgs* pargs;

	// default values, may be overridden by command line arguments
    IPAddress  listenerAddress = IPAddress(0,0,0,0);
	int listenerPort = 7890;
	sdrplay_device* pd = 0;
	crc32* _crc32 = 0;
	uint32_t serialCRCs[MAX_DEVICES];

public:

	/// <summary>
	/// The list of all RSP2 devices
	/// Key: Serial Number, Value: Object
	/// </summary>
	//map<string, sdrplay_device*> sdrplayDevices;
};


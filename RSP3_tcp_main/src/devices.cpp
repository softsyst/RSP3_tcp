/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSPs
** Copyright (C) 2017-2023 Clem Schmidt, softsyst, http://qirx.softsyst.com
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

#include <iostream>
#include <string>
#include <thread>
#include "common.h"
#include "devices.h"
#include "crc32.h"
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

using namespace std;
static rsp_cmdLineArgs* pargs = 0;
extern bool exitRequest;

/// <summary>
/// Collect all sdrplay devices
/// </summary>
/// <returns></returns>
bool devices::collectDevices()
{
	try
	{
		for (int i = 0; i < MAX_DEVICES; i++)
		{
			sdrplayDevices[i].dev = 0;
			memset(&sdrplayDevices[i].SerNo, 0, 64);
			sdrplayDevices[i].valid = 0;
			sdrplayDevices[i].hwVer = 111;
			sdrplayDevices[i].tuner = sdrplay_api_Tuner_Neither;

			serialCRCs[i] = 0;
		}
		_crc32 = new crc32(0xffffffff, true, 0xedb88320);

		sdrplay_api_ErrT err;
		err = sdrplay_api_GetDevices(sdrplayDevices, (unsigned int*)&numDevices, MAX_DEVICES);
		if (numDevices == 0)
			return false;

		int ierr = (int)err;
		string error = "sdrplay_api_GetDevices failed with error :" + to_string(ierr);
		cout << "sdrplay_api_GetDevices returned with: " << err << endl;
		for (int i = 0; i < numDevices; i++)
		{
			if (err != sdrplay_api_Success)
				throw msg_exception(error.c_str());

			if ((sdrplayDevices[i].hwVer < SDRPLAY_RSP1_ID || sdrplayDevices[i].hwVer > SDRPLAY_RSPdx_ID) &&
				sdrplayDevices[i].hwVer != SDRPLAY_RSP1A_ID)
			{
				printf("Unknown Hardware version %d .\n", sdrplayDevices[i].hwVer);
				continue;
			}
			serialCRCs[i] = _crc32->calcCrcVal((uint8_t*)sdrplayDevices[i].SerNo, 64);
		}
	}
	catch (exception& e)
	{
		cout << "Error reading devices: " << e.what() << endl;
		return false;
	}
	return true;
}
// Loops endless until Stop
void devices::Start(rsp_cmdLineArgs*  args)
{
	if (pargs == 0)
		pargs = args;
	try
	{
		if (numDevices < 1)
			throw msg_exception("No sdrplay devices present.");

		listenerAddress = pargs->Address;
		listenerPort = pargs->Port;
		initListener();
		doListen();
	}
	catch (const std::exception& e)
	{
		cout << "Cannot start listener: " << e.what() << endl;
	}
}
void devices::Stop()
{
	try
	{
		closesocket(clientSocket);
		closesocket(listenSocket);
		if (pd != 0 )
		{
			pd->ctrlThreadExitFlag = true;
		}
		int sleep_ms = 2000;
#ifdef __GNUC__
		usleep(sleep_ms * 1000);
#else
		Sleep(sleep_ms); // let the controlThread terminate
#endif
		delete _crc32;
		exit(-5);
	}
	catch (const std::exception& e)
	{
		cout << "Exception when stopping: " << e.what() << endl;
	}
}
void devices::CloseClient()
{
	try
	{
		closesocket(clientSocket);
		int sleep_ms = 2000; // let the controlThread terminate
#ifdef __GNUC__
		usleep(sleep_ms * 1000);
#else
		Sleep(sleep_ms);
#endif
	}
	catch (const std::exception& e)
	{
		cout << "Exception when stopping: " << e.what() << endl;
	}
}

void devices::initListener()
{
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons((uint16_t)listenerPort);
	local.sin_addr.s_addr = inet_addr(listenerAddress.sIPAddress.c_str());

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		throw msg_exception("INVALID_SOCKET");
	int r = 1;
	struct linger ling = { 1,0 };

	int res = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&r, sizeof(int));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());

	res = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());

	res = ::bind(listenSocket, (struct sockaddr *)&local, sizeof(local));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());

}
void devices::doListen()
{
	/**/
	try
	{
		int maxConnections = 1;
		int res = listen(listenSocket, maxConnections);
		if (res == SOCKET_ERROR)
			throw msg_exception(common::getSocketErrorString());

		while (listenSocket != INVALID_SOCKET)
		{
			pd = new sdrplay_device(pargs);
			if (pd == 0)
				throw "Cannot create device";

			cout << "Listening to " << pargs->Address.sIPAddress << ":" << to_string(pargs->Port) << endl;
			socklen_t rlen = sizeof(remote);
			clientSocket = accept(listenSocket, (struct sockaddr*)&remote, &rlen);
			if (clientSocket == INVALID_SOCKET || exitRequest)
			{
				cout << "Server socket accept error." << endl;
				break;
			}
			cout << "Client Accepted!\n" << endl;
			int yes = 1;
			int result = setsockopt(clientSocket,
				IPPROTO_TCP,
				TCP_NODELAY,
				(char*)&yes,
				sizeof(int));    // 1 - on, 0 - off
			if (result < 0)
				cout << "Error on setting TCP_NODELAY" << endl;

			pd->start(clientSocket); // creates the receive and stream thread

			void* status;
			pthread_join(*pd->thrdRx, &status);
			cout << endl << "++++ Rx thread terminated ++++" << endl;
			pd->thrdRx = 0;
			pd->stop();

			pthread_join(*pd->thrdTx, &status);
			cout << endl << "++++ Tx thread terminated ++++" << endl;
			delete pd->thrdTx;
			pd->thrdTx = 0;


			/*###*/pthread_join(*pd->thrdCtrl, &status);
			cout << endl << "++++ Ctrl thread terminated ++++" << endl;
			delete pd->thrdCtrl;
			pd->thrdCtrl = 0;

			closesocket(clientSocket);
			pd->remoteClient = INVALID_SOCKET;
			cout << "Socket closed\n\n";
			delete pd;
			pd = 0;
		}
	}
	catch (exception& e)
	{
		cout << "*** Error starting listener: " << e.what() << endl;
	}
	cout << "*** Exiting listening loop" << endl;
	/**/
}

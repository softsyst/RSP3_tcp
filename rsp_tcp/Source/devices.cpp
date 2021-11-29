/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSPs
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

#include <iostream>
#include <string>
#include <thread>
#include "common.h"
#include "devices.h"
#ifndef _WIN32
#include <netdb.h>
#endif

using namespace std;
static rsp_cmdLineArgs* pargs = 0;

bool devices::SelectDevice(rsp_cmdLineArgs*  args)
{
	if (pargs == 0)
		pargs = args;
	sdrplay_api_TunerSelectT tun = (sdrplay_api_TunerSelectT)pargs->Tuner;
	SelectedDevice = 0;

	sdrplay_device* sdev = new sdrplay_device();

	if ((tun == sdrplay_api_Tuner_B) || (tun == sdrplay_api_Tuner_Both) ||
		(pargs->Master == true)) // requires RSPduo
	{
		// Choose device: Algo adapted from sdrplay's example
		for (int i=0; i < numDevices; i++)
		{
			// Pick first RSPduo
			sdrplay_api_DeviceT* pd = &sdrplayDevices[i];
			string devserno = string(pd->SerNo);
			if (common::hasEnding(devserno, pargs->Serial) &&
				pd->hwVer == SDRPLAY_RSPduo_ID )
			{
				sdev->setDevice(pd);
				SelectedDevice = sdev;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < numDevices; i++)
		{
			// Pick first 
			sdrplay_api_DeviceT* pd = &sdrplayDevices[i];
			string devserno = string(pd->SerNo);
			if (common::hasEnding(devserno, pargs->Serial))
			{
				sdev->setDevice(pd);
				SelectedDevice = sdev;
				break;
			}
		}
	}
	if (SelectedDevice == 0)
	{
		delete sdev;
		return false;
	}

	return true;
}
/// <summary>
/// Collect all sdrplay devices
/// </summary>
/// <returns></returns>
bool devices::getDevices()
{
	try
	{
		{
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

				if ((sdrplayDevices[i].hwVer < 1 || sdrplayDevices[i].hwVer > 3) &&
					sdrplayDevices[i].hwVer != 255)
				{
					printf("Unknown Hardware version %d .\n", sdrplayDevices[i].hwVer);
					continue;
				}
			}
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
		listenerAddress = pargs->Address;
		listenerPort = pargs->Port;
		initListener();
		doListen();

		if (numDevices < 1)
			throw msg_exception("No sdrplay devices present.");
	}
	catch (const std::exception& e)
	{
		cout << "Cannot start listener: " << e.what() << endl;
	}
}

void devices::initListener()
{
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(listenerPort);
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
	sdrplay_api_ErrT err;
	try
	{
		devices* d = this;
		int maxConnections = 1;
		SOCKET sock = listenSocket;
		int res = listen(sock, maxConnections);
		if (res == SOCKET_ERROR)
			throw msg_exception(common::getSocketErrorString());

		while (sock != INVALID_SOCKET)
		{
			if (SelectDevice(pargs))
			{
				std::cout << "Device with Serial " << devices::instance().SelectedDevice->serno() << " selected." << endl;
			}
			else
			{
				std::cout << "No matching device found." << endl;
				break;
			}
			currentDevice = 0;

			sdrplay_device* pd = SelectedDevice;
			if (pd == 0)
				return;
			pd->init(pargs);

			// create the control thread and its socket communication
			pd->createCtrlThread(pargs->Address.sIPAddress.c_str(), pargs->Port + 1);

			cout << "Listening to " << pargs->Address.sIPAddress << ":" << to_string(pargs->Port) << endl;
			socklen_t rlen = sizeof(remote);
			clientSocket = accept(sock, (struct sockaddr *)&remote, &rlen);
			cout << "Client Accepted!\n" << endl;

			pd->start(clientSocket); // creates the receive and stream thread

			void* status;
			pthread_join(*pd->thrdRx, &status);
			cout << endl << "++++ Rx thread terminated ++++" << endl;
			delete pd->thrdRx;
			pd->thrdRx = 0;
			pd->stop();

			pthread_join(*pd->thrdCtrl, &status);
			cout << endl << "++++ Ctrl thread terminated ++++" << endl;
			delete pd->thrdCtrl;
			pd->thrdCtrl = 0;

			closesocket(clientSocket);
			pd->remoteClient = INVALID_SOCKET;
			cout << "Socket closed\n\n";

		}
	}
	catch (exception& e)
	{
		cout << "Error starting listener: " << e.what();
	}
	/**/
}





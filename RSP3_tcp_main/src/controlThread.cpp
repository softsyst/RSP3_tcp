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

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <climits>

#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
//#include "getopt/getopt.h"
#endif

#ifdef NEED_PTHREADS_WORKARROUND
#define HAVE_STRUCT_TIMESPEC
#endif
//#include <pthread.h>

#include "sdrplay_device.h"
enum eIndications
{
	  IND_GAIN              = 0
	, IND_LNA_STATE         = 0x4b			  // 0: most sensitive, 8: least sensitive
	, IND_SERIAL			= 0x80            // A list with all available serial numbers, as a response on CMD_SET_RSP_REQUEST_ALL_SERIALS
	, IND_WELCOME			= 0x81            // Termination of the initial parameters
	, IND_MAGIC_STRING		= 0x82            // "RTL0"
	, IND_RX_STRING			= 0x83            // "RSPx"
	, IND_RX_TYPE			= 0x84            // 1 byte
	, IND_BIT_WIDTH			= 0x85            // 1 byte
	, IND_OVERLOAD_A		= 0x86            // 1 byte, 1 == overload
	, IND_OVERLOAD_B		= 0x87            // 1 byte
	, IND_DEVICE_RELEASED	= 0x88            // 1 byte bool
	, IND_RSPDUO_HiZ    	= 0x89            // 1 byte bool
	, IND_BIAST_STATE       = 0x8A			  // 0: off, 1: on
};

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")

typedef int socklen_t;
#else
#define closesocket close
#define SOCKADDR struct sockaddr
#define SOCKET int
#define SOCKET_ERROR -1
#endif
extern pthread_mutex_t stateLock;

#define MAX_LEN  (1024)
#define TX_BUF_LEN (1024) //tbd

ctrl_thread_data_t ctrl_thread_data;
unsigned char txbuf[TX_BUF_LEN];

//https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
template <typename T>
void swapEnd(T& var)
{
	static_assert(std::is_pod<T>::value, "Type must be POD type for safety");
	std::array<char, sizeof(T)> varArray;
	/*std::*/memcpy(varArray.data(), &var, sizeof(T));
	for (int i = 0; i < static_cast<int>(sizeof(var) / 2); i++)
		std::swap(varArray[sizeof(var) - 1 - i], varArray[i]);
	/*std::*/memcpy(&var, varArray.data(), sizeof(T));
}
/// <summary>
/// Preapres the buffer "ready to send" for an simple indication
/// </summary>
/// <param name="indic">Indication</param>
/// <param name="value">one item</param>
/// <param name="length">Length of value in bytes</param>
/// <remark>
/// One byte indication
/// Two bytes length, the length of the following value(s)
int prepareIntCommand(BYTE* tx, int startIx, int indic, int value, uint16_t length )
{
	//Big Endian / Network Byte Order
	int ix = startIx;

	tx[ix++] = BYTE(indic & 0xff);
	txbuf[ix++] = BYTE((length >> 8) & 0xff);
	txbuf[ix++] = BYTE(length & 0xff);
	uint16_t val = 0;
	switch (length)
	{
	case 1:
		tx[ix++] = BYTE(value & 0xff);
		break;
	case 2:
		val = uint16_t(value & 0xffff);
		tx[ix++] = BYTE((val>>8) & 0xff);
		tx[ix++] = BYTE(val & 0xff);
		break;
	case 4:
		tx[ix++] = BYTE((value >> 24) & 0xff);
		tx[ix++] = BYTE((value >> 16) & 0xff);
		tx[ix++] = BYTE((value >> 8) & 0xff);
		tx[ix++] = BYTE((value >> 0) & 0xff);
		break;
	default:
		break;
	}
	return ix;
}

int prepareStringCommand(BYTE* tx, int startIx, int indic, BYTE* value, uint16_t length )
{
	//Big Endian / Network Byte Order
	int ix = startIx;

	tx[ix++] = BYTE(indic & 0xff);
	txbuf[ix++] = BYTE((length >> 8) & 0xff);
	txbuf[ix++] = BYTE(length & 0xff);

	for (int i = 0; i < length; i++)
		txbuf[ix++] = value[i];
	return ix;
}

bool sendBuffer(const SOCKET& socket, void* buf, int len, bool* do_exit)
{
	/* now start (possibly blocking) transmission */
	struct timeval tv = { 1,0 };
	int bytessent = 0;
	int bytesleft = len;
	int index = 0;
	fd_set writefds;

	while (bytesleft > 0) {
		FD_ZERO(&writefds);
		FD_SET(socket, &writefds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		int r = select(socket + 1, NULL, &writefds, NULL, &tv);
		if (r) {
			bytessent = (int)send(socket, (const char*)&txbuf[index], bytesleft, 0);
			bytesleft -= bytessent;
			index += bytessent;
		}
		if (bytessent == SOCKET_ERROR || *do_exit) {
			return false;
		}
	}
	return true;
}

void *ctrl_thread_fn(void *arg)
{
	int r = 1;
	struct timeval tv = { 1,0 };
	struct linger ling = { 1,0 };
	SOCKET listensocket;
	SOCKET controlSocket;
	int haveControlSocket = 0;
	struct sockaddr_in local, remote;
	socklen_t rlen;

	int len, total_gain=123;
	fd_set connfds;
	fd_set writefds;

	ctrl_thread_data_t *data = (ctrl_thread_data_t *)arg;

	sdrplay_device *dev = (sdrplay_device *)data->dev;
	int port = data->port;
	int wait = data->wait;
	const char *addr = data->addr;
	bool* do_exit = data->pDoExit;
	int retval;
#ifdef _WIN32
	u_long blockmode = 1;
#endif

	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons((uint16_t)port);
	local.sin_addr.s_addr = inet_addr(addr);

	listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, (char *)&r, sizeof(int));
	setsockopt(listensocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
	retval = bind(listensocket, (struct sockaddr *)&local, sizeof(local));
	if (retval == SOCKET_ERROR)
		goto close;
#ifdef _WIN32
	ioctlsocket(listensocket, FIONBIO, &blockmode);
#else
	r = fcntl(listensocket, F_GETFL, 0);
	r = fcntl(listensocket, F_SETFL, r | O_NONBLOCK);
#endif

	while (1) 
	{
		printf("\n\nlistening on Control port %d...\n", port);
		retval = listen(listensocket, 1);
		if (retval == SOCKET_ERROR)
			goto close;
		while (1) 
		{
			FD_ZERO(&connfds);
			FD_SET(listensocket, &connfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			r = select(listensocket + 1, &connfds, NULL, NULL, &tv);
			if (*do_exit) {
				goto close;
			}
			else if (r) {
				rlen = sizeof(remote);
				controlSocket = accept(listensocket, (struct sockaddr *)&remote, &rlen);
				haveControlSocket = 1;
				break;
			}
		}

		setsockopt(controlSocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));

		printf("\nControl client accepted!\n");

		while (1) 
		{
			len = 2;
			total_gain = 123;
			
			sdrplay_api_GainValuesT* gvals = 0;
			float gain = 0;
			int lnastate = 0;
			int biasT = 0;
			bool bias = false;
			bool overload_a = false, overload_b = false;
			bool rspDuoHiZ = false;
			int buflen = 0;

			pthread_mutex_lock(&stateLock);

			switch (dev->CommState)
			{
			case ST_IDLE:
			case ST_SERIALS_PREPARED:
				goto sleep;

			case ST_DEVICE_RELEASED:
				len = prepareIntCommand(txbuf, len, IND_DEVICE_RELEASED, 1, 1);
				dev->CommState = ST_IDLE; // wait for socket to close
				break;

			case ST_SERIALS_REQUESTED: // 1st command
				BYTE tmp[1024];
				buflen = dev->prepareSerialsList(tmp);
				len = prepareStringCommand(txbuf, len, IND_SERIAL, tmp, (uint16_t)buflen);
				dev->CommState = ST_SERIALS_PREPARED; // wait for the next command changing the state
				break;

			case ST_DEVICE_CREATED:
				len = prepareStringCommand(txbuf, len, IND_MAGIC_STRING, (BYTE*)"RTL0", 4);
				char s[5];
#ifdef _WIN32
				strncpy_s(s, "RSPx", 4);
#else
				strncpy(s, "RSPx", 4);
#endif
				s[4] = 0;
				buflen = dev->getRxString(s);
				len = prepareStringCommand(txbuf, len, IND_RX_STRING, (BYTE*)s, (uint16_t)buflen);
				len = prepareIntCommand(txbuf, len, IND_RX_TYPE, dev->getExportedRxType(), 1);
				len = prepareIntCommand(txbuf, len, IND_BIT_WIDTH, dev->getBitWidth(), 1);
				len = prepareIntCommand(txbuf, len, IND_WELCOME, 1, 1);
				dev->CommState = ST_WELCOME_SENT;
				//fall through
			case ST_WELCOME_SENT:
				gvals = dev->getGainValues();
				if (gvals == 0)	// too early
					goto sleep;
				lnastate = dev->getLNAState();
				bias = dev->getBiasTState();
				biasT = bias ? 1 : 0;

				gain = gvals->curr;
				if (gain > 0)
					total_gain = (int)(gain * 10.0f);

				len = prepareIntCommand(txbuf, len, IND_GAIN, total_gain, 2);
				len = prepareIntCommand(txbuf, len, IND_LNA_STATE, lnastate, 1);
				len = prepareIntCommand(txbuf, len, IND_BIAST_STATE, biasT, 1);

				dev->getOverload(overload_a, overload_b);
				len = prepareIntCommand(txbuf, len, IND_OVERLOAD_A, overload_a ? 1 : 0, 1);
				len = prepareIntCommand(txbuf, len, IND_OVERLOAD_B, overload_b ? 1 : 0, 1);

				rspDuoHiZ = dev->getRspDuoHiZ();
				len = prepareIntCommand(txbuf, len, IND_RSPDUO_HiZ, rspDuoHiZ ? 1 : 0, 1);
			
				break;
			default:
				goto sleep;
				break;
			}
			txbuf[0] = BYTE((len >> 8) & 0xff);
			txbuf[1] = BYTE(len & 0xff);

			if (!sendBuffer(controlSocket, txbuf, len, do_exit))
				break;
		sleep:
			pthread_mutex_unlock(&stateLock);

			if (*do_exit)
				break;
			usleep(wait);
		}
	close:
		if (haveControlSocket)
			closesocket(controlSocket);
		if (*do_exit)
		{
			closesocket(listensocket);
			printf("Control Thread terminates\n");
			break;
		}
	}
	return 0;
}

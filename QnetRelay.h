/*
 *   Copyright (C) 2022 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <atomic>
#include <string>
#include <netinet/in.h>

#include "SockAddress.h"
#include "UnixPacketSock.h"
#include "KRBase.h"

#define CALL_SIZE 8
#define IP_SIZE 15

class CQnetRelay : CKRBase
{
public:
	// functions
	CQnetRelay();
	~CQnetRelay();
	bool Initialize(const char *cfgfile);
	bool Run();

private:
	// functions
	int OpenSocket(const CSockAddress &sock);
	void SendToIcom(const unsigned char *buf, const int size) const;

	// read configuration file
	bool ReadConfig(const char *);

	// Unix sockets
	std::string togate;
	CUnixPacketClient ToGate;

	// config data
	std::string LOCAL_IP, REPEATER_IP;
	unsigned short REPEATER_PORT;
	int available_module;

	// parameters
	int icom_fd, gateway_fd;
	CSockAddress icom_sock;
	CSockAddress icom_stack;
	unsigned short G2_COUNTER_OUT, OLD_REPLY_SEQ, NEW_REPLY_SEQ;
};

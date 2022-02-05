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

#include <exception>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cstdlib>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <thread>

#include "QnetIcomStack.h"
#include "QnetTypeDefs.h"
#include "QnetConfigure.h"

#define RELAY_VERSION "20202"

CQnetIcomStack::CQnetIcomStack() :
	G2_COUNTER_OUT(0)
{
}

CQnetIcomStack::~CQnetIcomStack()
{
}

bool CQnetIcomStack::Initialize(const char *cfgfile)
{
	if (ReadConfig(cfgfile))
		return true;

	// for the sendto address of the icom stack
	icom_stack.Initialize(AF_INET, REPEATER_PORT, REPEATER_IP.c_str());

	// open the icom socket
	printf("Open the UDP socket to the icom stack\n");
	icom_sock.Initialize(AF_INET, REPEATER_PORT, LOCAL_IP.c_str());
	icom_fd = OpenSocket(icom_sock);
	if (icom_fd < 0)
		return true;

	// open the unix socket to the gateway
	printf("Connecting to the gateway at %s\n", togate.c_str());
	if (ToGate.Open(togate.c_str(), this))
		return true;

	gateway_fd = ToGate.GetFD();

	printf("File descriptors: icom=%d, gateway=%d\n", icom_fd, gateway_fd);
	return false;
}

void CQnetIcomStack::IcomInit()
{
	// send INIT to Icom Stack
	unsigned char buf[500];
	memset(buf, 0, 10);
	memcpy(buf, "INIT", 4);
	buf[6] = 0x73U;
	// we can use the module a band_addr for INIT
	SendToIcom(buf, 10);
	printf("Initializing the Icom controller...\n");

	// get the acknowledgement from the ICOM Stack
	// do this with 100ms select() so we can Control-C abort cleanly
	CSockAddress addr;
	while (IsRunning()) {
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(icom_fd, &fdset);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		auto rval = select(icom_fd+1, &fdset, NULL, NULL, &tv);
		if (rval > 0)
		{
			socklen_t reclen;
			int recvlen = recvfrom(icom_fd, buf, 500, 0, addr.GetPointer(), &reclen);
			Dump("Got a packet from the stack:", buf, recvlen);
			if (10==recvlen && 0==memcmp(buf, "INIT", 4) && 0x72U==buf[6] && 0x0U==buf[7]) {
				OLD_REPLY_SEQ = 256U * buf[4] + buf[5];
				NEW_REPLY_SEQ = OLD_REPLY_SEQ + 1;
				G2_COUNTER_OUT = NEW_REPLY_SEQ;
				printf("SYNC: old=%u, new=%u\n", OLD_REPLY_SEQ, NEW_REPLY_SEQ);
				break;
			}
		}
	}
}

int CQnetIcomStack::OpenSocket(const CSockAddress &sock)
{
	int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		printf("Cannot create the UDP socket, err: %d, %s\n", errno, strerror(errno));
		return -1;
	}

	int reuse = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1)
	{
		printf("Cannot set the UDP socket %s:%u option, err: %d, %s\n", sock.GetAddress(), sock.GetPort(), errno, strerror(errno));
		close(fd);
		return -1;
	}

	if (0 > bind(fd, sock.GetCPointer(), sock.GetSize()))
	{
		printf("Cannot bind the UDP socket %s:%u address, err: %d, %s\n", sock.GetAddress(), sock.GetPort(), errno, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

void CQnetIcomStack::Run()
{
	IcomInit();

	while (IsRunning())
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(icom_fd, &readfds);
		FD_SET(gateway_fd, &readfds);
		int maxfs = (icom_fd > gateway_fd) ? icom_fd : gateway_fd;

		// don't care about writefds and exceptfds:
		// and we'll wait as long as needed
		int ret = ::select(maxfs+1, &readfds, NULL, NULL, NULL);
		if (ret < 0)
		{
			printf("ERROR: Run: select returned err=%d, %s\n", errno, strerror(errno));
			break;
		}
		if (ret == 0)
			continue;

		// there is something to read!
		sockaddr_in addr;
		memset(&addr, 0, sizeof(sockaddr_in));
		socklen_t size = sizeof(sockaddr);
		ssize_t len;

		if (FD_ISSET(icom_fd, &readfds))
		{
			SDSTR dstr;
			len = recvfrom(icom_fd, dstr.title, sizeof(SDSTR), 0, (sockaddr *)&addr, &size);

			if (ntohs(addr.sin_port) != REPEATER_PORT)
				printf("Unexpected icom port was %u, expected %u.\n", ntohs(addr.sin_port), REPEATER_PORT);

			// acknowledge the packet
			if (0x73U==dstr.flag[0] && (0x21U==dstr.flag[1] || 0x11U==dstr.flag[1] || 0x0U==dstr.flag[1]))
			{
				dstr.flag[0] = 0x72u;
				dstr.flag[1] = dstr.flag[2] = dstr.remaining = 0;
				SendToIcom(dstr.title, 10);
			}

			if (10 == len)
			{
				// handle Icom handshaking
				if (0x72u == dstr.flag[0])
				{	// ACK from rptr
					NEW_REPLY_SEQ = ntohs(dstr.counter);
					if (NEW_REPLY_SEQ == OLD_REPLY_SEQ) {
						G2_COUNTER_OUT = NEW_REPLY_SEQ;
						OLD_REPLY_SEQ = NEW_REPLY_SEQ - 1;
					} else
						OLD_REPLY_SEQ = NEW_REPLY_SEQ;
				}
			}
			else if ((58 == len || 29 == len) && (0 == memcmp(dstr.title, "DSTR", 4)))
			{
				SDSVT dsvt;
				memcpy(dsvt.title, "DSVT", 4);
				dsvt.config = (58 == len) ? 0x10u : 0x20u;
				memset(dsvt.flaga, 0, 3);
				dsvt.id = 0x20;
				dsvt.flagb[0] = dstr.vpkt.dst_rptr_id;
				dsvt.flagb[1] = dstr.vpkt.snd_rptr_id;
				dsvt.flagb[2] = dstr.vpkt.snd_term_id;
				dsvt.streamid = dstr.vpkt.streamid;
				dsvt.ctrl = dstr.vpkt.ctrl;
				if (58 == len)
				{
					memcpy(dsvt.hdr.flag, dstr.vpkt.hdr.flag, 41);
					memcpy(dsvt.hdr.rpt1, dstr.vpkt.hdr.r2, 8);
					memcpy(dsvt.hdr.rpt2, dstr.vpkt.hdr.r1, 4);
					ToGate.Write(dsvt.title, 56);
				}
				else
				{
					memcpy(dsvt.vasd.voice, dstr.vpkt.vasd.text, 12);
					ToGate.Write(dsvt.title, 27);
				}
			}
			else if (len < 0)
			{
				fprintf(stderr, "ERROR: Run: recvfrom() return error %d: %s\n", errno, strerror(errno));
				break;
			}
			else if (0 == len)
			{
				printf("Read zero bytes from the Icom repeater\n");
			}
			else
			{
				Dump("Unexpected packet from Icom reapeater", dstr.title, len);
			}
		}

		if (FD_ISSET(gateway_fd, &readfds))
		{
			SDSVT dsvt;
			len = ToGate.Read(dsvt.title, sizeof(SDSVT));

			if ((56 == len || 27 == len) && (0 == memcmp(dsvt.title, "DSVT", 4)))
			{
				SDSTR dstr;
				memcpy(dstr.title, "DSTR", 4);
				dstr.counter = ntohs(G2_COUNTER_OUT++);
				dstr.flag[0] = 0x73u;
				dstr.flag[1] = 0x12u;
				dstr.flag[2] = 0x00u;
				dstr.remaining = (56 == len) ? 48 : 19;
				dstr.vpkt.icm_id = 0x20u;
				dstr.vpkt.dst_rptr_id = dsvt.flagb[0];
				dstr.vpkt.snd_rptr_id = dsvt.flagb[1];
				dstr.vpkt.snd_term_id = dsvt.flagb[2];
				dstr.vpkt.streamid = dsvt.streamid;
				dstr.vpkt.ctrl = dsvt.ctrl;
				if (56 == len)
				{
					memcpy(dstr.vpkt.hdr.flag, dsvt.hdr.flag, 41);
					memcpy(dstr.vpkt.hdr.r1, dsvt.hdr.rpt2, 8);
					memcpy(dstr.vpkt.hdr.r2, dsvt.hdr.rpt1, 8);
					SendToIcom(dstr.title, 58);
				}
				else
				{
					memcpy(dstr.vpkt.vasd.voice, dsvt.vasd.voice, 12);
					SendToIcom(dstr.title, 29);
				}
			}
			else if (len < 0)
			{
				fprintf(stderr, "ERROR: Run: ToGate.Read() returned error %d: %s\n", errno, strerror(errno));
				break;
			}
			else if (0 == len)
			{
				printf("Read zero bytes from the gateway\n");
			}
			else
			{
				Dump("Unexpected packet from the gateway", dsvt.title, len);
			}
		}
	}

	close(icom_fd);
	ToGate.Close();
}

void CQnetIcomStack::SendToIcom(const unsigned char *buf, const int size) const
{
	int len = sendto(icom_fd, buf, size, 0, icom_stack.GetCPointer(), icom_stack.GetSize());
	if (len == size)
		return;
	if (len < 0)
		printf("sendto() Icom at %s:%u error: %s\n", icom_sock.GetAddress(), icom_sock.GetPort(), strerror(errno));
	else
		printf("ERROR: Icom short send %d bytes, actually sent %d.\n", size, len);
}

// process configuration file and return true if there was a problem
bool CQnetIcomStack::ReadConfig(const char *cfgFile)
{
	CQnetConfigure cfg;
	printf("Reading file %s\n", cfgFile);
	if (cfg.Initialize(cfgFile))
		return true;

	const std::string estr;	// an empty GetDefaultString

	std::string icom_path("module_");
	std::string type;
	// we need to find an Icom module
	for (int i=0; i<3; i++)
	{
		std::string test(icom_path);
		test.append(1, 'a'+i);
		if (cfg.KeyExists(test))
		{
			cfg.GetValue(test, estr, type, 1, 16);
			if (type.compare("icom"))
			{
				fprintf(stderr, "Found an incompatible module, '%s', aborting!\n", type.c_str());
				return true;
			}
			available_module = i;
			break;
		}
	}

	cfg.GetValue("gateway_to_icom", estr, togate, 1, FILENAME_MAX);
	cfg.GetValue("icom_internal_ip", estr, LOCAL_IP, 7, IP_SIZE);
	cfg.GetValue("icom_external_ip", estr, REPEATER_IP, 7, IP_SIZE);
	int i;
	cfg.GetValue("icom_port", estr, i, 10000, 65535);
	REPEATER_PORT = (unsigned short)i;

	return false;
}

int main(int argc, const char **argv)
{
	setbuf(stdout, NULL);
	if (2 != argc)
	{
		fprintf(stderr, "usage: %s path_to_config_file\n", argv[0]);
		return 1;
	}

	if ('-' == argv[1][0])
	{
		printf("QnetIcomStack Version #%s Copyright (C) 2022 by Thomas A. Early N7TAE\n", RELAY_VERSION);
		printf("QnetIcomStack comes with ABSOLUTELY NO WARRANTY; see the LICENSE for details.\n");
		printf("This is free software, and you are welcome to distribute it\nunder certain conditions that are discussed in the LICENSE file.\n");
		return 0;
	}

	CQnetIcomStack qnistack;

	if (qnistack.Initialize(argv[1]))
		return 1;

	qnistack.Run();

	printf("%s is closing.\n", argv[0]);

	return 0;
}

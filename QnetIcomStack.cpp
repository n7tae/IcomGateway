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

#define RELAY_VERSION "50309"

CQnetIcomStack::CQnetIcomStack() : G2_COUNTER_OUT(0)
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
	printf("Opening Gate2Icom\n");
	if (Gate2Icom.Open("gate2icom"))
		return true;
	Icom2Gate.SetUp("icom2gate");

	gateway_fd = Gate2Icom.GetFD();

	printf("File descriptors: icom=%d, gateway=%d\n", icom_fd, gateway_fd);
	return false;
}

void CQnetIcomStack::IcomInit()
{
	// send INIT to Icom Stack
	unsigned char buf[500];
	memset(buf, 0, 10);
	memcpy(buf, "INIT", 4);
	buf[6] = 0x73u;
	SendToIcom(buf, 10);
	printf("Initializing the Icom controller...\n");

	// get the acknowledgement from the ICOM Stack
	while (IsRunning()) {
		CSockAddress addr(AF_INET);
		socklen_t addrlen = addr.GetSize();
		int recvlen = recvfrom(icom_fd, buf, 500, 0, addr.GetPointer(), &addrlen);
		if (10==recvlen && 0==memcmp(buf, "INIT", 4) && 0x72U==buf[6] && 0x0U==buf[7]) {
			OLD_REPLY_SEQ = 256u * buf[4] + buf[5];
			NEW_REPLY_SEQ = OLD_REPLY_SEQ + 1;
			G2_COUNTER_OUT = NEW_REPLY_SEQ;
			printf("Detected the Icom controller! Counter=%u\n", G2_COUNTER_OUT);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

			if (0 == memcmp(dstr.title, "DSTR", 4))
			{
				if ((58 == len || 32 == len || 29 == len) && (0x73u == dstr.flag[0]) && (0x12u == dstr.flag[1]) && (0x0u == dstr.flag[2]))
				{	// regular packet!
					// first: acknowledge the packet
					unsigned char ackn[10];
					memcpy(ackn, dstr.title, 6);
					ackn[6] = 0x72u;
					memset(ackn+7, 0, 3);
					SendToIcom(ackn, 10);

					// finally: process the packet and send it on...
					SDSVT dsvt;
					memcpy(dsvt.title, "DSVT", 4);
					dsvt.config = (58 == len) ? 0x10u : 0x20u;
					memset(dsvt.flaga, 0, 3);
					dsvt.id = 0x20;
					memcpy(dsvt.flagb, dstr.vpkt.flagb, 3);
					dsvt.streamid = dstr.vpkt.streamid;
					dsvt.ctrl = dstr.vpkt.ctrl;
					if (58 == len)
					{
						char m = dstr.vpkt.hdr.r2[7];
						if (not TyMaMo.IsEqual(m, dstr.vpkt.flagb))
						{
							uint8_t tmm[3];
							if (TyMaMo.SetFlagb(m, tmm))
								fprintf(stderr, "Can't retrieve type/mark/module values for module '%c'!\n", m);
							else
								fprintf(stderr, "Incoming type/mark/module on Module %c, %u/%u/%u, doesn't match configured values, %u/%u/%u!\n", m, dstr.vpkt.flagb[0], dstr.vpkt.flagb[1], dstr.vpkt.flagb[2], tmm[0], tmm[1], tmm[2]);
						}
						if (LOG_QSO)
						{
							printf("id=%04x from RPTR count=%u f=%02x%02x%02x icmid=%02x%02x%02x%02x flag=%02x%02x%02x ur=%.8s r1=%.8s r2=%.8s my=%.8s/%.4s\n", ntohs(dstr.vpkt.streamid), ntohs(dstr.counter), dstr.flag[0], dstr.flag[1], dstr.flag[2], dstr.vpkt.icm_id, dstr.vpkt.flagb[0], dstr.vpkt.flagb[1], dstr.vpkt.flagb[2], dstr.vpkt.hdr.flag[0], dstr.vpkt.hdr.flag[1], dstr.vpkt.hdr.flag[2], dstr.vpkt.hdr.ur, dstr.vpkt.hdr.r1, dstr.vpkt.hdr.r2, dstr.vpkt.hdr.my, dstr.vpkt.hdr.nm);
						}
						memcpy(dsvt.hdr.flag, dstr.vpkt.hdr.flag, 3);	// flags
						memcpy(dsvt.hdr.rpt1, dstr.vpkt.hdr.r2, 8);		// reverse order...
						memcpy(dsvt.hdr.rpt2, dstr.vpkt.hdr.r1, 8);		// to make it right for the gateway
						memcpy(dsvt.hdr.urcall, dstr.vpkt.hdr.ur, 22);	// ur, my, nm, pfcs 8+8+4+2==22
						crc.calc(dsvt.title, 56);
						Icom2Gate.Write(dsvt.title, 56);
					}
					else // 29==len or 32==len
					{
						if (LOG_QSO && (dstr.vpkt.ctrl & 0x40u))
						{
							printf("id=%04x from RPTR count=%u end of transmission\n", ntohs(dstr.vpkt.streamid), ntohs(dstr.counter));
						}
						memcpy(dsvt.vasd.voice, (29==len)?dstr.vpkt.vasd.voice:dstr.vpkt.vasd1.voice, 12);
						Icom2Gate.Write(dsvt.title, 27);
					}
				}
				else if ((10 == len) && (0x72u == dstr.flag[0]))
				{
					NEW_REPLY_SEQ = ntohs(dstr.counter);
					if (NEW_REPLY_SEQ == OLD_REPLY_SEQ)
					{
						G2_COUNTER_OUT = NEW_REPLY_SEQ;
						OLD_REPLY_SEQ = NEW_REPLY_SEQ - 1;
					} else
						OLD_REPLY_SEQ = NEW_REPLY_SEQ;
				}
				else if (0x73U==dstr.flag[0] && (0x21U==dstr.flag[1] || 0x11U==dstr.flag[1] || 0x0U==dstr.flag[1]))
				{
					unsigned char buf[10];
					memcpy(buf, dstr.title, 6);
					buf[6]=0x72u;
					memset(buf+7, 0, 3);
					SendToIcom(buf, 10);
				}
				else
				{
					Dump("Unexpected packet from Icom repeater:", dstr.title, len);
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
				Dump("Unexpected packet from Icom reapeater:", dstr.title, len);
			}
		}

		if (FD_ISSET(gateway_fd, &readfds))
		{
			SDSVT dsvt;
			len = Gate2Icom.Read(dsvt.title, sizeof(SDSVT));

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
				char module = 'A' + dstr.vpkt.flagb[2];
				if ('C' < module)
					module = 'A';
				TyMaMo.SetFlagb(module, dstr.vpkt.flagb);
				if (56 == len)
				{
					memcpy(&dstr.vpkt.streamid, &dsvt.streamid, 44);
					crc.calc(dstr.title, 58);
					SendToIcom(dstr.title, 58);
					if (LOG_QSO)
					{
						printf("id=%04x to RPTR count=%u f=%02x%02x%02x icmid=%02x%02x%02x%02x flag=%02x%02x%02x ur=%.8s r1=%.8s r2=%.8s my=%.8s/%.4s\n", ntohs(dstr.vpkt.streamid), ntohs(dstr.counter), dstr.flag[0], dstr.flag[1], dstr.flag[2], dstr.vpkt.icm_id, dstr.vpkt.flagb[0], dstr.vpkt.flagb[1], dstr.vpkt.flagb[2], dstr.vpkt.hdr.flag[0], dstr.vpkt.hdr.flag[1], dstr.vpkt.hdr.flag[2], dstr.vpkt.hdr.ur, dstr.vpkt.hdr.r1, dstr.vpkt.hdr.r2, dstr.vpkt.hdr.my, dstr.vpkt.hdr.nm);
					}
				}
				else
				{
					memcpy(&dstr.vpkt.streamid, &dsvt.streamid, 15);
					SendToIcom(dstr.title, 29);
					if (LOG_QSO && (dstr.vpkt.ctrl & 0x40u))
					{
						printf("id=%04x to RPTR count=%u end of transmission\n", ntohs(dstr.vpkt.streamid), ntohs(dstr.counter));
					}
				}
			}
			else if (len < 0)
			{
				fprintf(stderr, "ERROR: Run: Gate2Icom.Read() returned error %d: %s\n", errno, strerror(errno));
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
	Gate2Icom.Close();
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
			char Mod = 'A' + i;
			cfg.GetValue(test, estr, type, 1, 16);
			if (type.compare("icom"))
			{
				fprintf(stderr, "Found an incompatible module, '%s', aborting!\n", type.c_str());
				return true;
			}
			int val;
			if(cfg.GetValue(test+"_type",test, val, 0, 1))
				return true;
			TyMaMo.SetType(Mod, uint8_t(val));
			if(cfg.GetValue(test+"_mark",test, val, 1, 1))
				return true;
			TyMaMo.SetMark(Mod, uint8_t(val));
			if(cfg.GetValue(test+"_module",test, val, 1, 3))
				return true;
			TyMaMo.SetModule(Mod, uint8_t(val));
		}
	}

	cfg.GetValue("log_qso", estr, LOG_QSO);
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

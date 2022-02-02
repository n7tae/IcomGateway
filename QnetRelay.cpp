/*
 *   Copyright (C) 2018-2021 by Thomas A. Early N7TAE
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

#include "QnetRelay.h"
#include "QnetTypeDefs.h"
#include "QnetConfigure.h"

#define RELAY_VERSION "210420"

CQnetRelay::CQnetRelay() :
	seed(time(NULL)),
	COUNTER(0)
{
}

CQnetRelay::~CQnetRelay()
{
}

bool CQnetRelay::Initialize(const char *cfgfile)
{
	if (ReadConfig(cfgfile))
		return true;

	return false;
}

int CQnetRelay::OpenSocket(const std::string &address, unsigned short port)
{
	if (! port)
	{
		printf("ERROR: OpenSocket: non-zero port must be specified.\n");
		return -1;
	}

	int fd = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		printf("Cannot create the UDP socket, err: %d, %s\n", errno, strerror(errno));
		return -1;
	}

	sockaddr_in addr;
	::memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (! address.empty())
	{
		addr.sin_addr.s_addr = ::inet_addr(address.c_str());
		if (addr.sin_addr.s_addr == INADDR_NONE)
		{
			printf("The local address is invalid - %s\n", address.c_str());
			close(fd);
			return -1;
		}
	}

	int reuse = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1)
	{
		printf("Cannot set the UDP socket %s:%u option, err: %d, %s\n", address.c_str(), port, errno, strerror(errno));
		close(fd);
		return -1;
	}

	if (::bind(fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1)
	{
		printf("Cannot bind the UDP socket %s:%u address, err: %d, %s\n", address.c_str(), port, errno, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

bool CQnetRelay::Run(const char *cfgfile)
{
	if (Initialize(cfgfile))
		return true;

	msock = OpenSocket(MMDVM_INTERNAL_IP, MMDVM_OUT_PORT);
	if (msock < 0)
		return true;

	if (ToGate.Open(togate.c_str(), this))
		return true;

	int fd = ToGate.GetFD();

	printf("msock=%d, gateway=%d\n", msock, fd);

	keep_running = true;

	while (keep_running)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(msock, &readfds);
		FD_SET(fd, &readfds);
		int maxfs = (msock > fd) ? msock : fd;

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
		unsigned char buf[100];
		sockaddr_in addr;
		memset(&addr, 0, sizeof(sockaddr_in));
		socklen_t size = sizeof(sockaddr);
		ssize_t len;

		if (FD_ISSET(msock, &readfds))
		{
			len = ::recvfrom(msock, buf, 100, 0, (sockaddr *)&addr, &size);

			if (len < 0)
			{
				fprintf(stderr, "ERROR: Run: recvfrom(mmdvmhost) return error %d: %s\n", errno, strerror(errno));
				break;
			}

			if (ntohs(addr.sin_port) != MMDVM_IN_PORT)
				fprintf(stderr, "DEBUG: Run: read from msock but port was %u, expected %u.\n", ntohs(addr.sin_port), MMDVM_IN_PORT);

		}

		if (FD_ISSET(fd, &readfds))
		{
			len = ToGate.Read(buf, 100);

			if (len < 0)
			{
				fprintf(stderr, "ERROR: Run: ToGate.Read() returned error %d: %s\n", errno, strerror(errno));
				break;
			}
		}

		if (len == 0)
		{
			fprintf(stderr, "DEBUG: Run: read zero bytes from %u\n", ntohs(addr.sin_port));
			continue;
		}

		if (0 == memcmp(buf, "DSRP", 4))
		{
			//printf("read %d bytes from MMDVMHost\n", (int)len);
			if (ProcessIcom(len, buf))
				break;
		}
		else if (0 == ::memcmp(buf, "DSVT", 4))
		{
			//printf("read %d bytes from MMDVMHost\n", (int)len);
			if (ProcessGateway(len, buf))
				break;
		}
		else
		{
			char title[5];
			for (int i=0; i<4; i++)
				title[i] = (buf[i]>=0x20u && buf[i]<0x7fu) ? buf[i] : '.';
			title[4] = '\0';
			fprintf(stderr, "DEBUG: Run: received unknow packet '%s' len=%d\n", title, (int)len);
		}
	}

	::close(msock);
	ToGate.Close();
	return false;
}

int CQnetRelay::SendTo(const int fd, const unsigned char *buf, const int size, const std::string &address, const unsigned short port)
{
	sockaddr_in addr;
	::memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(address.c_str());
	addr.sin_port = htons(port);

	int len = ::sendto(fd, buf, size, 0, (sockaddr *)&addr, sizeof(sockaddr_in));
	if (len < 0)
		printf("ERROR: SendTo: fd=%d failed sendto %s:%u err: %d, %s\n", fd, address.c_str(), port, errno, strerror(errno));
	else if (len != size)
		printf("ERROR: SendTo: fd=%d tried to sendto %s:%u %d bytes, actually sent %d.\n", fd, address.c_str(), port, size, len);
	return len;
}

bool CQnetRelay::ProcessGateway(const int len, const unsigned char *raw)
{
	if (27==len || 56==len)   //here is dstar data
	{
		SDSVT dsvt;
		::memcpy(dsvt.title, raw, len);	// transfer raw data to SDSVT struct

		SDSTR dstr;	// destination
		// fill in some inital stuff
		::memcpy(dstr.title, "DSTR", 4);
	}
	else
		printf("DEBUG: ProcessGateway: unusual packet size read len=%d\n", len);
	return false;
}

bool CQnetRelay::ProcessIcom(const int len, const unsigned char *raw)
{
	static unsigned short id = 0U;
	SDSTR dstr;
	memcpy(dstr.title, raw, len);	// transfer raw data to SDSRP struct

	return false;
}

// process configuration file and return true if there was a problem
bool CQnetRelay::ReadConfig(const char *cfgFile)
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
			icom_path.assign(test);
			break;
		}
	}

	cfg.GetValue("gateway_tomodem", estr, togate, 1, FILENAME_MAX);
	cfg.GetValue(icom_path+"_internal_ip", type, MMDVM_INTERNAL_IP, 7, IP_SIZE);
	cfg.GetValue(icom_path+"_target_ip", type, MMDVM_TARGET_IP, 7, IP_SIZE);

	int i;
	cfg.GetValue(icom_path+"_local_port", type, i, 10000, 65535);
	MMDVM_IN_PORT = (unsigned short)i;
	cfg.GetValue(icom_path+"_gateway_port", type, i, 10000, 65535);
	MMDVM_OUT_PORT = (unsigned short)i;

	cfg.GetValue(icom_path+"_is_dstarrepeater", type, IS_DSTARREPEATER);
	cfg.GetValue("log_qso", estr, log_qso);

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
		printf("\nQnetRelay Version #%s Copyright (C) 2018-2021 by Thomas A. Early N7TAE\n", RELAY_VERSION);
		printf("QnetRelay comes with ABSOLUTELY NO WARRANTY; see the LICENSE for details.\n");
		printf("This is free software, and you are welcome to distribute it\nunder certain conditions that are discussed in the LICENSE file.\n\n");
		return 0;
	}

	CQnetRelay qnrelay;

	bool trouble = qnrelay.Run(argv[1]);

	printf("%s is closing.\n", argv[0]);

	return trouble ? 1 : 0;
}

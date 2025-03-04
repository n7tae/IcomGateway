/*
 *   Copyright (C) 2020 by Thomas Early N7TAE
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

#include <csignal>
#include <iostream>

#include "Base.h"

std::atomic<bool> CBase::keep_running(true);

CBase::CBase()
{
	std::signal(SIGINT, CBase::SigHandler);
	std::signal(SIGHUP, CBase::SigHandler);
	std::signal(SIGTERM, CBase::SigHandler);
}

bool CBase::IsRunning()
{
	return keep_running;
}

void CBase::SetState(bool state)
{
	keep_running = state;
}

void CBase::SigHandler(int sig)
{
	switch (sig)
	{
	case SIGINT:
	case SIGHUP:
	case SIGTERM:
		keep_running = false;
		break;
	default:
		std::cerr << "caught an unexpected signal=" << sig << std::endl;
		break;
	}
}

void CBase::AddFDSet(int &max, int newfd, fd_set *set)
{
	if (newfd > max)
		max = newfd;
	FD_SET(newfd, set);
}
void CBase::Dump(const std::string &title, const unsigned char *data, unsigned int length)
{
	printf("%s\n", title.c_str());

	unsigned int offset = 0U;

	while (length > 0U)
	{
		std::string output;

		unsigned int bytes = (length > 16U) ? 16U : length;

		for (unsigned i = 0U; i < bytes; i++)
		{
			char temp[10U];
			::sprintf(temp, "%02X ", data[offset + i]);
			output += temp;
		}

		for (unsigned int i = bytes; i < 16U; i++)
			output += "   ";

		output += "   *";

		for (unsigned i = 0U; i < bytes; i++)
		{
			unsigned char c = data[offset + i];

			if (::isprint(c))
				output += c;
			else
				output += '.';
		}

		output += '*';

		printf("%04X:  %s\n", offset, output.c_str());

		offset += 16U;

		if (length >= 16U)
			length -= 16U;
		else
			length = 0U;
	}
}

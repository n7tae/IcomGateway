#pragma once

/*
 *   Copyright (C) 2020,2025 by Thomas Early N7TAE
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

// base class for all modems

#include <sys/select.h>
#include <atomic>
#include <string>

class CBase
{
public:
	CBase();
	bool IsRunning();
	void SetState(bool state);
protected:
	static std::atomic<bool> keep_running;
	static void SigHandler(int sig);
	void AddFDSet(int &max, int newfd, fd_set *set);
	void Dump(const std::string &title, const unsigned char *data, unsigned int length);
};

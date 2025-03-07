/*
 *   Copyright 2025 by Thomas Early, N7TAE
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

#include <string.h>

#include "TypeMarkModule.h"

// the one and only global object
CTypeMarkModule g_FlagB;

CTypeMarkModule::CTypeMarkModule()
{
	for (int i = 0; i<3; i++)
	{
		for (int j = 0; j < 3; j++)
			value[i][j] = 0u;
	}
}

bool CTypeMarkModule::LoadFlagb(char module, const uint8_t *in)
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		memcpy(value[i], in, 3);
		return false;
	}
	return true;
}

bool CTypeMarkModule::SetFlagb(char module, uint8_t *out) const
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		memcpy(out, value[i], 3);
		return false;
	}
	return true;
}

bool CTypeMarkModule::SetType(char module, uint8_t in)
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		value[i][0] = in;
		return false;
	}
	return true;
}

bool CTypeMarkModule::SetMark(char module, uint8_t in)
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		value[i][1] = in;
		return false;
	}
	return true;
}

bool CTypeMarkModule::SetModule(char module, uint8_t in)
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		value[i][2] = in;
		return false;
	}
	return true;
}

bool CTypeMarkModule::IsEqual(char module, const uint8_t *in) const
{
	int i = module - 'A';
	if (0 <= i and i < 3)
	{
		return 0 == memcmp(value[i], in, 3);
	}
	return false;
}

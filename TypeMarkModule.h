#pragma once
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

#include <cstdint>

class CTypeMarkModule
{
public:
	CTypeMarkModule();
	~CTypeMarkModule() {}

	bool LoadFlagb(char module, const uint8_t *in);
	bool SetFlagb(char module, uint8_t *out) const;
	bool SetType(char module, uint8_t in);
	bool SetMark(char module, uint8_t in);
	bool SetModule(char module, uint8_t in);
	bool IsEqual(char module, const uint8_t *in) const;
private:
	uint8_t value[3][3];
};

#pragma once
/*
 *   Copyright 2022 by Thomas Early, N7TAE
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

// for communicating with the Icom Repeater
#pragma pack(push, 1)	// used internally by Icom stacks
using SDSTR = struct dstr_tag
{
	unsigned char title[4];	//  0	"DSTR"
	unsigned short counter;		//  4
	unsigned char flag[3];		//  6	{ 0x73, 0x12, 0x00 }
	unsigned char remaining;	//  9	the number of bytes left in the packet
	union
	{
		struct
		{
			unsigned char mycall[8];	// 10
			unsigned char rpt[8];		// 18
		} spkt;							// total 26
		struct
		{
			unsigned char icm_id;		// 10
			unsigned char dst_rptr_id;	// 11
			unsigned char snd_rptr_id;	// 12
			unsigned char snd_term_id;	// 13
			unsigned short streamid;	// 14
			unsigned char ctrl;			// 16	sequence number hdr=0, voice%21, end|=0x40
			union
			{
				struct
				{
					unsigned char flag[3];	// 17
					unsigned char r1[8];	// 28
					unsigned char r2[8];	// 20
					unsigned char ur[8];	// 36
					unsigned char my[8];	// 44
					unsigned char nm[4];	// 52
					unsigned char pfcs[2];	// 56
				} hdr;						// total 58
				union
				{
					struct
					{
						unsigned char voice[9];	// 17
						unsigned char text[3];	// 26
					} vasd;						// total 29
					struct
					{
						unsigned char UNKNOWN[3];	// 17 not sure what this is, but g2_ doesn't seem to need it
						unsigned char voice[9];		// 20
						unsigned char text[3];		// 29
					} vasd1;						// total 32
				};
			};
		} vpkt;
	};
};
#pragma pack(pop)

// for the g2 external port and between QnetGateway programs
#pragma pack(push, 1)
using SDSVT = struct dsvt_tag
{
	unsigned char title[4];	//  0   "DSVT"
	unsigned char config;	//  4   0x10 is hdr 0x20 is vasd
	unsigned char flaga[3];	//  5   zeros
	unsigned char id;		//  8   0x20
	unsigned char flagb[3];	//  9   0x0 0x1 (A:0x3 B:0x1 C:0x2)
	unsigned short streamid;// 12
	unsigned char ctrl;		// 14   hdr: 0x80 vsad: framecounter (mod 21)
	union
	{
		struct                      // index
		{
			unsigned char flag[3];  // 15
			unsigned char rpt1[8];	// 18
			unsigned char rpt2[8];  // 26
			unsigned char urcall[8];// 34
			unsigned char mycall[8];// 42
			unsigned char sfx[4];   // 50
			unsigned char pfcs[2];  // 54
		} hdr;						// total 56
		struct
		{
			unsigned char voice[9]; // 15
			unsigned char text[3];  // 24
		} vasd;	// voice and slow data total 27
		struct
		{
			unsigned char voice[9];	  // 15
			unsigned char textend[6]; // 24
		} vend;	// voice and end seq total 32 (for DPlus)
	};
};
#pragma pack(pop)

#pragma pack(push, 1)
using SLINKFAMILY = struct link_family_tag
{
	char title[4];
	int family[3];
};
#pragma pack(pop)

/*
** compatibility.cpp
** Handles compatibility flags for maps that are unlikely to be updated.
**
**---------------------------------------------------------------------------
** Copyright 2009 Randy Heit
** Copyright 2009-2018 Christoph Oelckers
 All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** This file is for maps that have been rendered broken by bug fixes or other
** changes that seemed minor at the time, and it is unlikely that the maps
** will be changed. If you are making a map and you know it needs a
** compatibility option to play properly, you are advised to specify so with
** a MAPINFO.
*/

// HEADER FILES ------------------------------------------------------------

#include "sc_man.h"
#include "doomstat.h"
#include "c_dispatch.h"
#include "gi.h"
#include "g_level.h"
#include "p_lnspec.h"
#include "p_tags.h"
#include "filesystem.h"
#include "textures.h"
#include "g_levellocals.h"
#include "actor.h"
#include "p_setup.h"
#include "maploader/maploader.h"
#include "types.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

union FMD5Holder
{
	uint8_t Bytes[16];
	uint32_t DWords[4];
	hash_t Hash;
};

struct FCompatValues
{
	int CompatFlags[3];
	unsigned int ExtCommandIndex;
};

struct FMD5HashTraits
{
	hash_t Hash(const FMD5Holder key)
	{
		return key.Hash;
	}
	int Compare(const FMD5Holder left, const FMD5Holder right)
	{
		return left.DWords[0] != right.DWords[0] ||
			left.DWords[1] != right.DWords[1] ||
			left.DWords[2] != right.DWords[2] ||
			left.DWords[3] != right.DWords[3];
	}
};

struct FCompatOption
{
	const char *Name;
	uint32_t CompatFlags;
	int WhichSlot;
};

enum
{
	SLOT_COMPAT,
	SLOT_COMPAT2,
	SLOT_BCOMPAT
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

static TMap<FMD5Holder, FCompatValues, FMD5HashTraits> BCompatMap;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static const char *const LineSides[] =
{
	"Front", "Back", NULL
};

static const char *const WallTiers[] =
{
	"Top", "Mid", "Bot", NULL
};

static const char *const SectorPlanes[] =
{
	"floor", "ceil", NULL
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
// ParseCompatibility
//
//==========================================================================

void ParseCompatibility()
{
	TArray<FMD5Holder> md5array;
	FMD5Holder md5;
	FCompatValues flags;
	int i, x;
	unsigned int j;

	BCompatMap.Clear();

	// The contents of this file are not cumulative, as it should not
	// be present in user-distributed maps.
	FScanner sc(fileSystem.GetNumForFullName("compatibility.txt"));

	while (sc.GetString())	// Get MD5 signature
	{
		do
		{
			if (strlen(sc.String) != 32)
			{
				sc.ScriptError("MD5 signature must be exactly 32 characters long");
			}
			for (i = 0; i < 32; ++i)
			{
				if (sc.String[i] >= '0' && sc.String[i] <= '9')
				{
					x = sc.String[i] - '0';
				}
				else
				{
					sc.String[i] |= 'a' ^ 'A';
					if (sc.String[i] >= 'a' && sc.String[i] <= 'f')
					{
						x = sc.String[i] - 'a' + 10;
					}
					else
					{
						x = 0;
						sc.ScriptError("MD5 signature must be a hexadecimal value");
					}
				}
				if (!(i & 1))
				{
					md5.Bytes[i / 2] = x << 4;
				}
				else
				{
					md5.Bytes[i / 2] |= x;
				}
			}
			md5array.Push(md5);
			sc.MustGetString();
		} while (!sc.Compare("{"));
		memset(flags.CompatFlags, 0, sizeof(flags.CompatFlags));
		flags.ExtCommandIndex = ~0u;
		while (sc.GetString())
		{
			sc.UnGet();
			break;
		}
		sc.MustGetStringName("}");
		for (j = 0; j < md5array.Size(); ++j)
		{
			BCompatMap[md5array[j]] = flags;
		}
		md5array.Clear();
	}
}

//==========================================================================
//
// CheckCompatibility
//
//==========================================================================

FName MapLoader::CheckCompatibility(MapData *map)
{
	FMD5Holder md5;
	FCompatValues *flags;
	
	map->GetChecksum(md5.Bytes);

	flags = BCompatMap.CheckKey(md5);

	FString hash;

	for (size_t j = 0; j < sizeof(md5.Bytes); ++j)
	{
		hash.AppendFormat("%02X", md5.Bytes[j]);
	}

	if (developer >= DMSG_NOTIFY)
	{
		Printf("MD5 = %s", hash.GetChars());
		if (flags != NULL)
		{
			Printf(", cflags = %08x, cflags2 = %08x, bflags = %08x\n",
				flags->CompatFlags[SLOT_COMPAT], flags->CompatFlags[SLOT_COMPAT2], flags->CompatFlags[SLOT_BCOMPAT]);
		}
		else
		{
			Printf("\n");
		}
	}

	return FName(hash, true);	// if this returns NAME_None it means there is no scripted compatibility handler.
}

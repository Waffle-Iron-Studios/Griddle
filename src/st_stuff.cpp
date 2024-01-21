//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1994-1996 Raven Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//		Cheat code. See *_sbar.cpp for status bars.
//
//-----------------------------------------------------------------------------

#include "d_protocol.h"
#include "gstrings.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "gi.h"
#include "d_net.h"
#include "doomstat.h"
#include "g_level.h"
#include "g_levellocals.h"
#include "d_main.h"

EXTERN_CVAR (Bool, ticker);
EXTERN_CVAR (Int, am_cheat);
EXTERN_CVAR (Int, cl_blockcheats);

struct cheatseq_t
{
	uint8_t *Sequence;
	uint8_t *Pos;
	uint8_t DontCheck;
	uint8_t CurrentArg;
	uint8_t Args[2];
	bool (*Handler)(cheatseq_t *);
};

static bool CheatCheckList (event_t *ev, cheatseq_t *cheats, int numcheats);
static bool CheatAddKey (cheatseq_t *cheat, uint8_t key, bool *eat);
static bool Cht_Generic (cheatseq_t *);
static bool Cht_Music (cheatseq_t *);
static bool Cht_BeholdMenu (cheatseq_t *);
static bool Cht_PumpupMenu (cheatseq_t *);
static bool Cht_AutoMap (cheatseq_t *);
static bool Cht_ChangeLevel (cheatseq_t *);
static bool Cht_ChangeStartSpot (cheatseq_t *);
static bool Cht_WarpTransLevel (cheatseq_t *);
static bool Cht_MyPos (cheatseq_t *);
static bool Cht_Sound (cheatseq_t *);
static bool Cht_Ticker (cheatseq_t *);

// Smashing Pumpkins Into Small Piles Of Putrid Debris. 
static uint8_t CheatNoclip[] =		{ 'p','l','o','i','s','h','i','n','g',255};
static uint8_t CheatNoclip2[] =		{ 'd','o','n','t','c','l','i','p','m','e',255};
static uint8_t CheatGod[] =			{ 'l','g','b','t','p','l','u','s',255 };
static uint8_t CheatAmmo[] =		{ 'g','a','y','a','n','d','a','r','m','e','d',255};
static uint8_t CheatAmmoNoKey[] =	{ 'p','e','r','k','e','l','e',255};
static uint8_t CheatMypos[] =		{ 'p','o','i','n','t','e','r',255};
static uint8_t CheatAmap[] =		{ 'k','o','r','z','o','d','i','n',255};
static uint8_t CheatDishes[] =		{ 'l','i','f','e','h','a','c','k','s',255};

static cheatseq_t WISCheats[] =
{
	{ CheatMypos,			0, 1, 0, {0,0},				Cht_MyPos },
	{ CheatAmap,			0, 0, 0, {0,0},				Cht_AutoMap },
	{ CheatGod,				0, 0, 0, {CHT_IDDQD,0},		Cht_Generic },
	{ CheatAmmo,			0, 0, 0, {CHT_IDKFA,0},		Cht_Generic },
	{ CheatAmmoNoKey,		0, 0, 0, {CHT_IDFA,0},		Cht_Generic },
	{ CheatNoclip,			0, 0, 0, {CHT_NOCLIP,0},	Cht_Generic },
	{ CheatNoclip2,			0, 0, 0, {CHT_NOCLIP,0},	Cht_Generic },
	{ CheatDishes,			0, 0, 0, {CHT_NOWUDIE,0},	Cht_Generic },
};

CVAR(Bool, nocheats, false, CVAR_ARCHIVE)

// Respond to keyboard input events, intercept cheats.
// [RH] Cheats eat the last keypress used to trigger them
bool ST_Responder (event_t *ev)
{
	bool eat = false;

	if (nocheats || !!cl_blockcheats || (gameinfo.nokeyboardcheats))
	{
		return false;
	}
	else
	{
		if (CheatCheckList(ev, WISCheats, countof(WISCheats)))
			return true;
	}
	return false;
}

static bool CheatCheckList (event_t *ev, cheatseq_t *cheats, int numcheats)
{
	bool eat = false;

	if (ev->type == EV_KeyDown)
	{
		int i;

		for (i = 0; i < numcheats; i++, cheats++)
		{
			if (CheatAddKey (cheats, (uint8_t)ev->data2, &eat))
			{
				if (cheats->DontCheck || !CheckCheatmode ())
				{
					eat |= cheats->Handler (cheats);
				}
			}
			else if (cheats->Pos - cheats->Sequence > 2)
			{ // If more than two characters into the sequence,
			  // eat the keypress, just so that the Hexen cheats
			  // with T in them will work without unbinding T.
				eat = true;
			}
		}
	}
	return eat;
}

//--------------------------------------------------------------------------
//
// FUNC CheatAddkey
//
// Returns true if the added key completed the cheat, false otherwise.
//
//--------------------------------------------------------------------------

static bool CheatAddKey (cheatseq_t *cheat, uint8_t key, bool *eat)
{
	if (cheat->Pos == NULL)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0)
	{
		*eat = true;
		cheat->Args[cheat->CurrentArg++] = key;
		cheat->Pos++;
	}
	else if (key == *cheat->Pos)
	{
		cheat->Pos++;
	}
	else
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
	}
	if (*cheat->Pos == 0xff)
	{
		cheat->Pos = cheat->Sequence;
		cheat->CurrentArg = 0;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------
//
// CHEAT FUNCTIONS
//
//--------------------------------------------------------------------------

static bool Cht_Generic (cheatseq_t *cheat)
{
	Net_WriteInt8 (DEM_GENERICCHEAT);
	Net_WriteInt8 (cheat->Args[0]);
	return true;
}

static bool Cht_Music (cheatseq_t *cheat)
{
	char buf[9] = "idmus xx";

	buf[6] = cheat->Args[0];
	buf[7] = cheat->Args[1];
	C_DoCommand (buf);
	return true;
}

static bool Cht_BeholdMenu (cheatseq_t *cheat)
{
	Printf ("%s\n", GStrings("STSTR_BEHOLD"));
	return false;
}

static bool Cht_PumpupMenu (cheatseq_t *cheat)
{
	// How many people knew about the PUMPUPT cheat, since
	// it isn't printed in the list?
	Printf ("Bzrk, Inviso, Mask, Health, Pack, Stats\n");
	return false;
}

static bool Cht_AutoMap (cheatseq_t *cheat)
{
	if (automapactive)
	{
		am_cheat = (am_cheat + 1) % 3;
		return true;
	}
	else
	{
		return false;
	}
}

static bool Cht_ChangeLevel (cheatseq_t *cheat)
{
	char cmd[10] = "idclev xx";

	cmd[7] = cheat->Args[0];
	cmd[8] = cheat->Args[1];
	C_DoCommand (cmd);
	return true;
}

static bool Cht_ChangeStartSpot (cheatseq_t *cheat)
{
	char cmd[64];

	mysnprintf (cmd, countof(cmd), "changemap %s %c", primaryLevel->MapName.GetChars(), cheat->Args[0]);
	C_DoCommand (cmd);
	return true;
}

static bool Cht_WarpTransLevel (cheatseq_t *cheat)
{
	char cmd[11] = "hxvisit xx";
	cmd[8] = cheat->Args[0];
	cmd[9] = cheat->Args[1];
	C_DoCommand (cmd);
	return true;
}

static bool Cht_MyPos (cheatseq_t *cheat)
{
	C_DoCommand ("toggle idmypos");
	return true;
}

static bool Cht_Ticker (cheatseq_t *cheat)
{
	ticker = !ticker;
	Printf ("%s\n", GStrings(ticker ? "TXT_CHEATTICKERON" : "TXT_CHEATTICKEROFF"));
	return true;
}

static bool Cht_Sound (cheatseq_t *cheat)
{
	AddCommandString("stat sounddebug");
	return true;
}

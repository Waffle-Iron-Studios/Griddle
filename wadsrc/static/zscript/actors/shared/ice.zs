

//==========================================================================
//
// Ice chunk
//
//==========================================================================

class IceChunk : Actor
{
	Default
	{
		Radius 3;
		Height 4;
		Mass 5;
		Gravity 0.125;
		+DROPOFF
		+CANNOTPUSH
		+FLOORCLIP
		+NOTELEPORT
		+NOBLOCKMAP
		+MOVEWITHSECTOR
	}
	
	void A_IceSetTics ()
	{
		tics = random[IceTics](70, 133);
		Name dtype = GetFloorTerrain().DamageMOD;
		if (dtype == 'Fire')
		{
			tics >>= 2;
		}
		else if (dtype == 'Ice')
		{
			tics <<= 1;
		}
	}

	States
	{
	Spawn:
		ICEC ABCD 10 A_IceSetTics;
		Stop;
	}
}

//==========================================================================
//
// A chunk of ice that is also a player
//
//==========================================================================

class IceChunkHead : PlayerChunk
{
	Default
	{
		Radius 3;
		Height 4;
		Mass 5;
		Gravity 0.125;
		DamageType "Ice";
		+DROPOFF
		+CANNOTPUSH
	}
	States
	{
	Spawn:
		ICEC A 0;
		ICEC A 10 A_CheckPlayerDone;
		wait;
	}
}


extend class Actor
{

	//============================================================================
	//
	// A_FreezeDeath
	//
	//============================================================================

	void A_FreezeDeath()
	{

		int t = random[freezedeath]();
		tics = 75+t+random[freezedeath]();
		bSolid = bShootable = bNoBlood = bIceCorpse = bPushable = bTelestomp = bCanPass = bSlidesOnWalls = bCrashed = true;
		Height = Default.Height;
		A_SetRenderStyle(1, STYLE_Normal);

		A_StartSound ("misc/freeze", CHAN_BODY);

		// [RH] Andy Baker's stealth monsters
		if (bStealth)
		{
			Alpha = 1;
			visdir = 0;
		}

		if (player)
		{
			player.damagecount = 0;
			player.poisoncount = 0;
			player.bonuscount = 0;
		}
		else if (bIsMonster && special)
		{ // Initiate monster death actions
			A_CallSpecial(special, args[0],	args[1], args[2], args[3], args[4]);
			special = 0;
		}
	}

	//============================================================================
	//
	// A_FreezeDeathChunks
	//
	//============================================================================

	void A_FreezeDeathChunks()
	{
		if (Vel != (0,0,0) && !bShattering)
		{
			tics = 3*TICRATE;
			return;
		}
		Vel = (0,0,0);
		A_StartSound ("misc/icebreak", CHAN_BODY);

		// [RH] In Hexen, this creates a random number of shards (range [24,56])
		// with no relation to the size of the self shattering. I think it should
		// base the number of shards on the size of the dead thing, so bigger
		// things break up into more shards than smaller things.
		// An actor with radius 20 and height 64 creates ~40 chunks.
		int numChunks = max(4, int(radius * Height)/32);
		int i = Random[FreezeDeathChunks](0, numChunks/4 - 1);
		for (i = max(24, numChunks + i); i >= 0; i--)
		{
			double xo = (random[FreezeDeathChunks]() - 128)*radius / 128;
			double yo = (random[FreezeDeathChunks]() - 128)*radius / 128;
			double zo = (random[FreezeDeathChunks]() * Height / 255);

			Actor mo = Spawn("IceChunk", Vec3Offset(xo, yo, zo), ALLOW_REPLACE);
			if (mo)
			{
				mo.SetState (mo.SpawnState + random[FreezeDeathChunks](0, 2));
				mo.Vel.X = random2[FreezeDeathChunks]() / 128.;
				mo.Vel.Y = random2[FreezeDeathChunks]() / 128.;
				mo.Vel.Z = (mo.pos.Z - pos.Z) / Height * 4;
			}
		}
		if (player)
		{ // attach the player's view to a chunk of ice
			Actor head = Spawn("IceChunkHead", pos + (0, 0, player.mo.ViewHeight), ALLOW_REPLACE);
			if (head != null)
			{
				head.Vel.X = random2[FreezeDeathChunks]() / 128.;
				head.Vel.Y = random2[FreezeDeathChunks]() / 128.;
				head.Vel.Z = (head.pos.Z - pos.Z) / Height * 4;

				head.health = health;
				head.Angle = Angle;
				if (head is "PlayerPawn")
				{
					head.player = player;
					head.player.mo = PlayerPawn(head);
					player = null;
					head.ObtainInventory (self);
				}
				head.Pitch = 0.;
				if (head.player.camera == self)
				{
					head.player.camera = head;
				}
			}
		}

		// [RH] Do some stuff to make this more useful outside Hexen
		if (bBossDeath)
		{
			A_BossDeath();
		}
		A_NoBlocking();

		SetStateLabel('null');
	}

	void A_GenericFreezeDeath()
	{
		A_SetTranslation('Ice');
		A_FreezeDeath();
	}	
	
	Actor A_GriddleSpawnGib(Class<Actor> gibtype = null, bool dontthrust = false)
	{
		if (!gibtype)
			return null;

		Actor a;
		
		Vector3 vel3;
		
		Vector3 pos3;
		pos3.x = self.pos.x + Random[GibFx](-24, 24);
		pos3.y = self.pos.y + Random[GibFx](-24, 24);
		pos3.z = self.pos.z + Random[GibFx](4, 10);
		
		if (!dontthrust)
		{
			vel3.x = self.vel.x + Random[GibFx](-20, 20);
			vel3.y = self.vel.y + Random[GibFx](-20, 20);
			vel3.z = self.vel.z + Random[GibFx](0, 12);
		}
		
		a = self.Spawn(gibtype, pos3, ALLOW_REPLACE);
		
		a.vel = vel3;
		a.A_FaceMovementDirection();
		
		return a;
	}

	void A_GriddleFreezeDeath(Name trans='Ice', Class<Actor> chunk="IceChunk")
	{
		for (int i = 0; i <= 10; i++)
		{
			A_GriddleSpawnGib(chunk);
		}
		
		bSolid = bShootable = bNoBlood = bIceCorpse = bPushable = bTelestomp = bCanPass = bSlidesOnWalls = bCrashed = true;

		Height = Default.Height;
		
		A_SetTranslation(trans);
		A_SetRenderStyle(1.0, STYLE_Normal);
		A_StartSound("misc/freeze", CHAN_BODY);

		// [RH] Do some stuff to make this more useful outside Hexen
		if (bBossDeath)
		{
			A_BossDeath();
		}

		if (special)
		{
			A_CallSpecial(special, args[0],	args[1], args[2], args[3], args[4]);
			special = 0;
		}
	}
	
	void A_GriddleIceShards(Class<Actor> chunk="IceChunk", Class<Actor> icepuddle = null, Class<Actor> chunkhead = "IceChunkHead")
	{
		if (vel != (0, 0, 0) && !bShattering)
			return;
		
		vel = (0, 0, 0);
		
		A_StartSound("misc/icebreak", CHAN_BODY);
		
		for (int i = 0; i <= 24; i++)
		{
			A_GriddleSpawnGib("IceChunk");
		}
		
		A_NoBlocking();
		
		if (icepuddle)
			A_SpawnItemEx(icepuddle, flags:SXF_NOCHECKPOSITION);

		if (player)
		{
			// attach the player's view to a chunk of ice
			Actor head = Spawn(chunkhead, pos + (0, 0, player.mo.ViewHeight), ALLOW_REPLACE);
			if (head != null)
			{
				head.Vel.X = random2[FreezeDeathChunks]() / 128.;
				head.Vel.Y = random2[FreezeDeathChunks]() / 128.;
				head.Vel.Z = (head.pos.Z - pos.Z) / Height * 4;

				head.health = health;
				head.Angle = Angle;
				if (head is "PlayerPawn")
				{
					head.player = player;
					head.player.mo = PlayerPawn(head);
					player = null;
					head.ObtainInventory (self);
				}
				head.Pitch = 0.;
				if (head.player.camera == self)
				{
					head.player.camera = head;
				}
			}
		}
		
		SetStateLabel("Null");
	}
}

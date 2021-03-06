/*
		Spectator mode
*/

#include "modules\header\spectate.txt"

// This timer is called frequently to update textdraws for spectators so they can see correct info about who they're spectating
TIMER(UpdateInfoForSpectators)
{
	if(Iter_Count(AllSpectators) == 0)
		return 0;
	
	new specStr[205], specid;
	foreach(new i : AllSpectators)
	{
		specid = Player[i][IsSpectatingID];
		format(specStr, sizeof(specStr),"%s%s ~r~~h~%d~n~~n~%s(%d) (~r~~h~%d%s)~n~FPS: ~r~~h~%d %sPing: ~r~~h~%d~n~%sPacket-Loss: ~r~~h~%.1f~n~%sKills: ~r~~h~%d~n~%sDamage: ~r~~h~%d~n~%sTotal Dmg: ~r~~h~%d",
			MAIN_TEXT_COLOUR, Player[specid][Name], specid, MAIN_TEXT_COLOUR, Player[specid][pArmour], Player[specid][pHealth], MAIN_TEXT_COLOUR, GetPlayerFPS(specid), MAIN_TEXT_COLOUR, GetPlayerPing(specid), MAIN_TEXT_COLOUR, GetPlayerPacketLoss(specid), MAIN_TEXT_COLOUR, Player[specid][RoundKills], MAIN_TEXT_COLOUR, Player[specid][RoundDamage], MAIN_TEXT_COLOUR, Player[specid][TotalDamage]);
		PlayerTextDrawSetString(i, SpecText[i][1], specStr);
		PlayerTextDrawSetString(i, SpecText[i][3], ListWeapsSpec(specid));
	}
	return 1;
}

// This initializes the system
InitSpectate()
{
	Iter_Init(PlayerSpectators); // empty the iterator array
	SetTimer("UpdateInfoForSpectators", UPDATE_SPECINFO_INTERVAL, true); // start timer to update the textdraws of spectators
	return 1;
}
	
// This will empty the iteration array that holds the IDs of the spectators of this player
// This will be used to initialize spectate mode for a player when they connect to the server
InitPlayerSpectate(playerid)
{
	Iter_Clear(PlayerSpectators[playerid]);
	Iter_Remove(AllSpectators, playerid);
}

HandleSpectatedPlayerDisconnect(playerid)
{
	if(!Player[playerid][BeingSpeced])
		return 0;
	
	printf("HandleSpectatedPlayerDisconnect(%d)", playerid);
	foreach(new i : AllSpectators)
	{
		if(Player[i][IsSpectatingID] == playerid)
		{
			if(Player[i][Team] == REFEREE)
				SpectateAnyPlayer(i, false, true, playerid);
			else
			{
				if(Current != -1) 
				{
					SpectateAnyPlayer(i, true, true, playerid);
				}
				else 
				{
					SpectateAnyPlayer(i, false, true, playerid);
				}
			}
		}
	}
	return 1;
}

// Returns a list of the weapons which playerid got
ListWeapsSpec(playerid)
{
	new 
		str[256],
		wep, ammo;
	format(str, sizeof str, "   %sWeapons~n~~n~", MAIN_TEXT_COLOUR);
	for(new slt = 0; slt < 13; slt ++)
	{
		GetPlayerWeaponData(playerid, slt, wep, ammo); // fetch the weapon data in this slot
		if(ammo > 0) // if it isn't out of ammo
		{
			if(IsValidWeapon(wep)) // ignore some weapons like fist and bruss knuckless..etc
			{
				format(str, sizeof(str), "%s%s%s ~r~~h~%d~n~", str, MAIN_TEXT_COLOUR, WeaponNames[wep], ammo);
			}
		}
	}
	return str;
}

// Add this player to the spectators iteration array
RememberSpectator(playerid)
{
	if(!Iter_Contains(AllSpectators, playerid))
		Iter_Add(AllSpectators, playerid);
}

// Remove this player from the spectators iteration array
// Warning: can't be used in a loop over AllSpectators
ForgetSpectator(playerid)
{
	printf("START:ForgetSpectator(%d)", playerid);
	if(Iter_Contains(AllSpectators, playerid))
		Iter_Remove(AllSpectators, playerid);
	printf("END:ForgetSpectator(%d)", playerid);
}

// Update/fix the textdraw that shows who are spectating you
UpdatePlayerSpectators(playerid)
{
	// Iter_Count(PlayerSpectators[playerid]); ==  how many are spectating this guy?
	if(Iter_Count(PlayerSpectators[playerid]) == 0) // if none are spectating 
	{
		PlayerTextDrawSetString(playerid, WhoSpec[playerid], " ");
		Player[playerid][BeingSpeced] = false;
		return 0;
	}
	else
	{
		new str[SPECTATORS_STRING_SIZE];
		format(str, sizeof str, "%sSPECING~n~~n~", MAIN_TEXT_COLOUR); // This is what the 17 characters are for in SPECTATORS_STRING_SIZE
		foreach(new i : PlayerSpectators[playerid]) // Loop through the array that contains this player's spectators
		{
			// Format string with spectator name
			format(str, sizeof str, "%s%s%s~n~", str, MAIN_TEXT_COLOUR, Player[i][NameWithoutTag]);
		}
		foreach(new i : PlayerSpectators[playerid])
			PlayerTextDrawSetString(i, WhoSpec[i], str);
		PlayerTextDrawSetString(playerid, WhoSpec[playerid], str);
	}
	return 1;
}

SpectatePlayer(playerid, specid) 
{
	if(playerid == specid)
		return 0;
	
	if(!IsPlayerConnected(playerid) || !IsPlayerConnected(specid))
		return 0;
	
	if(Iter_Count(PlayerSpectators[specid]) == MAX_PLAYER_SPECTATORS)
	{
		SendErrorMessage(playerid, "This player does already have enough spectators.");
		if(Player[playerid][Spectating])
			StopSpectate(playerid);
		return 0;
	}
		
	if(TargetInfoData[playerid][TargetInfoTimer] != -1)
		HideTargetInfo(playerid); // hides the target info textdraws
	new OldSpecID = -1;
	if(Player[playerid][BeingSpeced] == true) // If anyone is spectating this player, we gotta turn off spectate for them
	{
		foreach(new i : AllSpectators)
		{
		    if(Player[i][IsSpectatingID] == playerid) 
			{
				StopSpectate(i, false);
				Iter_SafeRemove(AllSpectators, i, i);
			}
		}
	} 
	else if (Player[playerid][Spectating] == true && specid != Player[playerid][IsSpectatingID]) 
	{
	    OldSpecID = Player[playerid][IsSpectatingID];
	}
	
	RememberSpectator(playerid); // Remember that this player is a spectator
	
	if(!Player[playerid][Spectating]) // If he's specing for the first time (i.e not from next/previous spectate functions)
	{
		// Spectation info textdraws
		for(new i; i < 4; i++) 
			PlayerTextDrawShow(playerid, SpecText[playerid][i]);
		
		// Team damage textdraws
		ShowTeamBarsForPlayer(playerid);
		// Hide some HUD textdraws
		PlayerTextDrawHide(playerid, RoundKillDmgTDmg[playerid]);
		PlayerTextDrawHide(playerid, ArmourTextDraw[playerid]);
		PlayerTextDrawHide(playerid, HPTextDraw_TD[playerid]);
		HidePlayerProgressBar(playerid, HealthBar[playerid]);
		HidePlayerProgressBar(playerid, ArmourBar[playerid]);
		#if defined _league_included
		PlayerTextDrawHide(playerid, PlayerLeagueStatsTD[playerid]);
		#endif
	}
	
	// This will hide any damage textdraws shown for the spectator sooner
	for(new i = 0; i < MAX_VISUAL_DAMAGES; i ++)
	{
		VisualDamage[playerid][TakenDamageTick][i] = 0;
		VisualDamage[playerid][GivenDamageTick][i] = 0;
	}
	
	// Update variables
	Player[playerid][IsSpectatingID] = specid;
	Player[playerid][Spectating] = true;
	Player[specid][BeingSpeced] = true;

	// Set up spectating mode
	SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(specid));
	SetPlayerInterior(playerid, GetPlayerInterior(specid));
    TogglePlayerSpectating(playerid, 1);
	if(GetPlayerVehicleID(specid)) 
		PlayerSpectateVehicle(playerid, GetPlayerVehicleID(specid));
	else 
		PlayerSpectatePlayer(playerid, specid);

 	if(Player[specid][Playing] == true && Current != -1) 
	{
	    SetCPForPlayer(playerid);
	}
	if(OldSpecID != -1)
	{
		RadarFix(OldSpecID);
		Iter_Remove(PlayerSpectators[OldSpecID], playerid); // State that this player is no longer a spectator of 'OldSpecID'
		UpdatePlayerSpectators(OldSpecID); // Update who-specing textdraws
	}
	Iter_Add(PlayerSpectators[specid], playerid); // State that this player is one of the spectators of 'specid'
	UpdatePlayerSpectators(specid); // Update who-specing 

 	if(Player[specid][Playing] == true && Current != -1) // Show them the checkpoint of the current base
	    SetCPForPlayer(playerid);
	
	RadarFix(playerid);
	RadarFix(specid);
    return 1;
}

/* Note:
	set remove_iterator to false if you're using this in a loop 
	and safely remove, for example

	foreach(new i : AllSpectators)
	{
		StopSpectate(i, false);
		Iter_SafeRemove(AllSpectators, i, i);
	}
*/
StopSpectate(playerid, bool:remove_iterator = true)
{
	printf("START:StopSpectate(%d, %b)", playerid, remove_iterator);
	if(!Player[playerid][Spectating])
		return 0;
	
	if(playerid == INVALID_PLAYER_ID)
		return 0;
	
	if(Player[playerid][SpectatingRound] != -1)
	{
	    Player[playerid][SpectatingRound] = -1;
	    PlayerTextDrawSetString(playerid, TD_RoundSpec[playerid], "_");
	    Player[playerid][Spectating] = false;
		SetSpawnInfo(playerid, GetPlayerTeam(playerid), Skin[Player[playerid][Team]], MainSpawn[0]+random(5),MainSpawn[1]+random(5),MainSpawn[2]+2, MainSpawn[3], 0, 0, 0, 0, 0, 0);
		SetPlayerInterior(playerid,MainInterior);
		Player[playerid][IgnoreSpawn] = true;
		SpawnPlayerEx(playerid);
		return 1;
	}
 	if(Player[playerid][BeingSpeced] == true)
	{
		foreach(new i : AllSpectators)
		{
		    if(i == playerid) // Absence of this causes a stupid error on crashdetect "stack heap size". Endless loop?
		        continue;
		        
			if(Player[i][IsSpectatingID] == playerid) 
			{
			    StopSpectate(i, false);
				Iter_SafeRemove(AllSpectators, i, i);
			}
		}
	}
	new specid = Player[playerid][IsSpectatingID];
	Player[playerid][IgnoreSpawn] = noclipdata[playerid][FlyMode];
	CancelFlyMode(playerid); // Cancel freecam
	
	// Reset variables
	if(remove_iterator)
		ForgetSpectator(playerid);
	
	Player[playerid][Spectating] = false;
	Player[playerid][IsSpectatingID] = -1;
	Player[playerid][BeingSpeced] = false;
	// Hide spec textdraws
	for(new i; i < 4; i++) 
		PlayerTextDrawHide(playerid, SpecText[playerid][i]);
	// Hide team damage textdraws
	HideTeamBarsForPlayer(playerid);
	// Reset text
	PlayerTextDrawSetString(playerid, WhoSpec[playerid], " ");
	// Reshow HUD textdraws
	#if defined _league_included
	if(LeagueMode)
	{
		PlayerTextDrawShow(playerid, PlayerLeagueStatsTD[playerid]);
	}
	#endif
	switch(PlayerInterface[playerid][INTERFACE_HP])
	{
		case true:
		{
			PlayerTextDrawShow(playerid, HPTextDraw_TD[playerid]);
			PlayerTextDrawShow(playerid, ArmourTextDraw[playerid]);
		}
	}
	PlayerTextDrawShow(playerid, RoundKillDmgTDmg[playerid]);
	if(PlayerInterface[playerid][INTERFACE_NET])
		PlayerTextDrawShow(playerid, FPSPingPacket[playerid]);
	PlayerTextDrawShow(playerid, BaseID_VS[playerid]);
	ShowPlayerProgressBar(playerid, HealthBar[playerid]);
	ShowPlayerProgressBar(playerid, ArmourBar[playerid]);

	// Cancel spectating mode
	if(Player[playerid][IgnoreSpawn] == false)
	{
		// This will be called if he was spectating normally, not /freecam mode
		TogglePlayerSpectating(playerid, 0);
		SetPlayerPos(playerid, MainSpawn[0]+random(5), MainSpawn[1]+random(5), MainSpawn[2]+2);
		SetPlayerFacingAngle(playerid, MainSpawn[3]);
	}
	else
	{
		new Float:camPos[3];
		GetPlayerCameraPos(playerid, camPos[0], camPos[1], camPos[2]);
		SetPlayerPos(playerid, camPos[0], camPos[1], camPos[2]);
	}
	SetPlayerInterior(playerid, MainInterior);
	SetPlayerVirtualWorld(playerid, 0);
	SetCameraBehindPlayer(playerid);
	
	if(IsPlayerConnected(specid))
	{
		Iter_Remove(PlayerSpectators[specid], playerid); // State that this player is no longer a spectator of 'specid'
		UpdatePlayerSpectators(specid);
	}
	printf("END:StopSpectate(%d, %b)", playerid, remove_iterator);
	return 1;
}

SpectateNextTeamPlayer(playerid, bool:stop_spectate = false)
{
	// Calling Iter_Next inside loop comparison stage makes the starting index equal to the next player after spectator
	// Loop through all connected players ignoring players with IDs lower than spectator's
	for(new i = Player[playerid][IsSpectatingID]; (i = Iter_Next(Player, i)) != Iter_End(Player);)
	{	
		if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
			continue; // Skip this player if they're dead/wasted or not spawned yet
		
		if(!Player[i][Playing])
			continue; // Skip this player if they're not playing
		
		if(Player[playerid][Team] != REFEREE && !IsTeamTheSame(Player[i][Team], Player[playerid][Team])) 
			continue; // Skip this player if he's in a different team than spectator's team
		
		if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
			continue; // Skip this player if he's got enough people spectating him
		
		if(SpectatePlayer(playerid, i) == 1)
			return; // Stop the loop AS WELL AS the function process when the spectate process is a success
	}
	// This will be reached if no available players to be spectated are found
	if(stop_spectate)	
	{
		StopSpectate(playerid); // Exit spectate mode
		return;
	}
	SpectateAnyPlayer(playerid, (Player[playerid][Team] == REFEREE) ? (false) : (true), true);
}

SpectatePreviousTeamPlayer(playerid, bool:stop_spectate = false) 
{
	// Calling Iter_Next inside loop comparison stage makes the starting index equal to the previous player before spectator
	// Loop through all connected players (descending loop: from highest ID to lowest)
	for(new i = Player[playerid][IsSpectatingID]; (i = Iter_Prev(Player, i)) != Iter_Begin(Player);) 
	{
		if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
			continue; // Skip this player if they're dead/wasted or not spawned yet
		
		if(!Player[i][Playing])
			continue; // Skip this player if they're not playing
		
		if(Player[playerid][Team] != REFEREE && !IsTeamTheSame(Player[i][Team], Player[playerid][Team])) 
			continue; // Skip this player if he's in a different team than spectator's team
		
		if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
			continue; // Skip this player if he's got enough people spectating him
		
		if(SpectatePlayer(playerid, i) == 1)
			return; // Stop the loop AS WELL AS the function process when the spectate process is a success
	}
	// This will be reached if no available players to be spectated are found
	if(stop_spectate)	
	{
		StopSpectate(playerid); // Exit spectate mode
		return;
	}
	SpectateAnyPlayer(playerid, (Player[playerid][Team] == REFEREE) ? (false) : (true), false);
}

SpectateNextPlayer(playerid, bool:stop_spectate = false)
{
	// Calling Iter_Next inside loop comparison stage makes the starting index equal to the next player after spectator
	// Loop through all connected players ignoring players with IDs lower than spectator's
	for(new i = Player[playerid][IsSpectatingID]; (i = Iter_Next(Player, i)) != Iter_End(Player);)
	{	
		if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
			continue; // Skip this player if they're dead/wasted or not spawned yet
		
		if(Player[i][Spectating])
			continue; // Skip this player if they're spectating someone else
		
		if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
			continue; // Skip this player if he's got enough people spectating him
		
		if(SpectatePlayer(playerid, i) == 1)
			return; // Stop the loop AS WELL AS the function process when the spectate process is a success
	}
	// This will be reached if no available players to be spectated are found
	if(stop_spectate)	
	{
		StopSpectate(playerid); // Exit spectate mode
		return;
	}
	SpectateAnyPlayer(playerid, false, true);
}

SpectatePreviousPlayer(playerid, bool:stop_spectate = false) 
{
	// Calling Iter_Next inside loop comparison stage makes the starting index equal to the previous player before spectator
	// Loop through all connected players (descending loop: from highest ID to lowest)
	for(new i = Player[playerid][IsSpectatingID]; (i = Iter_Prev(Player, i)) != Iter_Begin(Player);) 
	{
		if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
			continue; // Skip this player if they're dead/wasted or not spawned yet
		
		if(Player[i][Spectating])
			continue; // Skip this player if they're spectating someone else
		
		if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
			continue; // Skip this player if he's got enough people spectating him
		
		if(SpectatePlayer(playerid, i) == 1)
			return; // Stop the loop AS WELL AS the function process when the spectate process is a success
	}
	// This will be reached if no available players to be spectated are found
	if(stop_spectate)	
	{
		StopSpectate(playerid); // Exit spectate mode
		return;
	}
	SpectateAnyPlayer(playerid, false, false);
}

// Called by timer from OnPlayerDeath
forward SpectateAnyPlayerT(playerid);
public SpectateAnyPlayerT(playerid)
{
	SpectateAnyPlayer(playerid, true, true);
}

forward RemoveSpectatorSafely(playerid);
public RemoveSpectatorSafely(playerid)
{
	printf("START:RemoveSpectatorSafely(%d)", playerid);
	Iter_Remove(AllSpectators, playerid);
	printf("END:RemoveSpectatorSafely(%d)", playerid);
	return 1;
}

SpectateAnyPlayer(playerid, bool:teammatch, bool:ascending, exception = -1)
{
	switch(teammatch)
	{
		case true:
		{
			switch(ascending)
			{
				case true:
				{
					foreach(new i : PlayersInRound)
					{
						if(i == exception)
							continue;
						
						if(!IsTeamTheSame(Player[playerid][Team], Player[i][Team]))
							continue;

						if(SpectatePlayer(playerid, i) == 1)
							return; // Stop the loop AS WELL AS the function process when the spectate process is a success
					}
				}
				case false:
				{
					foreach(new i : Reverse(PlayersInRound))
					{
						if(i == exception)
							continue;
						
						if(!IsTeamTheSame(Player[playerid][Team], Player[i][Team]))
							continue;
						
						if(SpectatePlayer(playerid, i) == 1)
							return; // Stop the loop AS WELL AS the function process when the spectate process is a success
					}
				}
			}
			// This will be reached if no available players to be spectated are found
			StopSpectate(playerid, false); // Exit spectate mode
			SetTimerEx("RemoveSpectatorSafely", 500, false, "i", playerid);
		}
		case false:
		{
			switch(ascending)
			{
				case true:
				{
					switch(Current)
					{
						case -1:
						{
							foreach(new i : Player)
							{
								if(i == exception)
									continue;
								
								if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
									continue; // Skip this player if they're dead/wasted or not spawned yet
								
								if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
									continue; // Skip this player if he's got enough people spectating him
								
								if(SpectatePlayer(playerid, i) == 1)
									return; // Stop the loop AS WELL AS the function process when the spectate process is a success
							}
						}
						default:
						{
							foreach(new i : PlayersInRound)
							{
								if(i == exception)
									continue;
								
								if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
									continue; // Skip this player if they're dead/wasted or not spawned yet
								
								if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
									continue; // Skip this player if he's got enough people spectating him
								
								if(SpectatePlayer(playerid, i) == 1)
									return; // Stop the loop AS WELL AS the function process when the spectate process is a success
							}
						}
					}
				}
				case false:
				{
					switch(Current)
					{
						case -1:
						{
							foreach(new i : Reverse(Player))
							{
								if(i == exception)
									continue;
								
								if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
									continue; // Skip this player if they're dead/wasted or not spawned yet
								
								if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
									continue; // Skip this player if he's got enough people spectating him
								
								if(SpectatePlayer(playerid, i) == 1)
									return; // Stop the loop AS WELL AS the function process when the spectate process is a success
							}
						}
						default:
						{
							foreach(new i : Reverse(PlayersInRound))
							{
								if(i == exception)
									continue;
								
								if(Player[i][Team] == NON || GetPlayerState(i) == PLAYER_STATE_WASTED)
									continue; // Skip this player if they're dead/wasted or not spawned yet
								
								if(Iter_Count(PlayerSpectators[i]) >= MAX_PLAYER_SPECTATORS)
									continue; // Skip this player if he's got enough people spectating him
								
								if(SpectatePlayer(playerid, i) == 1)
									return; // Stop the loop AS WELL AS the function process when the spectate process is a success
							}
						}
					}
				}
			}
			// This will be reached if no available players to be spectated are found
			StopSpectate(playerid, false); // Exit spectate mode
			SetTimerEx("RemoveSpectatorSafely", 500, false, "i", playerid);
		}
	}
}
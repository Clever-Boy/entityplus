// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if( !activator->client ) {
		return;
	}

	if( activator->client->ps.powerups[PW_REDFLAG] ) {
		Team_ReturnFlag( TEAM_RED );
	} else if( activator->client->ps.powerups[PW_BLUEFLAG] ) {
		Team_ReturnFlag( TEAM_BLUE );
	} else if( activator->client->ps.powerups[PW_NEUTRALFLAG] ) {
		Team_ReturnFlag( TEAM_FREE );
	}

	memset( activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t *ent ) {
	ent->use = Use_target_remove_powerups;
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent ) {
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	AddScore( activator, ent->r.currentOrigin, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if ( activator->client && ( ent->spawnflags & 4 ) ) {
		trap_SendServerCommand( activator-g_entities, va("cp \"%s\"", ent->message ));
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			G_TeamCommand( TEAM_RED, va("cp \"%s\"", ent->message) );
		}
		if ( ent->spawnflags & 2 ) {
			G_TeamCommand( TEAM_BLUE, va("cp \"%s\"", ent->message) );
		}
		return;
	}

	trap_SendServerCommand( -1, va("cp \"%s\"", ent->message ));
}

void SP_target_print( gentity_t *ent ) {
	ent->use = Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if (ent->spawnflags & 3) {	// looping sound toggles
		if (ent->s.loopSound)
			ent->s.loopSound = 0;	// turn it off
		else
			ent->s.loopSound = ent->noise_index;	// start it
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) ) {
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if ( s[0] == '*' ) {
		ent->spawnflags |= 8;
	}

	if (!strstr( s, ".wav" )) {
		Com_sprintf (buffer, sizeof(buffer), "%s.wav", s );
	} else {
		Q_strncpyz( buffer, s, sizeof(buffer) );
	}
	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think (gentity_t *self) {
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA (point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir, 
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy (tr.endpos, self->s.origin2);

	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			G_Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;

	if (!activator->client)
		return;
	dest = 	G_PickTarget( self->target );
	if (!dest) {
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if (!self->targetname)
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
A count key can be set to delay the triggering until the entity has been triggered [count] number of times
*/
void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	if ( ( self->spawnflags & 1 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}
	if ( self->spawnflags & 4 ) {
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			ent->use( ent, self, activator );
		}
		return;
	}

	if (!self->count || self->count < 0) {
		self->count = 1;
	}

	if (!self->damage)
		self->damage = 1;
	else
		self->damage++;

	if (self->damage == self->count)
	{
		G_UseTargets (self, activator);
		self->damage = 0;
	}
}

void SP_target_relay (gentity_t *self) {
	self->use = target_relay_use;
}

//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}

//==========================================================

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}

static void target_location_linkup(gentity_t *ent)
{
	int i;
	int n;

	if (level.locationLinked) 
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for (i = 0, ent = g_entities, n = 1;
			i < level.num_entities;
			i++, ent++) {
		if (ent->classname && !Q_stricmp(ent->classname, "target_location")) {
			// lets overload some variables!
			ent->health = n; // use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

//==========================================================

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ){
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}

//==========================================================

/*QUAKED target_logic (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM STAY_ON
This doesn't perform any actions except fire its targets when it's triggered by two different triggers.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
*/
void target_logic_reset (gentity_t *self) {
	int i;
	for (i = 0; i < MAX_LOGIC_ENTITIES; i++)
		self->logicEntities[i] = 0;
}

void target_logic_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	int			i;
	int			triggerCount;		//number of entities targetting this target_logic
	int			triggeredCount;		//number of entities targetting this target_logic that have already been triggered
	qboolean	found;
	gentity_t	*t;

	if ( ( self->spawnflags & 1 ) && activator->client && activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client && activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	//determine the number of entities (triggers) targeting this target_logic
	triggerCount = triggeredCount = 0;
	t = NULL;
	while ( (t = G_Find (t, FOFS(target), self->targetname)) != NULL ) {
		found = qfalse;
		if ( t == self ) {
			G_Printf ("WARNING: Entity used itself.\n");
		} else {
			triggerCount++;
		}
	}

	//count the number of entities that have already triggered the target_logic
	for ( i = 0; i < MAX_LOGIC_ENTITIES; i++ ) {
		if ( self->logicEntities[i] ) {
			triggeredCount++;

			//if the entity that is currently being triggered is in the list, see if we should remove it again
			if ( self->logicEntities[i] == other->s.number ) {
				found = qtrue;
				
				//STAY_ON is not selected, remove the trigger for the list of triggered entities
				if ( !(self->spawnflags & 8) ) {
					self->logicEntities[i] = 0;
					triggeredCount--;		//the trigger was counted as being triggerd, but is not so anymore
				}
			}
		}
	}

	//the entity was not yet in the list of triggered entities, so add it
	if ( !found ) {
		for ( i = 0; i < MAX_LOGIC_ENTITIES; i++ ) {
			if ( !self->logicEntities[i] )
			{
				self->logicEntities[i] = other->s.number;
				triggeredCount++;
				break;
			}
		}
	}

	if ( triggerCount == triggeredCount ) {
		target_logic_reset( self );

		//If RANDOM is selected so use a random target
		if ( self->spawnflags & 4 ) {
			gentity_t	*ent;

			ent = G_PickTarget( self->target );
			if ( ent && ent->use ) {
				ent->use( ent, self, activator );
			}
			return;
		}
		
		//The RANDOM spawnflag wasn't selected, so use all targets
		G_UseTargets (self, activator);
	}
}



void SP_target_logic (gentity_t *self) {
	self->use = target_logic_use;
	target_logic_reset( self );
}

//==========================================================

/*QUAKED target_mapchange (.5 .5 .5) (-8 -8 -8) (8 8 8) SHOW_INTERMISSION SCRIPT
When triggered, loads the specified map. 
When the SHOW_INTERMISSION spawnflag is set, the intermission screen is displayed before loading the next map.
*/
void target_mapchange_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	char	*cmd;

	//store session data to persist health/armor/weapons/ammo to next level (only in SP mode)
	if ( g_gametype.integer == GT_ENTITYPLUS )
		G_UpdateSessionDataForMapChange( activator->client );

	//determine map switch command to use
	if ( self->spawnflags & 2 )
		cmd = "exec";		//a cfg script will be executed instead
	else if ( g_gametype.integer == GT_ENTITYPLUS )
		cmd = "spmap";		//stay in single player mode
	else if ( g_cheats.integer )
		cmd = "devmap";		//keep cheats enabled
	else
		cmd = "map";

	//perform map switch/script execution
	if ( ( self->spawnflags & 1 ) )
	{
		if ( self->mapname ) {
			if ( self->spawnflags & 2 )
				trap_SendConsoleCommand( EXEC_INSERT, va( "exec %s\n", self->mapname ) ); 
			else
				trap_SendConsoleCommand( EXEC_INSERT, va( "nextmap \"%s %s\"\n", cmd, self->mapname ) ); 
		}
		
		BeginIntermission();
	} else {
		G_Printf("%s %s\n", cmd, self->mapname);
		if ( self->mapname )
			trap_SendConsoleCommand( EXEC_INSERT, va( "%s %s\n", cmd, self->mapname ) ); 
		else
			trap_SendConsoleCommand( EXEC_INSERT, "map_restart 0\n" ); //shouldn't happen
	}
}

void SP_target_mapchange (gentity_t *self) {
	char info[1024];

	if ( !self->mapname )
	{
		trap_GetServerinfo(info, sizeof(info));
		self->mapname = Info_ValueForKey( info, "mapname" );
	}
	self->use = target_mapchange_use;
}

//==========================================================

/*QUAKED target_gravity (.5 .5 .5) (-8 -8 -8) (8 8 8) GLOBAL
Sets the gravity of the activator. The gravity is set through the "count" key.
If GLOBAL is checked, all players in the game will have their gravity changed.
*/
void target_gravity_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	int i;

	if ( !self->count )
		self->count = g_gravity.integer;
	
	if ( (self->spawnflags & 1) )
	{
		for (i = 0; i < level.maxclients; i++)
		{
			level.clients[i].ps.gravity = self->count;
		}
	}
	else
		activator->client->ps.gravity = self->count;
}

void SP_target_gravity (gentity_t *self) {
	self->use = target_gravity_use;
}

//==========================================================

/*QUAKED target_botspawn (.5 .5 .5) (-8 -8 -8) (8 8 8) 
OPPOSING_TEAM WP_MACHINEGUN WP_SHOTGUN WP_GRENADE_LAUNCHER WP_ROCKET_LAUNCHER WP_LIGHTNING WP_RAILGUN WP_PLASMAGUN WP_BFG
Spawns a bot into the game
Use the health key to determine the amount of health the bot will spawn with
The entity specified with deathtarget will be activated when the spawned bot dies
Use the skill key to specify the skill level for the bot relative to the g_spskill level. This is only applied to the amount of damage it deals.
*/
void target_botspawn_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	char *team;
	int teamnum;
	float skill;

	switch (activator->client->sess.sessionTeam) {
		case TEAM_BLUE:
			if (( self->spawnflags & 1 )) {
				team = "red";
				teamnum = TEAM_RED;
			} else {
				team = "blue";
				teamnum = TEAM_BLUE;
			}
			break;
		case TEAM_RED:
			if (( self->spawnflags & 1 )) {
				team = "blue";
				teamnum = TEAM_BLUE;
			} else {
				team = "red";
				teamnum = TEAM_RED;
			}
			break;
		default:
			team = "free";
			teamnum = TEAM_FREE;
			break;
	}

	G_AddCustomBot( self->clientname, self->s.number, self->target );
}

void SP_target_botspawn (gentity_t *self) {
	if ( !self->clientname || !strcmp(self->clientname, "") )
		self->clientname = "sarge";

	G_SpawnFloat( "skill", "0", &self->skill );

	self->use = target_botspawn_use;
}

//==========================================================

/*QUAKED target_disable (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY ALWAYS_DISABLE ALWAYS_ENABLE IMMEDIATELY
links or unlinks entities from the world, effectively enabling or disabling triggers
*/
void target_disable_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	if ( ( self->spawnflags & 1 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	G_ToggleTargetsEnabled ( self );
}

//used for immediately spawnflag
void target_disable_think (gentity_t *self) {
	self->nextthink = 0;
	G_ToggleTargetsEnabled ( self );
}

void SP_target_disable (gentity_t *self) {
	self->use = target_disable_use;
	
	if ( ( self->spawnflags & 16 ) ) {
		self->nextthink = level.time + 300;
		self->think = target_disable_think;
	}
}

//==========================================================

/*QUAKED target_playerspeed (.5 .5 .5) (-8 -8 -8) (8 8 8) GLOBAL
Sets the movement speed for player(s). Defaults to the speed set through the g_speed cvar (320 by default).
*/
void target_playerspeed_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	int i;

	if ( !self->speed ) {
		self->speed = g_speed.value;
	}

	activator->speed = self->speed;	//this doesn't actually change the player's speed. This value is read in ClientThink_real (active.c) again.
}

void SP_target_playerspeed (gentity_t *self) {
	self->use = target_playerspeed_use;
}

//==========================================================

/*QUAKED target_debrisemitter (.5 .5 .5) (-8 -8 -8) (8 8 8) see PickDebrisType in g_util.c for spawnflags
Emits chunks of debris.
If no spawnflag is set, the entity will emit light chunks of concrete
*/

void target_debrisemitter_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	gentity_t *ent;
	
	if ( !self->count )
		self->count = 10;

	ent = G_TempEntity( self->s.origin, PickDebrisType( self->spawnflags ) );
	ent->s.eventParm = self->count;
}

void SP_target_debrisemitter (gentity_t *self) {
	self->use = target_debrisemitter_use;
}

//==========================================================

/*QUAKED target_objective (.5 .5 .5) (-8 -8 -8) (8 8 8) SECONDARY
Sets the textual representation of the player's objective.
Set the SECONDARY spawnflag to set the secondary objective instead of the primary.
*/

void target_objective_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	
	if ( self->spawnflags & 1 )
		trap_SetConfigstring( CS_SECONDARYOBJECTIVE, self->message );
	else
		trap_SetConfigstring( CS_PRIMARYOBJECTIVE, self->message );

	trap_SendServerCommand( -1, va("cp \"%s\"", "Objectives updated" ));
}

void SP_target_objective (gentity_t *self) {
	self->use = target_objective_use;
}

//==========================================================

/*QUAKED target_skill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Sets the skill level for the next map that will be loaded
*/

void target_skill_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	trap_SendConsoleCommand( EXEC_INSERT, va( "seta g_spskill %i\n", self->skill) );
}

void SP_target_skill (gentity_t *self) {
	G_SpawnFloat( "skill", "2", &self->skill );

	if ( self->skill > 5 ) 
		self->skill = 5;
	else if ( self->skill < 1 )
		self->skill = 1;
	
	self->use = target_skill_use;
}

//==========================================================

/*QUAKED target_earthquake (.5 .5 .5) (-8 -8 -8) (8 8 8)
starts earthquake
"length" - length in  seconds (2-32, in steps of 2)
"intensity" - strength of earthquake (1-16)
*/

void target_earthquake_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	G_AddEvent(self, EV_EARTHQUAKE, self->s.generic1);
}

void SP_target_earthquake (gentity_t *self) {
	int param;
	float length;		// length in seconds (2 to 32)
	float intensity;	// intensity (1 to 16)
	int length_;
	int intensity_;
	
	// read parameters
	G_SpawnFloat( "length", "1000", &length );
	G_SpawnFloat( "intensity", "50", &intensity );
	if ( length < 2 ) length = 2;
	if ( length > 32 ) length = 32;
	if ( intensity < 1 ) intensity = 1;
	if ( intensity > 16 ) intensity = 16;
	
	// adjust parameters
	length_ = ((int)(length) - 2) / 2;
	intensity_ = (int)intensity - 1;
	param = ( intensity_ | (length_ << 4) );
	self->s.generic1 = param;
	self->use = target_earthquake_use;
	self->s.eType = ET_EVENTS;
	trap_LinkEntity (self);
}

//==========================================================

/*QUAKED target_effect (.5 .5 .5) (-8 -8 -8) (8 8 8) EXPLOSION PARTICLES_GRAVITY PARTICLES_LINEAR PARTICLES_LINEAR_UP PARTICLES_LINEAR_DOWN
shows animated environmental effect
The EXPLOSION spawnflag will cause the entity to show an explosion
The PARTICLES_GRAVITY spawnflag will cause the entity to show particles which are affected by gravity and drop to the ground
The PARTICLES_LINEAR spawnflag will cause the entity to show particles which are not affected by gravity and move in a straight line
color key takes normalized color values (0.0 - 1.0)
count key takes int (0 - 255)
speed key takes int (0 - 255)
*/

void target_effect_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	gentity_t	*ent;
	gentity_t	*ent2;
	gentity_t	*ent3;
	gentity_t	*ent4;
	gentity_t	*ent5;

	//set default values
	if ( !self->count )
		self->count = 100;
	else if (self->count > 255)
		self->count = 255;

	if ( !self->speed )
		self->speed = 100;

	//explosion
	if ( self->spawnflags & 1 ) {
		ent = G_TempEntity( self->s.origin, EV_EXPLOSION );
	}

	//particles_gravity
	if ( self->spawnflags & 2 ) {
		ent2 = G_TempEntity( self->s.origin, EV_PARTICLES_GRAVITY );
		ent2->s.constantLight = self->s.constantLight;	//constantLight is used to determine particle color
		ent2->s.eventParm = self->count; //eventParm is used to determine the number of particles
		ent2->s.generic1 = self->speed; //generic1 is used to determine the speed of the particles
	}

	//particles_linear
	if ( self->spawnflags & 4 ) {
		ent3 = G_TempEntity( self->s.origin, EV_PARTICLES_LINEAR );
		ent3->s.constantLight = self->s.constantLight;	//constantLight is used to determine particle color
		ent3->s.eventParm = self->count; //eventParm is used to determine the number of particles
		ent3->s.generic1 = self->speed; //generic1 is used to determine the speed of the particles
	}

	//particles_linear_up
	if ( self->spawnflags & 8 ) {
		ent4 = G_TempEntity( self->s.origin, EV_PARTICLES_LINEAR_UP );
		ent4->s.constantLight = self->s.constantLight;	//constantLight is used to determine particle color
		ent4->s.eventParm = self->count; //eventParm is used to determine the number of particles
		ent4->s.generic1 = self->speed; //generic1 is used to determine the speed of the particles
	}

	//particles_linear_down
	if ( self->spawnflags & 16 ) {
		ent5 = G_TempEntity( self->s.origin, EV_PARTICLES_LINEAR_DOWN );
		ent5->s.constantLight = self->s.constantLight;	//constantLight is used to determine particle color
		ent5->s.eventParm = self->count; //eventParm is used to determine the number of particles
		ent5->s.generic1 = self->speed; //generic1 is used to determine the speed of the particles
	}
}

void SP_target_effect (gentity_t *self) {
	vec3_t		color;
	int			r, g, b;

	//check if effects are selected
	if ( !self->spawnflags ) {
		G_Printf( va( S_COLOR_YELLOW "WARNING: target_effect without selected effects at %s\n", vtos(self->s.origin) ) );
		G_FreeEntity( self );
	}
	
	// particle color
	G_SpawnVector( "color", "1 1 1", color );

	r = color[0] * 255;
	if (r > 255) r = 255;

	g = color[1] * 255;
	if (g > 255) g = 255;

	b = color[2] * 255;
	if (b > 255) b = 255;
	
	self->s.constantLight = r + (g << 8) + (b << 16);

	//preload assets if necessary
	if ( self->spawnflags & 1 ) {
		RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );	//uses RL gfx so we must register the RL
	}

	self->use = target_effect_use;
}

/*
	structures shared between etdos & plugins
*/

#ifndef __SHARED_H
#define __SHARED_H

typedef int qboolean;
typedef unsigned char byte;

typedef float vec_t;
typedef vec_t vec2_t[3];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

#define qtrue 1
#define qfalse 0
#define MAX_STRING_TOKENS 1024
#define MAX_STRING_CHARS 1024
#define MAX_INFO_STRING 1024
#define	MAX_RELIABLE_COMMANDS 64
#define MAX_OSPATH 256
#define MAX_QPATH 256

#define	MAX_CLIENTS 64
#define	GENTITYNUM_BITS 10
#define	MAX_GENTITIES (1<<GENTITYNUM_BITS)
#define	ENTITYNUM_NONE (MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD (MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL (MAX_GENTITIES-2)

#define	MAX_PARSE_ENTITIES 2048
#define	MAX_CONFIGSTRINGS 1024
#define	CS_SERVERINFO		0
#define	CS_SYSTEMINFO		1
#define CS_PLAYERS			689
#define	RESERVED_CONFIGSTRINGS	2
#define	MAX_GAMESTATE_CHARS	16000

typedef struct {
	int			stringOffsets[MAX_CONFIGSTRINGS];
	char		stringData[MAX_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;

typedef struct usercmd_s {
	int		serverTime;
	byte	buttons;
	byte	wbuttons;
	byte	weapon;
	byte	flags;
	int		angles[3];
	signed char	forwardmove, rightmove, upmove;
	byte	doubleTap;
	byte	identClient;
} usercmd_t;

#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_HOLDABLE			16
#define	MAX_POWERUPS			16
#define	MAX_MAP_AREA_BYTES		32
#define	MAX_WEAPONS				64
#define	MAX_EVENTS				4

typedef struct playerState_s {
	int			commandTime;
	int			pm_type;
	int			bobCycle;
	int			pm_flags;
	int			pm_time;
	vec3_t		origin;
	vec3_t		velocity;
	int			weaponTime;
	int			weaponDelay;
	int			grenadeTimeLeft;
	int			gravity;
	float		leanf;
	int			speed;
	int			delta_angles[3];
	int			groundEntityNum;
	int			legsTimer;
	int			legsAnim;
	int			torsoTimer;
	int			torsoAnim;
	int			movementDir;
	int			eFlags;
	int			eventSequence;
	int			events[MAX_EVENTS];
	int			eventParms[MAX_EVENTS];
	int			oldEventSequence;
	int			externalEvent;
	int			externalEventParm;
	int			externalEventTime;
	int			clientNum;
	int			weapon;
	int			weaponstate;
	int			item;
	vec3_t		viewangles;
	int			viewheight;
	int			damageEvent;
	int			damageYaw;
	int			damagePitch;
	int			damageCount;
	int			stats[MAX_STATS];
	int			persistant[MAX_PERSISTANT];
	int			powerups[MAX_POWERUPS];
	int			ammo[MAX_WEAPONS];
	int			ammoclip[MAX_WEAPONS];
	int			holdable[16];
	int			holding;
	int			weapons[MAX_WEAPONS/(sizeof(int)*8)];
	vec3_t		mins, maxs;
	float		crouchMaxZ;
	float		crouchViewHeight, standViewHeight, deadViewHeight;
	float		runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	int			viewlocked;
	int			viewlocked_entNum;
	float		friction;
	int			nextWeapon;
	int			teamNum;
	int			onFireStart;
	int			serverCursorHint;
	int			serverCursorHintVal;
	int			ping;
	int			pmove_framecount;
	int			entityEventSequence;
	int			sprintExertTime;
	int			classWeaponTime;
	int			jumpTime;
	int			weapAnim;
	qboolean	releasedFire;
	float		aimSpreadScaleFloat;
	int			aimSpreadScale;
	int			lastFireTime;
	int			quickGrenTime;
	int			leanStopDebounceTime;
	int			weapHeat[MAX_WEAPONS];
	int			curWeapHeat;
	int			identifyClient;
	int			identifyClientHealth;
	int			aiState;
} playerState_t;

typedef struct {
	qboolean		valid;
	int				snapFlags;
	int				serverTime;
	int				messageNum;
	int				deltaNum;
	int				ping;
	byte			areamask[MAX_MAP_AREA_BYTES];
	int				cmdNum;
	playerState_t	ps;
	int				numEntities;
	int				parseEntitiesNum;
	int				serverCommandNum;
} clSnapshot_t;

typedef struct {
	int 	trType;
	int		trTime;
	int		trDuration;
	vec3_t	trBase;
	vec3_t	trDelta;
} trajectory_t;

typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_CONCUSSIVE_TRIGGER,
	ET_OID_TRIGGER,
	ET_EXPLOSIVE_INDICATOR,
	ET_EXPLOSIVE,
	ET_EF_SPOTLIGHT,
	ET_ALARMBOX,
	ET_CORONA,
	ET_TRAP,
	ET_GAMEMODEL,
	ET_FOOTLOCKER,
	ET_FLAMEBARREL,
	ET_FP_PARTS,
	ET_FIRE_COLUMN,
	ET_FIRE_COLUMN_SMOKE,
 	ET_RAMJET,
	ET_FLAMETHROWER_CHUNK,
	ET_EXPLO_PART,
	ET_PROP,
	ET_AI_EFFECT,
	ET_CAMERA,
	ET_MOVERSCALED,
	ET_CONSTRUCTIBLE_INDICATOR,
	ET_CONSTRUCTIBLE,
	ET_CONSTRUCTIBLE_MARKER,
	ET_BOMB,
	ET_WAYPOINT,
	ET_BEAM_2,
	ET_TANK_INDICATOR,
	ET_TANK_INDICATOR_DEAD,
	ET_BOTGOAL_INDICATOR,
	ET_CORPSE,
	ET_SMOKER,
	ET_TEMPHEAD,
	ET_MG42_BARREL,
	ET_TEMPLEGS,
	ET_TRIGGER_MULTIPLE,
	ET_TRIGGER_FLAGONLY,
	ET_TRIGGER_FLAGONLY_MULTIPLE,
	ET_GAMEMANAGER,
	ET_AAGUN,
	ET_CABINET_H,
	ET_CABINET_A,
	ET_HEALER,
	ET_SUPPLIER,
	ET_LANDMINE_HINT,
	ET_ATTRACTOR_HINT,
	ET_SNIPER_HINT,
	ET_LANDMINESPOT_HINT,

	ET_COMMANDMAP_MARKER,
	ET_WOLF_OBJECTIVE,
	ET_EVENTS
} entityType_t;

typedef struct entityState_s {
	int				number;
	entityType_t	eType;
	int				eFlags;
	trajectory_t	pos;
	trajectory_t	apos;
	int		time;
	int		time2;
	vec3_t	origin;
	vec3_t	origin2;
	vec3_t	angles;
	vec3_t	angles2;
	int		otherEntityNum;
	int		otherEntityNum2;
	int		groundEntityNum;
	int		constantLight;
	int		dl_intensity;
	int		loopSound;
	int		modelindex;
	int		modelindex2;
	int		clientNum;
	int		frame;
	int		solid;
	int		event;
	int		eventParm;
	int		eventSequence;
	int		events[MAX_EVENTS];
	int		eventParms[MAX_EVENTS];
	int		powerups;
	int		weapon;
	int		legsAnim;
	int		torsoAnim;
	int		density;
	int		dmgFlags;
	int		onFireStart, onFireEnd;
	int		nextWeapon;
	int		teamNum;
	int		effect1Time, effect2Time, effect3Time;
	int		aiState;
	int		animMovetype;
} entityState_t;

#endif

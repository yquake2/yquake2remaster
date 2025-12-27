/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Main header file for the game module.
 *
 * =======================================================================
 */

#ifndef GAME_LOCAL_H
#define GAME_LOCAL_H

#include "../../common/header/shared.h"

/* define GAME_INCLUDE so that game.h does not define the
   short, server-visible gclient_t and edict_t structures,
   because we define the full size ones in this file */
#define GAME_INCLUDE
#include "game.h"

/* the "gameversion" client command will print this plus compile date */
#define GAMEVERSION "baseq2"

/* protocol bytes that can be directly added to messages */
#define svc_muzzleflash 1
#define svc_muzzleflash2 2
#define svc_temp_entity 3
#define svc_layout 4
#define svc_inventory 5
#define svc_stufftext 11
#define svc_fog 21

/* ================================================================== */

/* view pitching times */
#define DAMAGE_TIME 0.5
#define FALL_TIME 0.3

/* these are set with checkboxes on each entity in the map editor */

/* edict->spawnflags */
#define SPAWNFLAG_NOT_EASY 0x00000100
#define SPAWNFLAG_NOT_MEDIUM 0x00000200
#define SPAWNFLAG_NOT_HARD 0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH 0x00000800
#define SPAWNFLAG_NOT_COOP 0x00001000

/* edict->flags */
#define FL_FLY 0x00000001
#define FL_SWIM 0x00000002                  /* implied immunity to drowining */
#define FL_IMMUNE_LASER 0x00000004
#define FL_INWATER 0x00000008
#define FL_GODMODE 0x00000010
#define FL_NOTARGET 0x00000020
#define FL_IMMUNE_SLIME 0x00000040
#define FL_IMMUNE_LAVA 0x00000080
#define FL_PARTIALGROUND 0x00000100         /* not all corners are valid */
#define FL_WATERJUMP 0x00000200             /* player jumping out of water */
#define FL_TEAMSLAVE 0x00000400             /* not the first on the team */
#define FL_NO_KNOCKBACK 0x00000800
#define FL_POWER_ARMOR 0x00001000           /* power armor (if any) is active */
#define FL_COOP_TAKEN 0x00002000            /* Another client has already taken it */
#define FL_RESPAWN 0x80000000               /* used for item respawning */

#define FL_MECHANICAL 0x00002000            /* entity is mechanical, use sparks not blood */
#define FL_SAM_RAIMI 0x00004000             /* entity is in sam raimi cam mode */
#define FL_DISGUISED 0x00008000             /* entity is in disguise, monsters will not recognize. */
#define FL_NOGIB 0x00010000                 /* player has been vaporized by a nuke, drop no gibs */
#define FL_FLASHLIGHT 0x00020000            /* enable flashlight */

#define FRAMETIME 0.1

/* memory tags to allow dynamic memory to be cleaned up */
#define TAG_GAME 765        /* clear when unloading the dll */
#define TAG_LEVEL 766       /* clear when loading a new level */

#define MELEE_DISTANCE 80
#define BODY_QUEUE_SIZE 8
#define STEPSIZE 18

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,         /* will take damage if hit */
	DAMAGE_AIM          /* auto targeting recognizes this */
} damage_t;

typedef enum
{
	WEAPON_READY,
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

typedef enum
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS,
	AMMO_MAGSLUG,
	AMMO_TRAP,
	AMMO_FLECHETTES,
	AMMO_TESLA,
	AMMO_PROX,
	AMMO_DISRUPTOR
} ammo_t;

/* Maximum debris / gibs per frame */
#define MAX_GIBS 20
#define MAX_DEBRIS 20

/* deadflag */
#define DEAD_NO 0
#define DEAD_DYING 1
#define DEAD_DEAD 2
#define DEAD_RESPAWNABLE 3

/* range */
#define RANGE_MELEE 0
#define RANGE_NEAR 1
#define RANGE_MID 2
#define RANGE_FAR 3

/* gib types */
typedef enum
{
	GIB_NONE,
	GIB_ORGANIC,
	GIB_METALLIC,
	GIB_STONE,
	GIB_GREYSTONE,
	GIB_CLOTH,
	GIB_POTTERY,
	GIB_GLASS,
	GIB_LEAF,
	GIB_WOOD,
	GIB_BROWNSTONE,
	GIB_INSECT,
} gibtype_t;

/* monster ai flags */
#define AI_STAND_GROUND 0x00000001
#define AI_TEMP_STAND_GROUND 0x00000002
#define AI_SOUND_TARGET 0x00000004
#define AI_LOST_SIGHT 0x00000008
#define AI_PURSUIT_LAST_SEEN 0x00000010
#define AI_PURSUE_NEXT 0x00000020
#define AI_PURSUE_TEMP 0x00000040
#define AI_HOLD_FRAME 0x00000080
#define AI_GOOD_GUY 0x00000100
#define AI_BRUTAL 0x00000200
#define AI_NOSTEP 0x00000400
#define AI_DUCKED 0x00000800
#define AI_COMBAT_POINT 0x00001000
#define AI_MEDIC 0x00002000
#define AI_RESURRECTING 0x00004000
#define AI_IGNORE_PAIN 0x00008000

/* ROGUE */
#define AI_WALK_WALLS 0x00008000
#define AI_MANUAL_STEERING 0x00010000
#define AI_TARGET_ANGER 0x00020000
#define AI_DODGING 0x00040000
#define AI_CHARGING 0x00080000
#define AI_HINT_PATH 0x00100000
#define AI_IGNORE_SHOTS 0x00200000
#define AI_DO_NOT_COUNT 0x00400000          /* set for healed monsters */
#define AI_SPAWNED_CARRIER 0x00800000       /* both do_not_count and spawned are set for spawned monsters */
#define AI_SPAWNED_MEDIC_C 0x01000000       /* both do_not_count and spawned are set for spawned monsters */
#define AI_SPAWNED_WIDOW 0x02000000         /* both do_not_count and spawned are set for spawned monsters */
#define AI_SPAWNED_MASK 0x03800000          /* mask to catch all three flavors of spawned */
#define AI_BLOCKED 0x04000000               /* used by blocked_checkattack: set to say I'm attacking while blocked */
                                            /* (prevents run-attacks) */
/* monster attack state */
#define AS_STRAIGHT 1
#define AS_SLIDING 2
#define AS_MELEE 3
#define AS_MISSILE 4
#define AS_BLIND 5

/* armor types */
#define ARMOR_NONE 0
#define ARMOR_JACKET 1
#define ARMOR_COMBAT 2
#define ARMOR_BODY 3
#define ARMOR_SHARD 4

/* power armor types */
#define POWER_ARMOR_NONE 0
#define POWER_ARMOR_SCREEN 1
#define POWER_ARMOR_SHIELD 2

/* handedness values */
#define RIGHT_HANDED 0
#define LEFT_HANDED 1
#define CENTER_HANDED 2

/* game.serverflags values */
#define SFL_CROSS_TRIGGER_1 0x00000001
#define SFL_CROSS_TRIGGER_2 0x00000002
#define SFL_CROSS_TRIGGER_3 0x00000004
#define SFL_CROSS_TRIGGER_4 0x00000008
#define SFL_CROSS_TRIGGER_5 0x00000010
#define SFL_CROSS_TRIGGER_6 0x00000020
#define SFL_CROSS_TRIGGER_7 0x00000040
#define SFL_CROSS_TRIGGER_8 0x00000080
#define SFL_CROSS_TRIGGER_MASK 0x000000ff

/* noise types for PlayerNoise */
#define PNOISE_SELF 0
#define PNOISE_WEAPON 1
#define PNOISE_IMPACT 2

/* edict->movetype values */
typedef enum
{
	MOVETYPE_NONE,      /* never moves */
	MOVETYPE_NOCLIP,    /* origin and angles change with no interaction */
	MOVETYPE_PUSH,      /* no clip to world, push on box contact */
	MOVETYPE_STOP,      /* no clip to world, stops on box contact */

	MOVETYPE_WALK,      /* gravity */
	MOVETYPE_STEP,      /* gravity, special edge handling */
	MOVETYPE_FLY,
	MOVETYPE_TOSS,      /* gravity */
	MOVETYPE_FLYMISSILE, /* extra size to monsters */
	MOVETYPE_BOUNCE, /* added this (the comma at the end of line) */
	MOVETYPE_WALLBOUNCE,
	MOVETYPE_NEWTOSS    /* for deathball */
} movetype_t;

typedef struct
{
	int base_count;
	int max_count;
	float normal_protection;
	float energy_protection;
	int armor;
} gitem_armor_t;

/* gitem_t->flags */
#define IT_WEAPON 0x00000001                /* use makes active weapon */
#define IT_AMMO 0x00000002
#define IT_ARMOR 0x00000004
#define IT_STAY_COOP 0x00000008
#define IT_KEY 0x00000010
#define IT_POWERUP 0x00000020
#define IT_MELEE 0x00000040
#define IT_NOT_GIVEABLE 0x00000080      /* item can not be given */
#define IT_INSTANT_USE 0x000000100		/* item is insta-used on pickup if dmflag is set */
#define IT_TECH 0x000000200 /* CTF */

/* gitem_t->weapmodel for weapons indicates model index */
#define IT_HEALTH 0x000000400 /* JABot */
#define IT_FLAG	0x000000800 /* JABot */

/* gitem_t->weapmodel for weapons indicates model index,
 * update MAX_CLIENTWEAPONMODELS on client side if changed */
typedef enum
{
	WEAP_NONE,
	WEAP_BLASTER,
	WEAP_SHOTGUN,
	WEAP_SUPERSHOTGUN,
	WEAP_MACHINEGUN,
	WEAP_CHAINGUN,
	WEAP_GRENADES,
	WEAP_GRENADELAUNCHER,
	WEAP_ROCKETLAUNCHER,
	WEAP_HYPERBLASTER,
	WEAP_RAILGUN,
	WEAP_BFG,
	WEAP_PHALANX,
	WEAP_BOOMER,
	WEAP_DISRUPTOR,
	WEAP_ETFRIFLE,
	WEAP_PLASMA,
	WEAP_PROXLAUNCH,
	WEAP_CHAINFIST,
	WEAP_GRAPPLE,
	WEAP_FLAREGUN,
	WEAP_BETA_DISRUPTOR,
	WEAP_TOTAL
} weapmodel_t;

typedef struct gitem_s
{
	char *classname; /* spawning name */
	qboolean (*pickup)(struct edict_s *ent, struct edict_s *other);
	void (*use)(struct edict_s *ent, struct gitem_s *item);
	void (*drop)(struct edict_s *ent, struct gitem_s *item);
	void (*weaponthink)(struct edict_s *ent);
	char *pickup_sound;
	char *world_model;
	int world_model_flags;
	char *view_model;

	/* client side info */
	char *icon;
	char *pickup_name;          /* for printing on pickup */
	int count_width;            /* number of digits to display by icon */

	int quantity;               /* for ammo how much, for weapons how much is used per shot */
	char *ammo;                 /* for weapons */
	int flags;                  /* IT_* flags */

	weapmodel_t weapmodel;      /* weapon model index (for weapons) */

	void *info;
	int tag;

	char *precaches;            /* string of all models, sounds, and images this item will use */
} gitem_t;

/* this structure is left intact through an entire game
   it should be initialized at dll load time, and read/written to
   the server.ssv file for savegames */
typedef struct
{
	char helpmessage1[512];
	char helpmessage2[512];
	int helpchanged;            /* flash F1 icon if non 0, play sound */
	                            /* and increment only if 1, 2, or 3 */

	gclient_t *clients;         /* [maxclients] */

	/* can't store spawnpoint in level, because
	   it would get overwritten by the savegame
	   restore */
	char spawnpoint[512];       /* needed for coop respawns */

	/* store latched cvars here that we want to get at often */
	int maxclients;
	int maxentities;

	/* cross level triggers */
	int serverflags;

	/* items */
	int num_items;

	qboolean autosaved;
} game_locals_t;

/* this structure is cleared as each map is entered
   it is read/written to the level.sav file for savegames */
typedef struct
{
	int framenum;
	float time;

	char level_name[MAX_QPATH];         /* the descriptive name (Outer Base, etc) */
	char mapname[MAX_QPATH];            /* the server name (base1, etc) */
	char nextmap[MAX_QPATH];            /* go here when fraglimit is hit */
	char forcemap[MAX_QPATH];           /* go here */

	/* intermission state */
	float intermissiontime;             /* time the intermission was started */
	char *changemap;
	int exitintermission;
	vec3_t intermission_origin;
	vec3_t intermission_angle;

	edict_t *sight_client;			     /* changed once each frame for coop games */

	edict_t *sight_entity;
	int sight_entity_framenum;
	edict_t *sound_entity;
	int sound_entity_framenum;
	edict_t *sound2_entity;
	int sound2_entity_framenum;

	int pic_health;

	int total_secrets;
	int found_secrets;

	int total_goals;
	int found_goals;

	int total_monsters;
	int killed_monsters;

	edict_t *current_entity;        /* entity running from G_RunFrame */
	int body_que;                   /* dead bodies */

	int power_cubes;                /* ugly necessity for coop */
	int shadow_light_count;          /* number of shadow lights in level */

	edict_t *disguise_violator;
	int disguise_violation_framenum;

	char *start_items;             /* level start items */
	float next_auto_save;          /* target_autosave */
} level_locals_t;

/* shadow light data structures */
typedef enum
{
	SHADOW_LIGHT_POINT = 0,
	SHADOW_LIGHT_CONE  = 1
} shadow_light_type_t;

typedef struct
{
	int lighttype; /* shadow_light_type_t */
	float radius;
	int resolution;
	float intensity;
	float fade_start;
	float fade_end;
	int lightstyle;
	float coneangle;
	float conedirection[3];
} shadow_light_data_t;

/* shadow light helpers */
void setup_shadow_lights(void);
void G_LoadShadowLights(void);

/* spawn_temp_t is only used to hold entity field values that
   can be set from the editor, but aren't actualy present/
   in edict_t during gameplay */
typedef struct
{
	/* world vars */
	char *sky;
	float skyrotate;
	int skyautorotate;
	vec3_t skyaxis;
	char *nextmap;
	char *music;

	int lip;
	int distance;
	int height;
	char *noise;
	float pausetime;
	char *item;
	char *gravity;
	char *start_items;

	float minyaw;
	float maxyaw;
	float minpitch;
	float maxpitch;

	/* misc_flare */
	float radius;
	float fade_start_dist;
	float fade_end_dist;
	char *image;
	unsigned rgba;
	char *goals;
	int effects;
	int renderfx;
	/* shadow/light specific spawn fields */
	float sl_radius;           /* shadow map resolution */
	int sl_resolution;         /* shadow map resolution */
	float sl_intensity;        /* shadow light intensity */
	float sl_fade_start;       /* start fade distance */
	float sl_fade_end;         /* end fade distance */
	int sl_lightstyle;         /* index to bind lightstyle */
	float sl_coneangle;        /* _cone key for spotlights */
	char *sl_lightstyletarget; /* target used to bind a lightstyle */

	/* Addional fields for models */
	vec3_t scale;
	float health_multiplier;

	int weight;//JABot
} spawn_temp_t;

typedef struct
{
	/* fixed data */
	vec3_t start_origin;
	vec3_t start_angles;
	vec3_t end_origin;
	vec3_t end_angles;

	int sound_start;
	int sound_middle;
	int sound_end;

	float accel;
	float speed;
	float decel;
	float distance;

	float wait;

	/* state data */
	int state;
	vec3_t dir;
	float current_speed;
	float move_speed;
	float next_speed;
	float remaining_distance;
	float decel_distance;
	void (*endfunc)(edict_t *);
} moveinfo_t;

typedef struct
{
	void (*aifunc)(edict_t *self, float dist);
	float dist;
	void (*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct
{
	int firstframe;
	int lastframe;
	mframe_t *frame;
	void (*endfunc)(edict_t *self);
} mmove_t;

typedef struct
{
	mmove_t *currentmove;
	unsigned int aiflags;           /* unsigned, since we're close to the max */
	int nextframe;
	float scale;

	void (*stand)(edict_t *self);
	void (*idle)(edict_t *self);
	void (*search)(edict_t *self);
	void (*walk)(edict_t *self);
	void (*run)(edict_t *self);
	void (*dodge)(edict_t *self, edict_t *other, float eta, trace_t *tr);
	void (*attack)(edict_t *self);
	void (*melee)(edict_t *self);
	void (*sight)(edict_t *self, edict_t *other);
	qboolean (*checkattack)(edict_t *self);

	/* dynamic actions */
	const char *action;
	float walk_dist;
	float run_dist;
	/* frames cache */
	int firstframe;
	int numframes;

	float pausetime;
	float attack_finished;

	vec3_t saved_goal;
	float search_time;
	float trail_time;
	vec3_t last_sighting;
	int attack_state;
	int lefty;
	float idle_time;
	int linkcount;

	int power_armor_type;
	int power_armor_power;

	qboolean (*blocked)(edict_t *self, float dist);
	float last_hint_time;           /* last time the monster checked for hintpaths. */
	edict_t *goal_hint;             /* which hint_path we're trying to get to */
	int medicTries;
	edict_t *badMedic1, *badMedic2; /* these medics have declared this monster "unhealable" */
	edict_t *healer;				/* this is who is healing this monster */
	void (*duck)(edict_t *self, float eta);
	void (*unduck)(edict_t *self);
	void (*sidestep)(edict_t *self);
	float base_height;
	float next_duck_time;
	float duck_wait_time;
	edict_t *last_player_enemy;
	qboolean blindfire;				/* will the monster blindfire? */
	float blind_fire_delay;
	vec3_t blind_fire_target;

	/* used by the spawners to not spawn too much and keep track of #s of monsters spawned */
	int monster_slots;
	int monster_used;
	edict_t *commander;

	/* powerup timers, used by widow, our friend */
	float quad_framenum;
	float invincible_framenum;
	float double_framenum;
} monsterinfo_t;

/* this determines how long to wait after a duck to duck again.
   this needs to be longer than the time after the monster_duck_up
   in all of the animation sequences */
#define DUCK_INTERVAL 0.5

extern game_locals_t game;
extern level_locals_t level;
extern game_import_t gi;
extern game_export_t globals;
extern spawn_temp_t st;

extern int sm_meat_index;
extern int snd_fry;

extern int debristhisframe;
extern int gibsthisframe;

/* means of death */
#define MOD_UNKNOWN 0
#define MOD_BLASTER 1
#define MOD_SHOTGUN 2
#define MOD_SSHOTGUN 3
#define MOD_MACHINEGUN 4
#define MOD_CHAINGUN 5
#define MOD_GRENADE 6
#define MOD_G_SPLASH 7
#define MOD_ROCKET 8
#define MOD_R_SPLASH 9
#define MOD_HYPERBLASTER 10
#define MOD_RAILGUN 11
#define MOD_BFG_LASER 12
#define MOD_BFG_BLAST 13
#define MOD_BFG_EFFECT 14
#define MOD_HANDGRENADE 15
#define MOD_HG_SPLASH 16
#define MOD_WATER 17
#define MOD_SLIME 18
#define MOD_LAVA 19
#define MOD_CRUSH 20
#define MOD_TELEFRAG 21
#define MOD_FALLING 22
#define MOD_SUICIDE 23
#define MOD_HELD_GRENADE 24
#define MOD_EXPLOSIVE 25
#define MOD_BARREL 26
#define MOD_BOMB 27
#define MOD_EXIT 28
#define MOD_SPLASH 29
#define MOD_TARGET_LASER 30
#define MOD_TRIGGER_HURT 31
#define MOD_HIT 32
#define MOD_TARGET_BLASTER 33
#define MOD_RIPPER 34
#define MOD_PHALANX 35
#define MOD_BRAINTENTACLE 36
#define MOD_BLASTOFF 37
#define MOD_GEKK 38
#define MOD_TRAP 39
#define MOD_GRAPPLE 40
#define MOD_FRIENDLY_FIRE 0x8000000
#define MOD_CHAINFIST 41
#define MOD_DISINTEGRATOR 42
#define MOD_ETF_RIFLE 43
#define MOD_BLASTER2 44
#define MOD_HEATBEAM 45
#define MOD_TESLA 46
#define MOD_PROX 47
#define MOD_NUKE 48
#define MOD_VENGEANCE_SPHERE 49
#define MOD_HUNTER_SPHERE 50
#define MOD_DEFENDER_SPHERE 51
#define MOD_TRACKER 52
#define MOD_DBALL_CRUSH 53
#define MOD_DOPPLE_EXPLODE 54
#define MOD_DOPPLE_VENGEANCE 55
#define MOD_DOPPLE_HUNTER 56

/* Easier handling of AI skill levels */
#define SKILL_EASY 0
#define SKILL_MEDIUM 1
#define SKILL_HARD 2
#define SKILL_HARDPLUS 3

extern int meansOfDeath;
extern edict_t *g_edicts;

#define FOFS(x) (size_t)&(((edict_t *)NULL)->x)
#define STOFS(x) (size_t)&(((spawn_temp_t *)NULL)->x)
#define LLOFS(x) (size_t)&(((level_locals_t *)NULL)->x)
#define CLOFS(x) (size_t)&(((gclient_t *)NULL)->x)

#define random() ((randk() & 0x7fff) / ((float)0x7fff))
#define crandom() (2.0 * (random() - 0.5))

extern cvar_t *maxentities;
extern cvar_t *deathmatch;
extern cvar_t *coop;
extern cvar_t *coop_baseq2;	/* treat spawnflags according to baseq2 rules */
extern cvar_t *coop_elevator_delay;
extern cvar_t *coop_pickup_weapons;
extern cvar_t *dmflags;
extern cvar_t *skill;
extern cvar_t *fraglimit;
extern cvar_t *timelimit;
extern cvar_t *capturelimit;
extern cvar_t *instantweap;
extern cvar_t *password;
extern cvar_t *spectator_password;
extern cvar_t *needpass;
extern cvar_t *g_select_empty;
extern cvar_t *dedicated;
extern cvar_t *g_footsteps;
extern cvar_t *g_monsterfootsteps;
extern cvar_t *g_fix_triggered;
extern cvar_t *g_commanderbody_nogod;

extern cvar_t *filterban;

extern cvar_t *sv_gravity;
extern cvar_t *sv_maxvelocity;

extern cvar_t *gun_x, *gun_y, *gun_z;
extern cvar_t *sv_rollspeed;
extern cvar_t *sv_rollangle;

extern cvar_t *run_pitch;
extern cvar_t *run_roll;
extern cvar_t *bob_up;
extern cvar_t *bob_pitch;
extern cvar_t *bob_roll;

extern cvar_t *sv_cheats;
extern cvar_t *maxclients;
extern cvar_t *maxspectators;

extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;

extern cvar_t *sv_maplist;

extern cvar_t *sv_stopspeed;

extern cvar_t *g_showlogic;
extern cvar_t *gamerules;
extern cvar_t *huntercam;
extern cvar_t *strong_mines;
extern cvar_t *randomrespawn;

extern cvar_t *g_disruptor;

extern cvar_t *aimfix;
extern cvar_t *g_machinegun_norecoil;
extern cvar_t *g_quick_weap;
extern cvar_t *g_swap_speed;
extern cvar_t *g_itemsbobeffect;
extern cvar_t *g_start_items;
extern cvar_t *ai_model_scale;
extern cvar_t *g_game;

/* this is for the count of monsters */
#define ENT_SLOTS_LEFT \
	(ent->monsterinfo.monster_slots - \
	 ent->monsterinfo.monster_used)
#define SELF_SLOTS_LEFT \
	(self->monsterinfo.monster_slots - \
	 self->monsterinfo.monster_used)

#define world (&g_edicts[0])

/* item spawnflags */
#define ITEM_TRIGGER_SPAWN 0x00000001
#define ITEM_NO_TOUCH 0x00000002
/* 6 bits reserved for editor flags */
/* 8 bits used as power cube id bits for coop games */
#define DROPPED_ITEM 0x00010000
#define DROPPED_PLAYER_ITEM 0x00020000
#define ITEM_TARGETS_USED 0x00040000

/* fields are needed for spawning from the entity
   string and saving / loading games */
#define FFL_SPAWNTEMP 1
#define FFL_NOSPAWN 2

typedef enum
{
	F_INT,
	F_FLOAT,
	F_LSTRING,          /* string on disk, pointer in memory, TAG_LEVEL */
	F_GSTRING,          /* string on disk, pointer in memory, TAG_GAME */
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,            /* index on disk, pointer in memory */
	F_ITEM,             /* index on disk, pointer in memory */
	F_CLIENT,           /* index on disk, pointer in memory */
	F_FUNCTION,
	F_MMOVE,
	F_IGNORE,
	F_RGBA,
	F_LRAWSTRING,       /* raw string on disk, pointer in memory, TAG_LEVEL */
} fieldtype_t;

typedef struct
{
	const char *name;
	size_t ofs;
	fieldtype_t type;
	int flags;
	short save_ver;
} field_t;

extern field_t fields[];
extern gitem_t itemlist[];

/* player/client.c */
void ClientBegin(edict_t *ent);
void ClientDisconnect(edict_t *ent);
void ClientUserinfoChanged(edict_t *ent, char *userinfo);
qboolean ClientConnect(edict_t *ent, char *userinfo);
void ClientThink(edict_t *ent, usercmd_t *cmd);
edict_t *SP_GetSpawnPoint(void);

/* g_cmds.c */
qboolean CheckFlood(edict_t *ent);
void Cmd_Help_f(edict_t *ent);
void ClientCommand(edict_t *ent);

/* g_items.c */
void droptofloor(edict_t *ent);
void FixEntityPosition(edict_t *ent);
void PrecacheItem(gitem_t *it);
void InitItems(void);
qboolean ItemHasValidModel(gitem_t *item);
void SetItemNames(void);
gitem_t *FindItem(const char *pickup_name);
gitem_t *FindItemByClassname(const char *classname);

#define ITEM_INDEX(x) ((x) - itemlist)

edict_t *Drop_Item(edict_t *ent, gitem_t *item);
void SetRespawn(edict_t *ent, float delay);
void ChangeWeapon(edict_t *ent);
void SpawnItem(edict_t *ent, gitem_t *item);
void Think_Weapon(edict_t *ent);
int ArmorIndex(edict_t *ent);
int PowerArmorType(edict_t *ent);
gitem_t *GetItemByIndex(int index);
qboolean Add_Ammo(edict_t *ent, gitem_t *item, int count);
void Touch_Item(edict_t *ent, edict_t *other, cplane_t *plane,
		csurface_t *surf);
void Use_Quad(edict_t *ent, gitem_t *item);
void Use_QuadFire(edict_t *ent, gitem_t *item);

/* g_utils.c */
qboolean KillBox(edict_t *ent);
void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward,
		const vec3_t right, vec3_t result);
edict_t *G_Find(edict_t *from, int fieldofs, const char *match);
edict_t *findradius(edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget(char *targetname);
void G_UseTargets(edict_t *ent, edict_t *activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);

void G_InitEdict(edict_t *e);
edict_t *G_SpawnOptional(void);
edict_t *G_Spawn(void);
void G_FreeEdict(edict_t *e);

void G_TouchTriggers(edict_t *ent);
void G_TouchSolids(edict_t *ent);

char *G_CopyString(char *in);

float *tv(float x, float y, float z);
char *vtos(vec3_t v);
void get_normal_vector(const cplane_t *p, vec3_t normal);

float vectoyaw(vec3_t vec);
void vectoangles(vec3_t vec, vec3_t angles);

void G_ProjectSource2(const vec3_t point, const vec3_t distance, const vec3_t forward,
		const vec3_t right, const vec3_t up, vec3_t result);
float vectoyaw2(vec3_t vec);
void vectoangles2(vec3_t vec, vec3_t angles);
edict_t *findradius2(edict_t *from, vec3_t org, float rad);

/* g_combat.c */
qboolean OnSameTeam(edict_t *ent1, edict_t *ent2);
qboolean CanDamage(edict_t *targ, edict_t *inflictor);
qboolean CheckTeamDamage(edict_t *targ, edict_t *attacker);
void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker,
		vec3_t dir, vec3_t point, vec3_t normal, int damage,
		int knockback, int dflags, int mod);
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker,
		float damage, edict_t *ignore, float radius,
		int mod);
void T_RadiusNukeDamage(edict_t *inflictor, edict_t *attacker, float damage,
		edict_t *ignore, float radius, int mod);
void cleanupHealTarget(edict_t *ent);

/* damage flags */
#define DAMAGE_RADIUS 0x00000001            /* damage was indirect */
#define DAMAGE_NO_ARMOR 0x00000002          /* armour does not protect from this damage */
#define DAMAGE_ENERGY 0x00000004            /* damage is from an energy based weapon */
#define DAMAGE_NO_KNOCKBACK 0x00000008      /* do not affect velocity, just view angles */
#define DAMAGE_BULLET 0x00000010            /* damage is from a bullet (used for ricochets) */
#define DAMAGE_NO_PROTECTION 0x00000020     /* armor, shields, invulnerability, and godmode have no effect */
#define DAMAGE_DESTROY_ARMOR 0x00000040     /* damage is done to armor and health. */
#define DAMAGE_NO_REG_ARMOR 0x00000080      /* damage skips regular armor */
#define DAMAGE_NO_POWER_ARMOR 0x00000100    /* damage skips power armor */

#define DEFAULT_BULLET_HSPREAD 300
#define DEFAULT_BULLET_VSPREAD 500
#define DEFAULT_SHOTGUN_HSPREAD 1000
#define DEFAULT_SHOTGUN_VSPREAD 500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT 12
#define DEFAULT_SHOTGUN_COUNT 12
#define DEFAULT_SSHOTGUN_COUNT 20

/* g_monster.c */
void monster_fire_bullet(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int kick, int hspread, int vspread, int flashtype);
void monster_fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int kick, int hspread, int vspread, int count,
		int flashtype);
void monster_fire_blaster(edict_t *self, vec3_t start, vec3_t dir,
		int damage, int speed, int flashtype, int effect);
void monster_fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int speed, int flashtype);
void monster_fire_rocket(edict_t *self, vec3_t start, vec3_t dir,
		int damage, int speed, int flashtype);
void monster_fire_railgun(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int kick, int flashtype);
void monster_fire_bfg(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int speed, int kick, float damage_radius,
		int flashtype);
void monster_fire_ionripper(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect);
void monster_fire_heat(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype);
void monster_fire_heatbeam(edict_t *self, vec3_t start, vec3_t dir, vec3_t offset,
		int damage, int kick, int flashtype);
void monster_dabeam(edict_t *self);
void monster_fire_blueblaster(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect);

void M_droptofloor(edict_t *ent);
void monster_dynamic_run(edict_t *self);
void monster_dynamic_walk(edict_t *self);
void monster_dynamic_idle(edict_t *self);
void monster_dynamic_stand(edict_t *self);
void monster_dynamic_search(edict_t *self);
void monster_dynamic_setinfo(edict_t *self);
void monster_dynamic_melee(edict_t *self);
void monster_dynamic_damage(edict_t *self);
void monster_dynamic_dodge(edict_t *self, edict_t *attacker, float eta,
	trace_t *tr /* unused */);
void monster_dynamic_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, vec3_t point);
void monster_dynamic_die_noanim(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, vec3_t point);
void monster_dynamic_dead(edict_t *self);
void monster_dynamic_attack(edict_t *self);
void monster_dynamic_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage);
void monster_dynamic_pain_noanim(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage);
void monster_dynamic_sight(edict_t *self, edict_t *other /* unused */);
void monster_think(edict_t *self);
void walkmonster_start(edict_t *self);
void swimmonster_start(edict_t *self);
void flymonster_start(edict_t *self);
void AttackFinished(edict_t *self, float time);
void monster_death_use(edict_t *self);
void M_CatagorizePosition(edict_t *ent);
qboolean M_CheckAttack(edict_t *self);
void M_FlyCheck(edict_t *self);
void M_CheckGround(edict_t *ent);
void M_FliesOff(edict_t *self);
void M_FliesOn(edict_t *self);
void M_SetEffects(edict_t *ent);
void object_think(edict_t *self);
void object_spawn(edict_t *self);

void monster_fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect);
void monster_fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, edict_t *enemy, int flashtype);
void stationarymonster_start(edict_t *self);
void monster_done_dodge(edict_t *self);

/* g_misc.c */
void ThrowHead(edict_t *self, const char *gibname, int damage, gibtype_t type);
void ThrowClientHead(edict_t *self, int damage);
void ThrowGib(edict_t *self, const char *gibname, int damage, gibtype_t type);
void BecomeExplosion1(edict_t *self);
void ThrowHeadACID(edict_t *self, const char *gibname, int damage, gibtype_t type);
void ThrowGibACID(edict_t *self, const char *gibname, int damage, gibtype_t type);
void barrel_delay (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void SP_misc_teleporter_dest(edict_t *ent);

/* g_ai.c */
void AI_SetSightClient(void);

void ai_stand(edict_t *self, float dist);
void ai_move(edict_t *self, float dist);
void ai_walk(edict_t *self, float dist);
void ai_turn(edict_t *self, float dist);
void ai_run(edict_t *self, float dist);
void ai_charge(edict_t *self, float dist);
int range(edict_t *self, edict_t *other);

void FoundTarget(edict_t *self);
qboolean FindTarget(edict_t *self);
qboolean infront(edict_t *self, edict_t *other);
qboolean visible(edict_t *self, edict_t *other);
qboolean FacingIdeal(edict_t *self);
void HuntTarget(edict_t *self);
qboolean ai_checkattack(edict_t *self);

/* g_weapon.c */
void ThrowDebris(edict_t *self, char *modelname, float speed, vec3_t origin);
qboolean fire_hit(edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int kick, int hspread, int vspread, int mod);
void fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int kick, int hspread, int vspread, int count, int mod);
void fire_blaster(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, int effect, qboolean hyper);
void fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, float timer, float damage_radius, qboolean monster);
void fire_grenade2(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, float timer, float damage_radius, qboolean held);
void fire_rocket(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, float damage_radius, int radius_damage);
void fire_rail(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, float damage_radius);
void fire_ionripper(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, int effect);
void fire_heat(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed,
		float damage_radius, int radius_damage);
void fire_heatbeam(edict_t *self, vec3_t start, vec3_t aimdir, vec3_t offset,
		int damage, int kick, qboolean monster);
void fire_blueblaster(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, int effect);
void fire_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed,
		float damage_radius, int radius_damage);
void fire_trap(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, float timer, float damage_radius, qboolean held);
void fire_flaregun(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, float timer, float damage_radius);

/* g_ptrail.c */
void PlayerTrail_Init(void);
void PlayerTrail_Add(vec3_t spot);
void PlayerTrail_New(vec3_t spot);
edict_t *PlayerTrail_PickFirst(edict_t *self);
edict_t *PlayerTrail_PickNext(edict_t *self);
edict_t *PlayerTrail_LastSpot(void);

/* g_client.c */
void respawn(edict_t *ent);
void BeginIntermission(edict_t *targ);
void PutClientInServer(edict_t *ent);
void InitClientPersistant(edict_t *ent);
void InitClientResp(gclient_t *client);
void InitBodyQue(void);
void ClientBeginServerFrame(edict_t *ent);

/* g_player.c */
void player_pain(edict_t *self, edict_t *other, float kick, int damage);
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, vec3_t point);

/* g_svcmds.c */
void ServerCommand(void);
qboolean SV_FilterPacket(char *from);

/* p_view.c */
void G_SetClientFrame(edict_t *ent, float speed);
void ClientEndServerFrame(edict_t *ent);

/* p_hud.c */
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void G_SetSpectatorStats(edict_t *ent);
void G_CheckChaseStats(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);
void DeathmatchScoreboardMessage(edict_t *client, edict_t *killer);
void HelpComputerMessage(edict_t *client);
void InventoryMessage(edict_t *client);

/* g_pweapon.c */
void PlayerNoise(edict_t *who, vec3_t where, int type);
void P_ProjectSource(const edict_t *ent, const vec3_t distance,
		vec3_t forward, const vec3_t right, vec3_t result);
void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST,
		int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames,
		const int *fire_frames, void (*fire)(edict_t *ent));
qboolean Pickup_Weapon(edict_t *ent, edict_t *other);
void Use_Weapon(edict_t *ent, gitem_t *inv);
void Use_Weapon2(edict_t *ent, gitem_t *inv);
void Drop_Weapon(edict_t *ent, gitem_t *inv);
void Weapon_Blaster(edict_t *ent);
void Weapon_Shotgun(edict_t *ent);
void Weapon_SuperShotgun(edict_t *ent);
void Weapon_Machinegun(edict_t *ent);
void Weapon_Chaingun(edict_t *ent);
void Weapon_HyperBlaster(edict_t *ent);
void Weapon_RocketLauncher(edict_t *ent);
void Weapon_Grenade(edict_t *ent);
void Weapon_GrenadeLauncher(edict_t *ent);
void Weapon_Railgun(edict_t *ent);
void Weapon_BFG(edict_t *ent);
void Weapon_ChainFist(edict_t *ent);
void Weapon_Disintegrator(edict_t *ent);
void Weapon_Beta_Disintegrator(edict_t *ent);
void Weapon_ETF_Rifle(edict_t *ent);
void Weapon_Heatbeam(edict_t *ent);
void Weapon_Prox(edict_t *ent);
void Weapon_Tesla(edict_t *ent);
void Weapon_ProxLauncher(edict_t *ent);
void Weapon_Ionripper(edict_t *ent);
void Weapon_Phalanx(edict_t *ent);
void Weapon_Trap(edict_t *ent);
void Weapon_FlareGun(edict_t *ent);

/* m_move.c */
qboolean M_CheckBottom(edict_t *ent);
qboolean M_walkmove(edict_t *ent, float yaw, float dist);
void M_MoveToGoal(edict_t *ent, float dist);
void M_ChangeYaw(edict_t *ent);
void M_SetAnimGroupFrame(edict_t *self, const char *name, qboolean fixpos);
void M_SetAnimGroupFrameValues(edict_t *self, const char *name,
	int *ofs_frames, int *num_frames, int select);
void M_SetAnimGroupMMove(edict_t *self, mmove_t *mmove, const mmove_t *mmove_old,
	const char *name, int select);

/* g_phys.c */
void G_RunEntity(edict_t *ent);
void SV_AddGravity(edict_t *ent);

/* g_main.c */
void SaveClientData(void);
void EndDMLevel(void);

/* g_chase.c */
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

/* savegame */
void InitGame(void);
void ReadLevel(const char *filename);
void WriteLevel(const char *filename);
void ReadGame(const char *filename);
void WriteGame(const char *filename, qboolean autosave);
void SpawnEntities(const char *mapname, char *entities, const char *spawnpoint);
void ReinitGameEntities(int ent_cnt);

void fire_flechette(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int kick);
void fire_prox(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed);
void fire_nuke(edict_t *self, vec3_t start, vec3_t aimdir, int speed);
void fire_flame(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed);
void fire_burst(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed);
void fire_maintain(edict_t *, edict_t *, vec3_t start, vec3_t aimdir,
		int damage, int speed);
void fire_incendiary_grenade(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int speed, float timer, float damage_radius);
void fire_player_melee(edict_t *self, vec3_t start, vec3_t aim, int reach,
		int damage, int kick, int quiet, int mod);
void fire_tesla(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed);
void fire_blaster2(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int speed, int effect, qboolean hyper);
void fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, edict_t *enemy);

/* g_newai.c */
qboolean blind_rocket_ok(edict_t *self, vec3_t start, vec3_t right, vec3_t target, float ofs,
	vec3_t dir);
qboolean blocked_checkplat(edict_t *self, float dist);
qboolean blocked_checkjump(edict_t *self, float dist, float maxDown, float maxUp);
qboolean blocked_checknewenemy(edict_t *self);
qboolean monsterlost_checkhint(edict_t *self);
qboolean inback(edict_t *self, edict_t *other);
float realrange(edict_t *self, edict_t *other);
edict_t *SpawnBadArea(vec3_t mins, vec3_t maxs, float lifespan, edict_t *owner);
edict_t *CheckForBadArea(edict_t *ent);
qboolean MarkTeslaArea(edict_t *self, edict_t *tesla);
void InitHintPaths(void);
void PredictAim(edict_t *target, vec3_t start, float bolt_speed, qboolean eye_height,
		float offset, vec3_t aimdir, vec3_t aimpoint);
qboolean below(edict_t *self, edict_t *other);
void drawbbox(edict_t *self);
void M_MonsterDodge(edict_t *self, edict_t *attacker, float eta, trace_t *tr);
void monster_duck_down(edict_t *self);
void monster_duck_hold(edict_t *self);
void monster_duck_up(edict_t *self);
qboolean has_valid_enemy(edict_t *self);
void TargetTesla(edict_t *self, edict_t *tesla);
void hintpath_stop(edict_t *self);
edict_t *PickCoopTarget(edict_t *self);
int CountPlayers(void);
void monster_jump_start(edict_t *self);
qboolean monster_jump_finished(edict_t *self);
qboolean face_wall(edict_t *self);

/* g_sphere.c */
void Defender_Launch(edict_t *self);
void Vengeance_Launch(edict_t *self);
void Hunter_Launch(edict_t *self);

/* g_newdm.c */
void InitGameRules(void);
edict_t *DoRandomRespawn(edict_t *ent);
void PrecacheForRandomRespawn(void);
qboolean Tag_PickupToken(edict_t *ent, edict_t *other);
void Tag_DropToken(edict_t *ent, gitem_t *item);
void Tag_PlayerDeath(edict_t *targ, edict_t *inflictor, edict_t *attacker);
void fire_doppleganger(edict_t *ent, vec3_t start, vec3_t aimdir);

/* g_spawn.c */
gitem_t *GetDynamicItems(int *count);
void ED_CallSpawn(edict_t *ent);
void DynamicResetSpawnModels(edict_t *self);
char *ED_NewString(const char *string, qboolean raw);
void SpawnInit(void);
void SpawnFree(void);
void P_ToggleFlashlight(edict_t *ent, qboolean state);
void P_SetAnimGroup(edict_t *ent, const char *animname,
	int firstframe, int lastframe, int select);
edict_t *CreateFlyMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname);
edict_t *CreateGroundMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname, int height);
qboolean FindSpawnPoint(vec3_t startpoint, vec3_t mins, vec3_t maxs,
		vec3_t spawnpoint, float maxMoveUp);
qboolean CheckSpawnPoint(vec3_t origin, vec3_t mins, vec3_t maxs);
qboolean CheckGroundSpawnPoint(vec3_t origin, vec3_t entMins, vec3_t entMaxs,
		float height, float gravity);
void SpawnGrow_Spawn(vec3_t startpos, int size);
void Widowlegs_Spawn(vec3_t startpos, vec3_t angles);
void ThrowSmallStuff(edict_t *self, vec3_t point);
void ThrowWidowGibSized(edict_t *self, char *gibname, int damage, gibtype_t type,
		vec3_t startpos, int hitsound, qboolean fade);
void spawngrow_think(edict_t *self);

/* p_client.c */
void RemoveAttackingPainDaemons(edict_t *self);
void ForceFogTransition(edict_t *ent, qboolean instant);

/* ============================================================================ */

#include "ai.h"//JABot

/* client_t->anim_priority */
#define ANIM_BASIC 0            /* stand / run */
#define ANIM_WAVE 1
#define ANIM_JUMP 2
#define ANIM_PAIN 3
#define ANIM_ATTACK 4
#define ANIM_DEATH 5
#define ANIM_REVERSE 6

/* height fog data values */
typedef struct
{
	// r g b dist
	float start[4];
	float end[4];
	float falloff;
	float density;
} height_fog_t;

/* client data that stays across multiple level loads */
typedef struct
{
	char userinfo[MAX_INFO_STRING];
	char netname[16];
	int hand;

	qboolean connected;             /* a loadgame will leave valid entities that
	                                   just don't have a connection yet */

	/* values saved and restored from
	   edicts when changing levels */
	int health;
	int max_health;
	int savedFlags;

	int selected_item;
	int inventory[MAX_ITEMS];

	/* ammo capacities */
	int max_bullets;
	int max_shells;
	int max_rockets;
	int max_grenades;
	int max_cells;
	int max_slugs;
	int max_magslug;
	int max_trap;

	gitem_t *weapon;
	gitem_t *lastweapon;

	int power_cubes;            /* used for tracking the cubes in coop games */
	int score;                  /* for calculating total unit score in coop games */

	int game_helpchanged;
	int helpchanged;

	qboolean spectator;         /* client is a spectator */
	int chasetoggle;       /* Chasetoggle */

	int max_tesla;
	int max_prox;
	int max_mines;
	int max_flechettes;
	int max_rounds;

	/* [Paril-KEX] fog that we want to achieve; density rgb skyfogfactor */
	float wanted_fog[5];
	height_fog_t wanted_heightfog;
	/* relative time value, copied from last touched trigger */
	float fog_transition_time;
} client_persistant_t;

/* client data that stays across deathmatch respawns */
typedef struct
{
	client_persistant_t coop_respawn;   /* what to set client->pers to on a respawn */
	int enterframe;                 /* level.framenum the client entered the game */
	int score;                      /* frags, etc */
	int ctf_team;                   /* CTF team */
	int ctf_state;
	float ctf_lasthurtcarrier;
	float ctf_lastreturnedflag;
	float ctf_flagsince;
	float ctf_lastfraggedcarrier;
	qboolean id_state;
	float lastidtime;
	qboolean voted;    /* for elections */
	qboolean ready;
	qboolean admin;
	struct ghost_s *ghost; /* for ghost codes */
	vec3_t cmd_angles;              /* angles sent over in the last command */
	int game_helpchanged;
	int helpchanged;
	qboolean spectator;             /* client is a spectator */
} client_respawn_t;

/*
 * CTF menu
 */
typedef struct pmenuhnd_s
{
	struct pmenu_s *entries;
	int cur;
	int num;
	void *arg;
} pmenuhnd_t;

/* this structure is cleared on each
   PutClientInServer(), except for 'client->pers' */
struct gclient_s
{
	/* known to server */
	player_state_t ps;              /* communicated by server to clients */
	int ping;

	/* private to game */
	client_persistant_t pers;
	client_respawn_t resp;
	pmove_state_t old_pmove;        /* for detecting out-of-pmove changes */

	qboolean showscores;            /* set layout stat */
	qboolean inmenu;                /* in menu */
	pmenuhnd_t *menu;               /* current menu */
	qboolean showinventory;         /* set layout stat */
	qboolean showhelp;
	qboolean showhelpicon;

	int ammo_index;

	int buttons;
	int oldbuttons;
	int latched_buttons;

	qboolean weapon_thunk;

	gitem_t *newweapon;

	/* sum up damage over an entire frame, so
	   shotgun blasts give a single big kick */
	int damage_armor;               /* damage absorbed by armor */
	int damage_parmor;              /* damage absorbed by power armor */
	int damage_blood;               /* damage taken out of health */
	int damage_knockback;           /* impact damage */
	vec3_t damage_from;             /* origin for vector calculation */

	float killer_yaw;               /* when dead, look at killer */

	weaponstate_t weaponstate;
	vec3_t kick_angles;				/* weapon kicks */
	vec3_t kick_origin;
	float v_dmg_roll, v_dmg_pitch, v_dmg_time;          /* damage kicks */
	float fall_time, fall_value;    /* for view drop on fall */
	float damage_alpha;
	float bonus_alpha;
	vec3_t damage_blend;
	vec3_t v_angle;                 /* aiming direction */
	float bobtime;                  /* so off-ground doesn't change it */
	vec3_t oldviewangles;
	vec3_t oldvelocity;

	float next_drown_time;
	int old_waterlevel;
	int breather_sound;

	int machinegun_shots;           /* for weapon raising */

	/* animation vars */
	int anim_end;
	int anim_priority;
	qboolean anim_duck;
	qboolean anim_run;

	/* powerup timers */
	float quad_framenum;
	float invincible_framenum;
	float invisible_framenum;
	float breather_framenum;
	float enviro_framenum;

	qboolean grenade_blew_up;
	float grenade_time;
	float quadfire_framenum;
	qboolean trap_blew_up;
	float trap_time;

	int silencer_shots;
	int weapon_sound;

	float pickup_msg_time;

	float flood_locktill;           /* locked from talking */
	float flood_when[10];           /* when messages were said */
	int flood_whenhead;             /* head pointer for when said */

	float respawn_time;             /* can respawn when time > this */

	edict_t *chase_target;          /* player we are chasing */
	qboolean update_chase;          /* need to update chase info? */

	void *ctf_grapple;              /* entity of grapple */
	int ctf_grapplestate;               /* true if pulling */
	float ctf_grapplereleasetime;       /* time of grapple release */
	float ctf_regentime;            /* regen tech */
	float ctf_techsndtime;
	float ctf_lasttechmsg;
	float menutime;                 /* time to update menu */
	qboolean menudirty;

	float double_framenum;
	float ir_framenum;
	float nuke_framenum;
	float tracker_pain_framenum;

	edict_t *owned_sphere;          /* this points to the player's sphere */

	/* Third person view */
	int chasetoggle;
	edict_t *chasecam;
	edict_t *oldplayer;
	int use;
	int zoom;
	int delayedstart;

	/* sync fog */
	float fog[5];
	height_fog_t heightfog;
};

typedef struct {
	vec3_t color;
	float density;
	float sky_factor;

	vec3_t color_off;
	float density_off;
	float sky_factor_off;
	/* Kingpin */
	vec3_t altcolor;
	float altdensity;
	/* Anachronox */
	char *afog;
} edictfog_t;

typedef struct {
	float falloff;
	float density;
	vec3_t start_color;
	float start_dist;
	vec3_t end_color;
	float end_dist;

	float falloff_off;
	float density_off;
	vec3_t start_color_off;
	float start_dist_off;
	vec3_t end_color_off;
	float end_dist_off;
} edicthfog_t;

typedef enum
{
	BMODEL_ANIM_FORWARDS,
	BMODEL_ANIM_BACKWARDS,
	BMODEL_ANIM_RANDOM
} bmodel_animstyle_t;

typedef struct
{
	/* range, inclusive */
	int start, end;
	bmodel_animstyle_t style;
	int speed; /* in milliseconds */
	qboolean nowrap;

	int alt_start, alt_end;
	bmodel_animstyle_t alt_style;
	int alt_speed; /* in milliseconds */
	qboolean alt_nowrap;

	/* game-only */
	bool enabled;
	bool alternate, currently_alternate;
	float next_tick;
} bmodel_anim_t;

struct edict_s
{
	entity_state_t s;
	struct gclient_s *client;       /* NULL if not a player the server expects the first part
	                                   of gclient_s to be a player_state_t but the rest of it is
									   opaque */

	qboolean inuse;
	int linkcount;

	link_t area;                    /* linked to a division node or leaf */

	int num_clusters;               /* if -1, use headnode instead */
	int clusternums[MAX_ENT_CLUSTERS];
	int headnode;                   /* unused if num_clusters != -1 */
	int areanum, areanum2;

	/* ================================ */

	int svflags;
	vec3_t mins, maxs;
	vec3_t absmin, absmax, size;
	solid_t solid;
	int clipmask;
	edict_t *owner;

	/* Additional state from ReRelease */
	entity_rrstate_t rrs;

	/* DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER */
	/* EXPECTS THE FIELDS IN THAT ORDER! */

	/* ================================ */

	int movetype;
	int flags;

	char *model;
	float freetime;                 /* sv.time when the object was freed */

	/* only used locally in game, not by server */
	char *message;
	char *classname;
	int spawnflags;

	float timestamp;

	float angle;					/* set in qe3, -1 = up, -2 = down */
	char *target;
	char *targetname;
	char *killtarget;
	char *team;
	char *pathtarget;
	char *deathtarget;
	char *combattarget;
	char *itemtarget; /* extra target string used by dynamic lights */
	edict_t *target_ent;

	float speed, accel, decel;
	vec3_t movedir;
	vec3_t pos1, pos2;

	vec3_t velocity;
	vec3_t avelocity;
	int mass;
	float air_finished;
	float gravity;              /* per entity gravity multiplier (1.0 is normal) */
	                            /* use for lowgrav artifact, flares */

	edict_t *goalentity;
	edict_t *movetarget;
	float yaw_speed;
	float ideal_yaw;

	float nextthink;
	void (*prethink)(edict_t *ent);
	void (*think)(edict_t *self);
	void (*blocked)(edict_t *self, edict_t *other);         /* move to moveinfo? */
	void (*touch)(edict_t *self, edict_t *other, cplane_t *plane,
			csurface_t *surf);
	void (*use)(edict_t *self, edict_t *other, edict_t *activator);
	void (*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void (*die)(edict_t *self, edict_t *inflictor, edict_t *attacker,
			int damage, vec3_t point);

	float touch_debounce_time;		/* now also used by fixbots for timeouts when getting stuck */
	float pain_debounce_time;
	float damage_debounce_time;
	float fly_sound_debounce_time;	/* now also used by insane marines to store pain sound timeout */
									/* and by fixbots for storing object_repair timeout when getting stuck */
	float last_move_time;

	int health;
	int max_health;
	int gib_health;
	int deadflag;

	float show_hostile;
	float powerarmor_time;

	char *map;                  /* target_changelevel */

	int viewheight;             /* height above origin where eyesight is determined */
	int takedamage;
	int dmg;                    /* base damage */
	int dmg_range;              /* additional damage range */
	vec3_t damage_aim;          /* aim for dynamic animation damage */
	gibtype_t gib;              /* default gib type */
	const char *gibtype;        /* gib value from level entity */
	int radius_dmg;
	float dmg_radius;
	int sounds;                 /* now also used for player death sound aggregation */
	int count;

	edict_t *chain;
	edict_t *enemy;
	edict_t *oldenemy;
	edict_t *activator;
	edict_t *groundentity;
	int groundentity_linkcount;
	edict_t *teamchain;
	edict_t *teammaster;

	edict_t *mynoise;           /* can go in client only */
	edict_t *mynoise2;

	int noise_index;
	int noise_index2;
	float volume;
	float attenuation;

	/* timing variables */
	float wait;
	float delay;                /* before firing targets */
	float random;

	float last_sound_time;

	int watertype;
	int waterlevel;

	vec3_t move_origin;
	vec3_t move_angles;

	/* move this to clientinfo? */
	int light_level;

	int style;                  /* also used as areaportal number */

	gitem_t *item;              /* for bonus items */

	/* common data blocks */
	moveinfo_t moveinfo;
	monsterinfo_t monsterinfo;

	int orders;

	int plat2flags;
	vec3_t offset;
	vec3_t gravityVector;
	edict_t *bad_area;
	edict_t *hint_chain;
	edict_t *monster_hint_chain;
	edict_t *target_hint_chain;
	int hint_chain_id;
	float lastMoveTime;

	/* Fog stuff */
	edictfog_t fog;
	edicthfog_t heightfog;

	/* Custom On/Off light */
	const char *style_on;
	const char *style_off;

	/* brush model animation */
	bmodel_anim_t bmodel_anim;

	/* Third person view */
	int chasedist1;
	int chasedist2;

	/* jabot */
	ai_handle_t *ai;
	/* AI_CategorizePosition */
	qboolean is_swim;
	qboolean is_step;
	qboolean is_ladder;
	qboolean was_swim;
	qboolean was_step;
	qboolean was_falling;
	int last_node;
	float last_update;
};

#define SPHERE_DEFENDER 0x0001
#define SPHERE_HUNTER 0x0002
#define SPHERE_VENGEANCE 0x0004
#define SPHERE_DOPPLEGANGER 0x0100

#define SPHERE_TYPE 0x00FF
#define SPHERE_FLAGS 0xFF00

/* deathmatch games */
#define     RDM_TAG 2
#define     RDM_DEATHBALL 3

typedef struct dm_game_rs
{
	void (*GameInit)(void);
	void (*PostInitSetup)(void);
	void (*ClientBegin)(edict_t *ent);
	void (*SelectSpawnPoint)(edict_t *ent, vec3_t origin, vec3_t angles);
	void (*PlayerDeath)(edict_t *targ, edict_t *inflictor, edict_t *attacker);
	void (*Score)(edict_t *attacker, edict_t *victim, int scoreChange);
	void (*PlayerEffects)(edict_t *ent);
	void (*DogTag)(edict_t *ent, edict_t *killer, char **pic);
	void (*PlayerDisconnect)(edict_t *ent);
	int (*ChangeDamage)(edict_t *targ, edict_t *attacker, int damage, int mod);
	int (*ChangeKnockback)(edict_t *targ, edict_t *attacker, int knockback, int mod);
	int (*CheckDMRules)(void);
} dm_game_rt;

extern dm_game_rt DMGame;

void Tag_GameInit(void);
void Tag_PostInitSetup(void);
void Tag_Score(edict_t *attacker, edict_t *victim, int scoreChange);
void Tag_PlayerEffects(edict_t *ent);
void Tag_DogTag(edict_t *ent, edict_t *killer, char **pic);
void Tag_PlayerDisconnect(edict_t *ent);
int Tag_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, int mod);

void DBall_GameInit(void);
void DBall_ClientBegin(edict_t *ent);
void DBall_SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles);
int DBall_ChangeKnockback(edict_t *targ, edict_t *attacker, int knockback, int mod);
int DBall_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, int mod);
void DBall_PostInitSetup(void);
int DBall_CheckDMRules(void);

/*
 * CTF Menu
 */
enum
{
	PMENU_ALIGN_LEFT,
	PMENU_ALIGN_CENTER,
	PMENU_ALIGN_RIGHT
};

typedef void (*SelectFunc_t)(edict_t *ent, pmenuhnd_t *hnd);

typedef struct pmenu_s
{
	char *text;
	int align;
	SelectFunc_t SelectFunc;
} pmenu_t;

pmenuhnd_t *PMenu_Open(edict_t *ent,
		pmenu_t *entries,
		int cur,
		int num,
		void *arg);
void PMenu_Close(edict_t *ent);
void PMenu_UpdateEntry(pmenu_t *entry,
		const char *text,
		int align,
		SelectFunc_t SelectFunc);
void PMenu_Do_Update(edict_t *ent);
void PMenu_Update(edict_t *ent);
void PMenu_Next(edict_t *ent);
void PMenu_Prev(edict_t *ent);
void PMenu_Select(edict_t *ent);

/*
 * CTF specific stuff.
 */

#define CTF_VERSION 1.52
#define CTF_VSTRING2(x) # x
#define CTF_VSTRING(x) CTF_VSTRING2(x)
#define CTF_STRING_VERSION CTF_VSTRING(CTF_VERSION)

#define STAT_CTF_TEAM1_PIC 18
#define STAT_CTF_TEAM1_CAPS 19
#define STAT_CTF_TEAM2_PIC 20
#define STAT_CTF_TEAM2_CAPS 21
#define STAT_CTF_FLAG_PIC 22
#define STAT_CTF_JOINED_TEAM1_PIC 23
#define STAT_CTF_JOINED_TEAM2_PIC 24
#define STAT_CTF_TEAM1_HEADER 25
#define STAT_CTF_TEAM2_HEADER 26
#define STAT_CTF_TECH 27
#define STAT_CTF_ID_VIEW 28
#define STAT_CTF_MATCH 29
#define STAT_CTF_ID_VIEW_COLOR 30
#define STAT_CTF_TEAMINFO 31

#define CONFIG_CTF_MATCH (CS_AIRACCEL - 1)
#define CONFIG_CTF_TEAMINFO (CS_AIRACCEL - 2)

typedef enum
{
	CTF_NOTEAM,
	CTF_TEAM1,
	CTF_TEAM2
} ctfteam_t;

typedef enum
{
	CTF_GRAPPLE_STATE_FLY,
	CTF_GRAPPLE_STATE_PULL,
	CTF_GRAPPLE_STATE_HANG
} ctfgrapplestate_t;

typedef struct ghost_s
{
	char netname[16];
	int number;

	/* stats */
	int deaths;
	int kills;
	int caps;
	int basedef;
	int carrierdef;

	int code; /* ghost code */
	int team; /* team */
	int score; /* frags at time of disconnect */
	edict_t *ent;
} ghost_t;

extern cvar_t *ctf;
extern char *ctf_statusbar;

#define CTF_TEAM1_SKIN "ctf_r"
#define CTF_TEAM2_SKIN "ctf_b"

#define DF_CTF_FORCEJOIN 131072
#define DF_ARMOR_PROTECT 262144
#define DF_CTF_NO_TECH 524288

#define CTF_CAPTURE_BONUS 15        /* what you get for capture */
#define CTF_TEAM_BONUS 10           /* what your team gets for capture */
#define CTF_RECOVERY_BONUS 1        /* what you get for recovery */
#define CTF_FLAG_BONUS 0            /* what you get for picking up enemy flag */
#define CTF_FRAG_CARRIER_BONUS 2    /* what you get for fragging enemy flag carrier */
#define CTF_FLAG_RETURN_TIME 40     /* seconds until auto return */

#define CTF_CARRIER_DANGER_PROTECT_BONUS 2      /* bonus for fraggin someone who has recently hurt your flag carrier */
#define CTF_CARRIER_PROTECT_BONUS 1             /* bonus for fraggin someone while either you or your target are near your flag carrier */
#define CTF_FLAG_DEFENSE_BONUS 1                /* bonus for fraggin someone while either you or your target are near your flag */
#define CTF_RETURN_FLAG_ASSIST_BONUS 1          /* awarded for returning a flag that causes a capture to happen almost immediately */
#define CTF_FRAG_CARRIER_ASSIST_BONUS 2         /* award for fragging a flag carrier if a capture happens almost immediately */

#define CTF_TARGET_PROTECT_RADIUS 400           /* the radius around an object being defended where a target will be worth extra frags */
#define CTF_ATTACKER_PROTECT_RADIUS 400         /* the radius around an object being defended where an attacker will get extra frags when making kills */

#define CTF_CARRIER_DANGER_PROTECT_TIMEOUT 8
#define CTF_FRAG_CARRIER_ASSIST_TIMEOUT 10
#define CTF_RETURN_FLAG_ASSIST_TIMEOUT 10

#define CTF_AUTO_FLAG_RETURN_TIMEOUT 30         /* number of seconds before dropped flag auto-returns */

#define CTF_TECH_TIMEOUT 60                     /* seconds before techs spawn again */

#define CTF_GRAPPLE_SPEED 650                   /* speed of grapple in flight */
#define CTF_GRAPPLE_PULL_SPEED 650              /* speed player is pulled at */

void CTFInit(void);
void CTFSpawn(void);
void CTFPrecache(void);

void SP_info_player_team1(edict_t *self);
void SP_info_player_team2(edict_t *self);

char *CTFTeamName(int team);
char *CTFOtherTeamName(int team);
void CTFAssignSkin(edict_t *ent, char *s);
void CTFAssignTeam(gclient_t *who);
edict_t *SelectCTFSpawnPoint(edict_t *ent);
qboolean CTFPickup_Flag(edict_t *ent, edict_t *other);
void CTFDrop_Flag(edict_t *ent, gitem_t *item);
void CTFEffects(edict_t *player);
void CTFCalcScores(void);
void SetCTFStats(edict_t *ent);
void CTFDeadDropFlag(edict_t *self);
void CTFScoreboardMessage(edict_t *ent, edict_t *killer);
void CTFTeam_f(edict_t *ent);
void CTFID_f(edict_t *ent);
void CTFSay_Team(edict_t *who, char *msg);
void CTFFlagSetup(edict_t *ent);
void CTFResetFlag(int ctf_team);
void CTFFragBonuses(edict_t *targ, edict_t *inflictor, edict_t *attacker);
void CTFCheckHurtCarrier(edict_t *targ, edict_t *attacker);

/* GRAPPLE */
void CTFWeapon_Grapple(edict_t *ent);
void CTFPlayerResetGrapple(edict_t *ent);
void CTFGrapplePull(edict_t *self);
void CTFResetGrapple(edict_t *self);

/* TECH */
gitem_t *CTFWhat_Tech(edict_t *ent);
qboolean CTFPickup_Tech(edict_t *ent, edict_t *other);
void CTFDrop_Tech(edict_t *ent, gitem_t *item);
void CTFDeadDropTech(edict_t *ent);
void CTFSetupTechSpawn(void);
int CTFApplyResistance(edict_t *ent, int dmg);
int CTFApplyStrength(edict_t *ent, int dmg);
qboolean CTFApplyStrengthSound(edict_t *ent);
qboolean CTFApplyHaste(edict_t *ent);
void CTFApplyHasteSound(edict_t *ent);
void CTFApplyRegeneration(edict_t *ent);
qboolean CTFHasRegeneration(edict_t *ent);
void CTFRespawnTech(edict_t *ent);
void CTFResetTech(void);

void CTFOpenJoinMenu(edict_t *ent);
qboolean CTFStartClient(edict_t *ent);
void CTFVoteYes(edict_t *ent);
void CTFVoteNo(edict_t *ent);
void CTFReady(edict_t *ent);
void CTFNotReady(edict_t *ent);
qboolean CTFNextMap(void);
qboolean CTFMatchSetup(void);
qboolean CTFMatchOn(void);
void CTFGhost(edict_t *ent);
void CTFAdmin(edict_t *ent);
qboolean CTFInMatch(void);
void CTFStats(edict_t *ent);
void CTFWarp(edict_t *ent);
void CTFBoot(edict_t *ent);
void CTFPlayerList(edict_t *ent);

qboolean CTFCheckRules(void);

void SP_misc_ctf_banner(edict_t *ent);
void SP_misc_ctf_small_banner(edict_t *ent);

void Cmd_Chasecam_Toggle(edict_t *ent);
void ChasecamStart(edict_t *ent);
void ChasecamRemove(edict_t *ent);
void CheckChasecam_Viewent(edict_t *ent);
void ChasecamTrack(edict_t *ent);

void CTFObserver(edict_t *ent);

void SP_trigger_teleport(edict_t *ent);
void SP_info_teleport_destination(edict_t *ent);

void CTFSetPowerUpEffect(edict_t *ent, int def);

qboolean Pickup_Adrenaline(edict_t * ent, edict_t * other);
qboolean Pickup_Ammo(edict_t * ent , edict_t * other);
qboolean Pickup_AncientHead(edict_t * ent, edict_t * other);
qboolean Pickup_Armor(edict_t * ent, edict_t * other);
qboolean Pickup_Bandolier(edict_t * ent, edict_t * other);
qboolean Pickup_Doppleganger(edict_t * ent, edict_t * other);
qboolean Pickup_Health(edict_t * ent, edict_t * other);
qboolean Pickup_Key(edict_t * ent, edict_t * other);
qboolean Pickup_Nuke(edict_t * ent, edict_t * other);
qboolean Pickup_Pack(edict_t * ent, edict_t * other);
qboolean Pickup_PowerArmor(edict_t * ent, edict_t * other);
qboolean Pickup_Powerup(edict_t * ent, edict_t * other);
qboolean Pickup_Sphere(edict_t * ent, edict_t * other);

void CopyToBodyQue(edict_t *ent);
void Use_Plat(edict_t *ent, edict_t *other, edict_t *activator);
void SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles);

/* platforms states */
#define STATE_TOP 0
#define STATE_BOTTOM 1
#define STATE_UP 2
#define STATE_DOWN 3

/*
 * Uncomment for check that exported functions declarations are same as in
 * implementation. (-Wmissing-prototypes )
 *
 */
#if 0
#include "../savegame/savegame.h"
#include "../savegame/tables/gamefunc_decs.h"
#endif

#endif /* GAME_LOCAL_H */

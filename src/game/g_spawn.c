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
 * Item spawning.
 *
 * =======================================================================
 */

#include "header/local.h"
#include "savegame/tables/spawnfunc_decs.h"

#define LEG_WAIT_TIME 1
#define MAX_LEGSFRAME 23

#define SPAWNGROW_LIFESPAN 0.3

typedef struct
{
	const char *name;
	void (*spawn)(edict_t *ent);
} spawn_t;

static spawn_t spawns[] = {
#include "savegame/tables/spawnfunc_list.h"
};

/* Definition of dynamic object */
typedef struct
{
	char classname[MAX_QPATH];
	/* could be up to three models */
	char model_path[MAX_QPATH * 3];
	vec3_t scale;
	char entity_type[MAX_QPATH];
	vec3_t mins;
	vec3_t maxs;
	char noshadow[MAX_QPATH];
	int solidflag;
	float walk_speed;
	float run_speed;
	int speed;
	int lighting;
	int blending;
	char target_sequence[MAX_QPATH];
	int misc_value;
	int no_mip;
	char spawn_sequence[MAX_QPATH];
	char description[MAX_QPATH];
	/* Additional fields */
	vec3_t color;
	int health;
	int mass;
	int damage;
	int damage_range;
	vec3_t damage_aim;
	gibtype_t gib;
	int gib_health;
} dynamicentity_t;

static dynamicentity_t *dynamicentities;
static int ndynamicentities;
static int nstaticentities;

static void
DynamicSpawnSetScale(edict_t *self)
{
	/* copy to other parts if zero */
	if (!st.scale[1])
	{
		st.scale[1] = st.scale[0];
	}

	if (!st.scale[2])
	{
		st.scale[2] = st.scale[0];
	}

	/* Copy to entity scale field */
	VectorCopy(st.scale, self->rrs.scale);
}

/*
 * Spawn method does not require any models to attach, so remove posible model
 * attached by dynamic spawn. In most cases spawn function will replace model
 * to correct one if need.
 */
void
DynamicResetSpawnModels(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.modelindex = 0;
	self->s.modelindex2 = 0;
	self->s.modelindex3 = 0;
}

static gibtype_t
DynamicSpawnGibFromName(const char *gib_type)
{
	if (!gib_type || !gib_type[0])
	{
		return GIB_NONE;
	}

	if (strlen(gib_type) <= 2 && gib_type[0] >= '0' && gib_type[0] <= '9')
	{
		/* Heretic 2: type is started from number */
		int gib_val;

		gib_val = (int)strtol(gib_type, (char **)NULL, 10);
		switch (gib_val)
		{
			case 0: return GIB_STONE;
			case 1: return GIB_GREYSTONE;
			case 2: return GIB_CLOTH;
			case 3: return GIB_METALLIC;
			case 4: return GIB_ORGANIC;
			case 5: return GIB_POTTERY;
			case 6: return GIB_GLASS;
			case 7: return GIB_LEAF;
			case 8: return GIB_WOOD;
			case 9: return GIB_BROWNSTONE;
			case 10: return GIB_NONE;
			case 11: return GIB_INSECT;
			default: return GIB_NONE;
		}
	}

	/* Combined ReMaster names and Heretic 2 materials */
	if (!strcmp(gib_type, "none"))
	{
		return GIB_NONE;
	}
	else if (!strcmp(gib_type, "metal") ||
		!strcmp(gib_type, "metallic"))
	{
		return GIB_METALLIC;
	}
	else if (!strcmp(gib_type, "flesh") ||
		!strcmp(gib_type, "organic"))
	{
		return GIB_ORGANIC;
	}
	else if (!strcmp(gib_type, "stone"))
	{
		return GIB_STONE;
	}
	else if (!strcmp(gib_type, "greystone"))
	{
		return GIB_GREYSTONE;
	}
	else if (!strcmp(gib_type, "cloth"))
	{
		return GIB_CLOTH;
	}
	else if (!strcmp(gib_type, "pottery"))
	{
		return GIB_POTTERY;
	}
	else if (!strcmp(gib_type, "glass"))
	{
		return GIB_GLASS;
	}
	else if (!strcmp(gib_type, "leaf"))
	{
		return GIB_LEAF;
	}
	else if (!strcmp(gib_type, "wood"))
	{
		return GIB_WOOD;
	}
	else if (!strcmp(gib_type, "brownstone"))
	{
		return GIB_BROWNSTONE;
	}
	else if (!strcmp(gib_type, "insect"))
	{
		return GIB_INSECT;
	}

	return GIB_NONE;
}

static void
DynamicSpawnUpdate(edict_t *self, dynamicentity_t *data)
{
	/* update properties by dynamic properties */
	char model_path[MAX_QPATH * 3];
	char *semicolon, *curr;

	Q_strlcpy(model_path, data->model_path,
		Q_min(sizeof(model_path), sizeof(data->model_path)));

	/* first model */
	curr = model_path;
	semicolon = strchr(curr, ';');
	if (semicolon)
	{
		*semicolon = 0;
		semicolon ++;
	}

	self->s.modelindex = gi.modelindex(curr);

	/* second model */
	if (semicolon)
	{
		curr = semicolon;
		semicolon = strchr(curr, ';');
		if (semicolon)
		{
			*semicolon = 0;
			semicolon ++;
		}
		self->s.modelindex2 = gi.modelindex(curr);
	}

	/* third model */
	if (semicolon)
	{
		curr = semicolon;
		semicolon = strchr(curr, ';');
		if (semicolon)
		{
			*semicolon = 0;
			semicolon ++;
		}
		self->s.modelindex3 = gi.modelindex(curr);
	}

	if (semicolon)
	{
		gi.dprintf("%s: '%s' use more than three models: %s\n",
			__func__, self->classname, semicolon);
	}

	VectorCopy(data->mins, self->mins);
	VectorCopy(data->maxs, self->maxs);
	VectorCopy(data->damage_aim, self->damage_aim);
	self->gib = data->gib;
	/* Heretic 2 material types */
	if (self->gibtype && self->gibtype[0])
	{
		self->gib = DynamicSpawnGibFromName(self->gibtype);
	}

	/* has updated scale */
	if (st.scale[0] || st.scale[1] || st.scale[2])
	{
		DynamicSpawnSetScale(self);
	}
	else
	{
		VectorCopy(data->scale, self->rrs.scale);
	}

	self->monsterinfo.scale = (
		data->scale[0] +
		data->scale[1] +
		data->scale[2]
	) / 3;

	self->monsterinfo.run_dist = data->run_speed;
	self->monsterinfo.walk_dist = data->walk_speed;

	/* override only by non zero value and only zero value*/
	if (data->health && !self->health)
	{
		self->health = data->health;
	}

	if (data->mass && !self->mass)
	{
		self->mass = data->mass;
	}

	if (data->damage && !self->dmg)
	{
		self->dmg = data->damage;
	}

	if (data->damage_range && !self->dmg_range)
	{
		self->dmg_range = data->damage_range;
	}

	/* set default solid flag */
	if (data->solidflag)
	{
		self->solid = SOLID_BBOX;
	}
	else
	{
		self->solid = SOLID_NOT;
	}
}

void
dynamicspawn_touch(edict_t *self, edict_t *other, cplane_t *plane /* unused */,
		csurface_t *surf /* unused */)
{
	if (!self || !other)
	{
		return;
	}

	if (!self->message || !self->message[0])
	{
		gi.centerprintf(other, "Entity classname: %s", self->classname);
		return;
	}

	gi.centerprintf(other, "Entity description: %s", self->message);
}

void
dynamicspawn_think(edict_t *self)
{
	M_SetAnimGroupFrame(self, "idle", true);
	self->nextthink = level.time + FRAMETIME;
}

static void
DynamicSpawn(edict_t *self, dynamicentity_t *data)
{
	const dmdxframegroup_t * frames;
	int num;

	/* All other properties could be updated in DynamicSpawnUpdate */
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;

	/* set message only if it has description */
	if (data->description[0])
	{
		self->message = data->description;
	}

	/* Set Mins/Maxs based on first frame */
	gi.GetModelFrameInfo(self->s.modelindex, self->s.frame,
		self->mins, self->maxs);

	gi.linkentity(self);

	/* Set Mins/Maxs based on whole model frames in animation group */
	frames = gi.GetModelInfo(self->s.modelindex, &num, NULL, NULL);
	if (frames && num)
	{
		qboolean has_idle = false;
		float speed;
		size_t i;

		monster_dynamic_setinfo(self);

		speed = (self->maxs[0] - self->mins[0]);

		if (self->monsterinfo.run_dist <= 0)
		{
			self->monsterinfo.run_dist = speed;
		}

		if (self->monsterinfo.walk_dist <= 0)
		{
			self->monsterinfo.walk_dist = speed / 2;
		}

		if (self->health <= 0)
		{
			self->health = 100;
		}

		if (self->mass <= 0)
		{
			self->mass = 100;
		}

		for (i = 0; i < num; i++)
		{
			if (!strcmp(frames[i].name, "walk") ||
				!strcmp(frames[i].name, "run"))
			{
				self->movetype = MOVETYPE_STEP;
				walkmonster_start(self);
				break;
			}
			else if (!strcmp(frames[i].name, "swim"))
			{
				self->movetype = MOVETYPE_STEP;
				swimmonster_start(self);
				break;
			}
			else if (!strcmp(frames[i].name, "fly"))
			{
				self->movetype = MOVETYPE_STEP;
				flymonster_start(self);
				break;
			}
			else if (!strcmp(frames[i].name, "idle"))
			{
				has_idle = true;
			}
		}

		if (has_idle && self->movetype != MOVETYPE_STEP)
		{
			self->think = dynamicspawn_think;
			self->nextthink = level.time + FRAMETIME;
		}
	}

	if (self->movetype != MOVETYPE_STEP)
	{
		self->touch = dynamicspawn_touch;
	}
}

static int
DynamicSpawnSearch(const char *name)
{
	int start, end;

	start = 0;
	end = ndynamicentities - 1;

	while (start <= end)
	{
		int i, res;

		i = start + (end - start) / 2;

		res = Q_stricmp(dynamicentities[i].classname, name);
		if (res == 0)
		{
			return i;
		}
		else if (res < 0)
		{
			start = i + 1;
		}
		else
		{
			end = i - 1;
		}
	}

	return -1;
}

static qboolean
Spawn_CheckCoop_MapHacks(edict_t *ent)
{
	if (!coop->value || !ent)
	{
		return false;
	}

	if (!Q_stricmp(level.mapname, "xsewer1"))
	{
		if (ent->classname && !Q_stricmp(ent->classname, "trigger_relay") && ent->target && !Q_stricmp(ent->target, "t3") && ent->targetname && !Q_stricmp(ent->targetname, "t2"))
		{
			return true;
		}
		if (ent->classname && !Q_stricmp(ent->classname, "func_button") && ent->target && !Q_stricmp(ent->target, "t16") && ent->model && !Q_stricmp(ent->model, "*71"))
		{
			ent->message = "Overflow valve maintenance\nhatch A opened.";
			return false;
		}

		if (ent->classname && !Q_stricmp(ent->classname, "trigger_once") && ent->model && !Q_stricmp(ent->model, "*3"))
		{
			ent->message = "Overflow valve maintenance\nhatch B opened.";
			return false;
		}
	}

	return false;
}

static const spawn_t *
StaticSpawnSearch(const char *classname)
{
	int start, end;

	start = 0;
	end = nstaticentities - 1;

	while (start <= end)
	{
		int i, res;

		i = start + (end - start) / 2;

		res = Q_stricmp(spawns[i].name, classname);
		if (res == 0)
		{
			return &spawns[i];
		}
		else if (res < 0)
		{
			start = i + 1;
		}
		else
		{
			end = i - 1;
		}
	}

	return NULL;
}

/*
 * Finds the spawn function for
 * the entity and calls it
 */
void
ED_CallSpawn(edict_t *ent)
{
	const spawn_t *s;
	gitem_t *item;
	int i, dyn_id;

	if (!ent)
	{
		return;
	}

	if (!ent->classname)
	{
		gi.dprintf("%s: NULL classname\n", __func__);
		G_FreeEdict(ent);
		return;
	}

	ent->gravityVector[0] = 0.0;
	ent->gravityVector[1] = 0.0;
	ent->gravityVector[2] = -1.0;

	if (st.health_multiplier <= 0)
	{
		st.health_multiplier = 1.0;
	}

	if (!strcmp(ent->classname, "weapon_nailgun"))
	{
		ent->classname = (FindItem("ETF Rifle"))->classname;
	}

	if (!strcmp(ent->classname, "ammo_nails"))
	{
		ent->classname = (FindItem("Flechettes"))->classname;
	}

	if (!strcmp(ent->classname, "weapon_heatbeam"))
	{
		ent->classname = (FindItem("Plasma Beam"))->classname;
	}

	/* search dynamic definitions */
	dyn_id = -1;
	if (dynamicentities && ndynamicentities)
	{
		dyn_id = DynamicSpawnSearch(ent->classname);

		if (dyn_id >= 0)
		{
			DynamicSpawnUpdate(ent, &dynamicentities[dyn_id]);
		}
	}

	/* No dynamic description */
	if (dyn_id < 0)
	{
		if (st.scale[0])
		{
			DynamicSpawnSetScale(ent);
		}
		else
		{
			VectorSet(ent->rrs.scale, 1.0, 1.0, 1.0);
		}
	}

	/* check item spawn functions */
	for (i = 0, item = itemlist; i < game.num_items; i++, item++)
	{
		if (!item->classname)
		{
			continue;
		}

		if (!strcmp(item->classname, ent->classname))
		{
			/* found it */
			SpawnItem(ent, item);
			return;
		}
	}

	/* check normal spawn functions */
	s = StaticSpawnSearch(ent->classname);
	if (s)
	{
		/* found it */
		s->spawn(ent);
		return;
	}

	if (dyn_id >= 0 && dynamicentities[dyn_id].model_path[0])
	{
		/* spawn only if know model */
		DynamicSpawn(ent, &dynamicentities[dyn_id]);

		return;
	}

	/* SiN entity could have model path as model field */
	if (ent->model && (ent->model[0] != '*') && (strlen(ent->model) > 4))
	{
		const char *ext;

		ext = COM_FileExtension(ent->model);
		if (!strcmp(ext, "def"))
		{
			dynamicentity_t self = {0};

			Q_strlcpy(self.classname, ent->classname, sizeof(self.classname));
			snprintf(self.model_path, sizeof(self.model_path), "models/%s", ent->model);

			if (gi.LoadFile(self.model_path, NULL) > 4)
			{
				/* Set default size */
				VectorSet(self.mins, -16, -16, -16);
				VectorSet(self.maxs, 16, 16, 16);

				DynamicSpawnUpdate(ent, &self);
				DynamicSpawn(ent, &self);
				return;
			}
		}
	}

	gi.dprintf("%s doesn't have a spawn function\n", ent->classname);
}

char *
ED_NewString(const char *string, qboolean raw)
{
	char *newb;
	size_t l;

	if (!string)
	{
		return NULL;
	}

	l = strlen(string) + 1;

	newb = gi.TagMalloc(l, TAG_LEVEL);

	if (!raw)
	{
		char *new_p;
		int i;

		new_p = newb;

		for (i = 0; i < l; i++)
		{
			if ((string[i] == '\\') && (i < l - 1))
			{
				i++;

				if (string[i] == 'n')
				{
					*new_p++ = '\n';
				}
				else
				{
					*new_p++ = '\\';
				}
			}
			else
			{
				*new_p++ = string[i];
			}
		}
	}
	else
	{
		/* just copy without convert */
		memcpy(newb, string, l);
	}

	return newb;
}

static unsigned
ED_ParseColorField(const char *value)
{
	/* space means rgba as values */
	if (strchr(value, ' '))
	{
		float v[4] = { 0, 0, 0, 1.0f };
		qboolean is_float = true;
		char *color_buffer, *tmp;
		int i;

		color_buffer = strdup(value);
		tmp = color_buffer;

		for (i = 0; i < 4; i++)
		{
			v[i] = (float)strtod(COM_Parse(&tmp), (char **)NULL);

			if (v[i] > 1.0f)
			{
				is_float = false;
			}

			if (!tmp)
			{
				break;
			}
		}
		free(color_buffer);

		/* convert to bytes */
		if (is_float)
		{
			for (i = 0; i < 4; i++)
			{
				v[i] *= 255.f;
			}
		}

		return ((byte)v[3]) |
				(((byte)v[2]) << 8) |
				(((byte)v[1]) << 16) |
				(((byte)v[0]) << 24);
	}

	/* integral */
	return atoi(value);
}

/*
 * Takes a key/value pair and sets
 * the binary values in an edict
 */
static void
ED_ParseField(const char *key, const char *value, edict_t *ent)
{
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	if (!ent || !value || !key)
	{
		return;
	}

	for (f = fields; f->name; f++)
	{
		if (!(f->flags & FFL_NOSPAWN) && !Q_strcasecmp(f->name, (char *)key))
		{
			/* found it */
			if (f->flags & FFL_SPAWNTEMP)
			{
				b = (byte *)&st;
			}
			else
			{
				b = (byte *)ent;
			}

			switch (f->type)
			{
				case F_LRAWSTRING:
					*(char **)(b + f->ofs) = ED_NewString(value, true);
					break;
				case F_LSTRING:
					*(char **)(b + f->ofs) = ED_NewString(value, false);
					break;
				case F_VECTOR:
					VectorClear(vec);
					sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
					((float *)(b + f->ofs))[0] = vec[0];
					((float *)(b + f->ofs))[1] = vec[1];
					((float *)(b + f->ofs))[2] = vec[2];
					break;
				case F_INT:
					*(int *)(b + f->ofs) = (int)strtol(value, (char **)NULL, 10);
					break;
				case F_FLOAT:
					*(float *)(b + f->ofs) = (float)strtod(value, (char **)NULL);
					break;
				case F_ANGLEHACK:
					v = (float)strtod(value, (char **)NULL);
					((float *)(b + f->ofs))[0] = 0;
					((float *)(b + f->ofs))[1] = v;
					((float *)(b + f->ofs))[2] = 0;
					break;
				case F_RGBA:
					*(unsigned *)(b + f->ofs) = ED_ParseColorField(value);
					break;
				case F_IGNORE:
					break;
				default:
					break;
			}

			return;
		}
	}

	gi.dprintf("'%s' is not a field. Value is '%s'\n", key, value);
}

/*
 * Parses an edict out of the given string,
 * returning the new position. ed should be
 * a properly initialized empty edict.
 */
static char *
ED_ParseEdict(char *data, edict_t *ent)
{
	qboolean init;
	char keyname[256];
	const char *com_token;

	if (!ent)
	{
		return NULL;
	}

	init = false;
	memset(&st, 0, sizeof(st));
	st.skyautorotate = 1;

	/* go through all the dictionary pairs */
	while (1)
	{
		/* parse key */
		com_token = COM_Parse(&data);

		if (com_token[0] == '}')
		{
			break;
		}

		if (!data)
		{
			gi.error("ED_ParseEntity: EOF without closing brace");
			break;
		}

		Q_strlcpy(keyname, com_token, sizeof(keyname));

		/* parse value */
		com_token = COM_Parse(&data);

		if (!data)
		{
			gi.error("%s: EOF without closing brace", __func__);
			break;
		}

		if (com_token[0] == '}')
		{
			gi.error("%s: closing brace without data", __func__);
			break;
		}

		init = true;

		/* keynames with a leading underscore are
		   used for utility comments, and are
		   immediately discarded by quake */
		if (keyname[0] == '_')
		{
			continue;
		}

		ED_ParseField(keyname, com_token, ent);
	}

	if (!init)
	{
		memset(ent, 0, sizeof(*ent));
	}

	return data;
}

/*
 * Chain together all entities with a matching team field.
 *
 * All but the first will have the FL_TEAMSLAVE flag set.
 * All but the last will have the teamchain field set to the next one
 */
static void
G_FixTeams(void)
{
	edict_t *e, *e2, *chain;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (!e->team)
		{
			continue;
		}

		if (!strcmp(e->classname, "func_train"))
		{
			if (e->flags & FL_TEAMSLAVE)
			{
				chain = e;
				e->teammaster = e;
				e->teamchain = NULL;
				e->flags &= ~FL_TEAMSLAVE;
				c++;
				c2++;

				for (j = 1, e2 = g_edicts + j;
					 j < globals.num_edicts;
					 j++, e2++)
				{
					if (e2 == e)
					{
						continue;
					}

					if (!e2->inuse)
					{
						continue;
					}

					if (!e2->team)
					{
						continue;
					}

					if (!strcmp(e->team, e2->team))
					{
						c2++;
						chain->teamchain = e2;
						e2->teammaster = e;
						e2->teamchain = NULL;
						chain = e2;
						e2->flags |= FL_TEAMSLAVE;
						e2->movetype = MOVETYPE_PUSH;
						e2->speed = e->speed;
					}
				}
			}
		}
	}

	gi.dprintf("%i teams repaired with %d entities\n", c, c2);
}

static void
G_FindTeams(void)
{
	edict_t *e, *e2, *chain;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (!e->team)
		{
			continue;
		}

		if (e->flags & FL_TEAMSLAVE)
		{
			continue;
		}

		chain = e;
		e->teammaster = e;
		c++;
		c2++;

		for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++)
		{
			if (!e2->inuse)
			{
				continue;
			}

			if (!e2->team)
			{
				continue;
			}

			if (e2->flags & FL_TEAMSLAVE)
			{
				continue;
			}

			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	G_FixTeams();

	gi.dprintf("%i teams with %i entities.\n", c, c2);
}

static int
SpawnEntitiesCount(const char *entities)
{
	int num_ent;

	/* little more space for dynamicly created monsters */
	num_ent = 128;

	while(*entities)
	{
		if (*entities == '{')
		{
			num_ent ++;
		}

		entities++;
	}

	return num_ent;
}

/*
 * Creates a server's entity / program execution context by
 * parsing textual entity definitions out of an ent file.
 */
void
SpawnEntities(const char *mapname, char *entities, const char *spawnpoint)
{
	edict_t *ent;
	int inhibit;
	const char *com_token;
	int i;
	float skill_level;

	if (!mapname || !entities || !spawnpoint)
	{
		return;
	}

	skill_level = floor(skill->value);

	if (skill_level < 0)
	{
		skill_level = 0;
	}

	if (skill_level > 3)
	{
		skill_level = 3;
	}

	if (skill->value != skill_level)
	{
		gi.cvar_forceset("skill", va("%f", skill_level));
	}

	SaveClientData();

	gi.FreeTags(TAG_LEVEL);

	ReinitGameEntities(SpawnEntitiesCount(entities));

	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));

	Q_strlcpy(level.mapname, mapname, sizeof(level.mapname));
	Q_strlcpy(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

	/* set client fields on player ents */
	for (i = 0; i < game.maxclients; i++)
	{
		g_edicts[i + 1].client = game.clients + i;
	}

	ent = NULL;
	inhibit = 0;

	/* parse ents */
	while (1)
	{
		/* parse the opening brace */
		com_token = COM_Parse(&entities);

		if (!entities)
		{
			break;
		}

		if (com_token[0] != '{')
		{
			gi.error("%s: found %s when expecting {", __func__, com_token);
			break;
		}

		if (!ent)
		{
			ent = g_edicts;
		}
		else
		{
			ent = G_Spawn();
		}

		entities = ED_ParseEdict(entities, ent);

		/* yet another map hack */
		if (!Q_stricmp(level.mapname, "command") &&
			!Q_stricmp(ent->classname, "trigger_once") &&
			!Q_stricmp(ent->model, "*27"))
		{
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;
		}

		/* ahh, the joys of map hacks .. */
		if (!Q_stricmp(level.mapname, "rhangar2") &&
			!Q_stricmp(ent->classname, "func_door_rotating") &&
			ent->targetname && !Q_stricmp(ent->targetname, "t265"))
		{
			ent->spawnflags &= ~SPAWNFLAG_NOT_COOP;
		}

		if (!Q_stricmp(level.mapname, "rhangar2") &&
			!Q_stricmp(ent->classname, "trigger_always") &&
			ent->target && !Q_stricmp(ent->target, "t265"))
		{
			ent->spawnflags |= SPAWNFLAG_NOT_COOP;
		}

		if (!Q_stricmp(level.mapname, "rhangar2") &&
			!Q_stricmp(ent->classname, "func_wall") &&
			!Q_stricmp(ent->model, "*15"))
		{
			ent->spawnflags |= SPAWNFLAG_NOT_COOP;
		}

		/* remove things (except the world) from
		   different skill levels or deathmatch */
		if (ent != g_edicts)
		{
			if (deathmatch->value)
			{
				if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}
			else if (coop->value && !coop_baseq2->value)
			{
				if (ent->spawnflags & SPAWNFLAG_NOT_COOP)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}

				/* stuff marked !easy & !med & !hard are coop only, all levels */
				if (!((ent->spawnflags & SPAWNFLAG_NOT_EASY) &&
					  (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM) &&
					  (ent->spawnflags & SPAWNFLAG_NOT_HARD)))
				{
					if (((skill->value == SKILL_EASY) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
						((skill->value == SKILL_MEDIUM) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
						(((skill->value == SKILL_HARD) || (skill->value == SKILL_HARDPLUS)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD)))
					{
						G_FreeEdict(ent);
						inhibit++;
						continue;
					}
				}
			}
			else
			{
				if (Spawn_CheckCoop_MapHacks(ent) || (
					((skill->value == SKILL_EASY) &&
					 (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == SKILL_MEDIUM) &&
					 (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill->value == SKILL_HARD) ||
					  (skill->value == SKILL_HARDPLUS)) &&
					 (ent->spawnflags & SPAWNFLAG_NOT_HARD)))
					)
				{
					G_FreeEdict(ent);
					inhibit++;
					continue;
				}
			}

			ent->spawnflags &=
				~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM |
				  SPAWNFLAG_NOT_HARD |
				  SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);
		}

		ent->gravityVector[0] = 0.0;
		ent->gravityVector[1] = 0.0;
		ent->gravityVector[2] = -1.0;

		ED_CallSpawn(ent);

		ent->s.renderfx |= RF_IR_VISIBLE;
	}

	/* in case the last entity in the entstring has spawntemp fields */
	memset(&st, 0, sizeof(st));

	gi.dprintf("%i entities inhibited.\n", inhibit);

	G_FindTeams();

	PlayerTrail_Init();

	if (deathmatch->value)
	{
		if (randomrespawn && randomrespawn->value)
		{
			PrecacheForRandomRespawn();
		}
	}
	else
	{
		InitHintPaths();
	}

	if (deathmatch->value && gamerules && gamerules->value)
	{
		if (DMGame.PostInitSetup)
		{
			DMGame.PostInitSetup();
		}
	}

	if (ctf->value)
	{
		CTFSpawn();
	}

	AI_NewMap();//JABot

	/* setup server-side shadow lights */
	setup_shadow_lights();
}

/* =================================================================== */

static char *single_statusbar =
	"yb	-24 "

/* health */
	"xv	0 "
	"hnum "
	"xv	50 "
	"pic 0 "

/* ammo */
	"if 2 "
	"	xv	100 "
	"	anum "
	"	xv	150 "
	"	pic 2 "
	"endif "

/* armor */
	"if 4 "
	"	xv	200 "
	"	rnum "
	"	xv	250 "
	"	pic 4 "
	"endif "

/* selected item */
	"if 6 "
	"	xv	296 "
	"	pic 6 "
	"endif "

	"yb	-50 "

/* picked up item */
	"if 7 "
	"	xv	0 "
	"	pic 7 "
	"	xv	26 "
	"	yb	-42 "
	"	stat_string 8 "
	"	yb	-50 "
	"endif "

/* timer */
	"if 9 "
	"	xv	262 "
	"	num	2	10 "
	"	xv	296 "
	"	pic	9 "
	"endif "

/*  help / weapon icon */
	"if 11 "
	"	xv	148 "
	"	pic	11 "
	"endif "
;

static char *dm_statusbar =
	"yb	-24 "

/* health */
	"xv	0 "
	"hnum "
	"xv	50 "
	"pic 0 "

/* ammo */
	"if 2 "
	"	xv	100 "
	"	anum "
	"	xv	150 "
	"	pic 2 "
	"endif "

/* armor */
	"if 4 "
	"	xv	200 "
	"	rnum "
	"	xv	250 "
	"	pic 4 "
	"endif "

/* selected item */
	"if 6 "
	"	xv	296 "
	"	pic 6 "
	"endif "

	"yb	-50 "

/* picked up item */
	"if 7 "
	"	xv	0 "
	"	pic 7 "
	"	xv	26 "
	"	yb	-42 "
	"	stat_string 8 "
	"	yb	-50 "
	"endif "

/* timer */
	"if 9 "
	"	xv	246 "
	"	num	2	10 "
	"	xv	296 "
	"	pic	9 "
	"endif "

/* help / weapon icon */
	"if 11 "
	"	xv	148 "
	"	pic	11 "
	"endif "

/* frags */
	"xr	-50 "
	"yt 2 "
	"num 3 14 "

/* spectator */
	"if 17 "
	"xv 0 "
	"yb -58 "
	"string2 \"SPECTATOR MODE\" "
	"endif "

/* chase camera */
	"if 16 "
	"xv 0 "
	"yb -68 "
	"string \"Chasing\" "
	"xv 64 "
	"stat_string 16 "
	"endif "
;

static char *roarke_statusbar =
	"yb	-70 "

/* health */
	"xl	3 "
	"pic 0 "
	"yb	-68 "
	"xl	35 "
	"hnum "

/* draw ammo value */
	"yb	-35 "
	"xl	3 "
	"pic 8 "
	"yb	-33 "
	"xl	35 "
	"num 3 9 "
	"yt	5 "
	"xr	-35 "
	"pic 31 "

/* selected item */
	"if 6 "
	"	yt	45 "
	"	xr	-70 "
	"	num	2	7 "
	"	xr	-35 "
	"	pic	6 "
	"endif "

/* chase camera */
	"if 16 "
	"	yb	-105 "
	"	xr	-35 "
	"	pic 16 "
	"endif "

/* ammo */
	"if 2 "
	"	yb	-70 "
	"	xr	-87 "
	"	anum "
	"	yb	-68 "
	"	xr	-35 "
	"	pic 2 "
	"endif "

/* armor */
	"if 4 "
	"	yb	-35 "
	"	xr	-87 "
	"	rnum "
	"	yb	-33 "
	"	xr	-35 "
	"	pic 4 "
	"endif "

/* selected item */
	"if 12 "
	"	xv	145 "
	"	yt 5 "
	"	pic 12 "
	"endif"
;

/*
 * QUAKED worldspawn (0 0 0) ?
 *
 * Only used for the world.
 * "sky"	environment map name
 * "skyaxis"	vector axis for rotating sky
 * "skyrotate"	speed of rotation in degrees/second
 * "sounds"	music cd track number
 * "gravity"	800 is default gravity
 * "message"	text to print at user logon
 */
void
SP_worldspawn(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true; /* since the world doesn't use G_Spawn() */
	ent->s.modelindex = 1; /* world model is always index 1 */

	/* --------------- */

	/* reserve some spots for dead
	   player bodies for coop / deathmatch */
	InitBodyQue();

	/* set configstrings for items */
	SetItemNames();

	if (st.nextmap)
	{
		Q_strlcpy(level.nextmap, st.nextmap, sizeof(level.nextmap));
	}

	/* make some data visible to the server */
	if (ent->message && ent->message[0])
	{
		Q_strlcpy(level.level_name, gi.LocalizationMessage(ent->message, NULL),
			sizeof(level.level_name));
		gi.configstring(CS_NAME, level.level_name);
	}
	else
	{
		Q_strlcpy(level.level_name, level.mapname, sizeof(level.level_name));
	}

	if (st.sky && st.sky[0])
	{
		gi.configstring(CS_SKY, st.sky);
	}
	else
	{
		gi.configstring(CS_SKY, "unit1_");
	}

	gi.configstring(CS_SKYROTATE, va("%f %d", st.skyrotate, st.skyautorotate));

	gi.configstring(CS_SKYAXIS, va("%f %f %f",
				st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));

	if (st.music && st.music[0])
	{
		gi.configstring(CS_CDTRACK, st.music);
	}
	else
	{
		gi.configstring(CS_CDTRACK, va("%i", ent->sounds));
	}

	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));

	/* status bar program */
	if (deathmatch->value)
	{
		if (ctf->value)
		{
			gi.configstring(CS_STATUSBAR, ctf_statusbar);
			CTFPrecache();
		}
		else
		{
			gi.configstring(CS_STATUSBAR, dm_statusbar);
		}
	}
	else
	{
		if (!strcmp(g_game->string, "roarke")) /* DoD */
		{
			gi.configstring(CS_STATUSBAR, roarke_statusbar);
		}
		else
		{
			gi.configstring(CS_STATUSBAR, single_statusbar);
		}
	}

	if (st.start_items && *st.start_items)
	{
		level.start_items = st.start_items;
	}
	else
	{
		level.start_items = NULL;
	}

	/* --------------- */

	/* help icon for statusbar */
	gi.imageindex("i_help");
	if (!strcmp(g_game->string, "roarke")) /* DoD */
	{
		level.pic_health = gi.imageindex("i_life");
	}
	else
	{
		level.pic_health = gi.imageindex("i_health");
	}
	gi.imageindex("help");
	gi.imageindex("field_3");

	if (!st.gravity)
	{
		gi.cvar_set("sv_gravity", "800");
	}
	else
	{
		gi.cvar_set("sv_gravity", st.gravity);
	}

	snd_fry = gi.soundindex("player/fry.wav"); /* standing in lava / slime */

	PrecacheItem(FindItem("Blaster"));

	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");

	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");

	gi.soundindex("misc/udeath.wav");

	/* gibs */
	gi.soundindex("items/respawn1.wav");

	/* sexed sounds */
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");
	gi.soundindex("*gurp1.wav"); /* drowning damage */
	gi.soundindex("*gurp2.wav");
	gi.soundindex("*jump1.wav"); /* player jump */
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");

	/* sexed models: THIS ORDER MUST MATCH THE DEFINES IN g_local.h
	   you can add more, max 19 (pete change)these models are only
	   loaded in coop or deathmatch. not singleplayer. */
	if (coop->value || deathmatch->value || ctf->value)
	{
		gi.modelindex("#w_blaster.md2");
		gi.modelindex("#w_shotgun.md2");
		gi.modelindex("#w_sshotgun.md2");
		gi.modelindex("#w_machinegun.md2");
		gi.modelindex("#w_chaingun.md2");
		gi.modelindex("#a_grenades.md2");
		gi.modelindex("#w_glauncher.md2");
		gi.modelindex("#w_rlauncher.md2");
		gi.modelindex("#w_hyperblaster.md2");
		gi.modelindex("#w_railgun.md2");
		gi.modelindex("#w_bfg.md2");
		gi.modelindex("#w_grapple.md2");
		gi.modelindex("#w_disrupt.md2");
		gi.modelindex("#w_etfrifle.md2");
		gi.modelindex("#w_plasma.md2");
		gi.modelindex("#w_plauncher.md2");
		gi.modelindex("#w_chainfist.md2");

		gi.modelindex("#w_phalanx.md2");
		gi.modelindex("#w_ripper.md2");
	}

	/* ------------------- */

	gi.soundindex("player/gasp1.wav");      /* gasping for air */
	gi.soundindex("player/gasp2.wav");      /* head breaking surface, not gasping */

	gi.soundindex("player/watr_in.wav");    /* feet hitting water */
	gi.soundindex("player/watr_out.wav");   /* feet leaving water */

	gi.soundindex("player/watr_un.wav");    /* head going underwater */

	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");

	gi.soundindex("items/pkup.wav");        /* bonus item pickup */
	gi.soundindex("world/land.wav");        /* landing thud */
	gi.soundindex("misc/h2ohit1.wav");      /* landing splash */

	gi.soundindex("items/damage.wav");
	gi.soundindex("misc/ddamage1.wav");
	gi.soundindex("items/protect.wav");
	gi.soundindex("items/protect4.wav");
	gi.soundindex("weapons/noammo.wav");

	gi.soundindex("infantry/inflies1.wav");

	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/arm/tris.md2");
	gi.modelindex("models/objects/gibs/bone/tris.md2");
	gi.modelindex("models/objects/gibs/bone2/tris.md2");
	gi.modelindex("models/objects/gibs/chest/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");
	gi.modelindex("models/objects/gibs/head2/tris.md2");

	/* Setup light animation tables. 'a'
	   is total darkness, 'z' is doublebright. */

	/* 0 normal */
	gi.configstring(CS_LIGHTS + 0, "m");

	/* 1 FLICKER (first variety) */
	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

	/* 2 SLOW STRONG PULSE */
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	/* 3 CANDLE (first variety) */
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	/* 4 FAST STROBE */
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");

	/* 5 GENTLE PULSE 1 */
	gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	/* 6 FLICKER (second variety) */
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");

	/* 7 CANDLE (second variety) */
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

	/* 8 CANDLE (third variety) */
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	/* 9 SLOW STROBE (fourth variety) */
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

	/* 10 FLUORESCENT FLICKER */
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

	/* 11 SLOW PULSE NOT FADE TO BLACK */
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	/* styles 32-62 are assigned by the light program for switchable lights */

	/* 63 testing */
	gi.configstring(CS_LIGHTS + 63, "a");
}

/*
 * Monster spawning code:
 * Used by the carrier, the medic_commander, and the black widow
 *
 * The sequence to create a flying monster is:
 *  FindSpawnPoint - tries to find suitable spot to spawn the monster in
 *  CreateFlyMonster  - this verifies the point as good and creates the monster
 *
 * To create a ground walking monster:
 *  FindSpawnPoint - same thing
 *  CreateGroundMonster - this checks the volume and makes sure the floor under the volume is suitable
 */

static edict_t *
CreateMonster(vec3_t origin, vec3_t angles, char *classname)
{
	edict_t *newEnt;

	if (!classname)
	{
		return NULL;
	}

	newEnt = G_Spawn();

	VectorCopy(origin, newEnt->s.origin);
	VectorCopy(angles, newEnt->s.angles);
	newEnt->classname = ED_NewString(classname, true);
	newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	VectorSet(newEnt->gravityVector, 0, 0, -1);
	ED_CallSpawn(newEnt);
	newEnt->s.renderfx |= RF_IR_VISIBLE;

	return newEnt;
}

static void
DetermineBBox(char *classname, vec3_t mins, vec3_t maxs)
{
	edict_t *newEnt;

	if (!classname)
	{
		return;
	}

	newEnt = G_Spawn();

	VectorCopy(vec3_origin, newEnt->s.origin);
	VectorCopy(vec3_origin, newEnt->s.angles);
	newEnt->classname = ED_NewString(classname, true);
	newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	ED_CallSpawn(newEnt);

	if (mins)
	{
		VectorCopy(newEnt->mins, mins);
	}

	if (maxs)
	{
		VectorCopy(newEnt->maxs, maxs);
	}

	G_FreeEdict(newEnt);
}

edict_t *
CreateFlyMonster(vec3_t origin, vec3_t angles, vec3_t mins,
		vec3_t maxs, char *classname)
{
	if (!classname)
	{
		return NULL;
	}

	if (!mins || !maxs ||
		VectorCompare(mins, vec3_origin) || VectorCompare(maxs, vec3_origin))
	{
		DetermineBBox(classname, mins, maxs);
	}

	if (!CheckSpawnPoint(origin, mins, maxs))
	{
		return NULL;
	}

	return CreateMonster(origin, angles, classname);
}

edict_t *
CreateGroundMonster(vec3_t origin, vec3_t angles, vec3_t entMins,
		vec3_t entMaxs, char *classname, int height)
{
	edict_t *newEnt;
	vec3_t mins, maxs;

	if (!classname)
	{
		return NULL;
	}

	/* if they don't provide us a bounding box, figure it out */
	if (!entMins || !entMaxs || VectorCompare(entMins,
				vec3_origin) || VectorCompare(entMaxs, vec3_origin))
	{
		DetermineBBox(classname, mins, maxs);
	}
	else
	{
		VectorCopy(entMins, mins);
		VectorCopy(entMaxs, maxs);
	}

	/* check the ground to make sure it's there, it's relatively flat, and it's not toxic */
	if (!CheckGroundSpawnPoint(origin, mins, maxs, height, -1))
	{
		return NULL;
	}

	newEnt = CreateMonster(origin, angles, classname);

	if (!newEnt)
	{
		return NULL;
	}

	return newEnt;
}

qboolean
FindSpawnPoint(vec3_t startpoint, vec3_t mins, vec3_t maxs,
		vec3_t spawnpoint, float maxMoveUp)
{
	trace_t tr;
	vec3_t top;

	tr = gi.trace(startpoint, mins, maxs, startpoint,
			NULL, MASK_MONSTERSOLID | CONTENTS_PLAYERCLIP);

	if ((tr.startsolid || tr.allsolid) || (tr.ent != world))
	{
		VectorCopy(startpoint, top);
		top[2] += maxMoveUp;

		tr = gi.trace(top, mins, maxs, startpoint, NULL, MASK_MONSTERSOLID);

		if (tr.startsolid || tr.allsolid)
		{
			return false;
		}
		else
		{
			VectorCopy(tr.endpos, spawnpoint);
			return true;
		}
	}
	else
	{
		VectorCopy(startpoint, spawnpoint);
		return true;
	}
}

qboolean
CheckSpawnPoint(vec3_t origin, vec3_t mins, vec3_t maxs)
{
	trace_t tr;

	if (!mins || !maxs ||
		VectorCompare(mins, vec3_origin) || VectorCompare(maxs, vec3_origin))
	{
		return false;
	}

	tr = gi.trace(origin, mins, maxs, origin, NULL, MASK_MONSTERSOLID);

	if (tr.startsolid || tr.allsolid)
	{
		return false;
	}

	if (tr.ent != world)
	{
		return false;
	}

	return true;
}

qboolean
CheckGroundSpawnPoint(vec3_t origin, vec3_t entMins, vec3_t entMaxs,
		float height, float gravity)
{
	trace_t tr;
	vec3_t start, stop;
	vec3_t mins, maxs;
	int x, y;
	float mid, bottom;

	if (!CheckSpawnPoint(origin, entMins, entMaxs))
	{
		return false;
	}

	VectorCopy(origin, stop);
	stop[2] = origin[2] + entMins[2] - height;

	tr = gi.trace(origin, entMins, entMaxs, stop,
			NULL, MASK_MONSTERSOLID | MASK_WATER);

	if ((tr.fraction < 1) && (tr.contents & MASK_MONSTERSOLID))
	{
		/* first, do the midpoint trace */
		VectorAdd(tr.endpos, entMins, mins);
		VectorAdd(tr.endpos, entMaxs, maxs);

		/* first, do the easy flat check */
		if (gravity > 0)
		{
			start[2] = maxs[2] + 1;
		}
		else
		{
			start[2] = mins[2] - 1;
		}

		for (x = 0; x <= 1; x++)
		{
			for (y = 0; y <= 1; y++)
			{
				start[0] = x ? maxs[0] : mins[0];
				start[1] = y ? maxs[1] : mins[1];

				if (gi.pointcontents(start) != CONTENTS_SOLID)
				{
					goto realcheck;
				}
			}
		}

		/* if it passed all four above checks, we're done */
		return true;

	realcheck:

		/* check it for real */
		start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
		start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
		start[2] = mins[2];

		tr = gi.trace(start, vec3_origin, vec3_origin,
				stop, NULL, MASK_MONSTERSOLID);

		if (tr.fraction == 1.0)
		{
			return false;
		}

		if (gravity < 0)
		{
			start[2] = mins[2];
			stop[2] = start[2] - (STEPSIZE * 2);
			mid = bottom = tr.endpos[2] + entMins[2];
		}
		else
		{
			start[2] = maxs[2];
			stop[2] = start[2] + (STEPSIZE * 2);
			mid = bottom = tr.endpos[2] - entMaxs[2];
		}

		for (x = 0; x <= 1; x++)
		{
			for (y = 0; y <= 1; y++)
			{
				start[0] = stop[0] = x ? maxs[0] : mins[0];
				start[1] = stop[1] = y ? maxs[1] : mins[1];

				tr = gi.trace(start, vec3_origin, vec3_origin,
						stop, NULL, MASK_MONSTERSOLID);

				if (gravity > 0)
				{
					if ((tr.fraction != 1.0) && (tr.endpos[2] < bottom))
					{
						bottom = tr.endpos[2];
					}

					if ((tr.fraction == 1.0) || (tr.endpos[2] - mid > STEPSIZE))
					{
						return false;
					}
				}
				else
				{
					if ((tr.fraction != 1.0) && (tr.endpos[2] > bottom))
					{
						bottom = tr.endpos[2];
					}

					if ((tr.fraction == 1.0) || (mid - tr.endpos[2] > STEPSIZE))
					{
						return false;
					}
				}
			}
		}

		return true; /* we can land on it, it's ok */
	}

	/* otherwise, it's either water (bad) or not
	 * there (too far) if we're here, it's bad below */
	return false;
}

void
spawngrow_think(edict_t *self)
{
	int i;

	if (!self)
	{
		return;
	}

	for (i = 0; i < 2; i++)
	{
		self->s.angles[PITCH] = rand() % 360;
		self->s.angles[YAW] = rand() % 360;
		self->s.angles[ROLL] = rand() % 360;
	}

	if ((level.time < self->wait) && (self->s.frame < 2))
	{
		self->s.frame++;
	}

	if (level.time >= self->wait)
	{
		if (self->s.effects & EF_SPHERETRANS)
		{
			G_FreeEdict(self);
			return;
		}
		else if (self->s.frame > 0)
		{
			self->s.frame--;
		}
		else
		{
			G_FreeEdict(self);
			return;
		}
	}

	self->nextthink += FRAMETIME;
}

void
SpawnGrow_Spawn(vec3_t startpos, int size)
{
	edict_t *ent;
	int i;
	float lifespan;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);

	for (i = 0; i < 2; i++)
	{
		ent->s.angles[PITCH] = rand() % 360;
		ent->s.angles[YAW] = rand() % 360;
		ent->s.angles[ROLL] = rand() % 360;
	}

	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "spawngro";

	if (size <= 1)
	{
		lifespan = SPAWNGROW_LIFESPAN;
		ent->s.modelindex = gi.modelindex("models/items/spawngro2/tris.md2");
	}
	else if (size == 2)
	{
		ent->s.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
		lifespan = 2;
	}
	else
	{
		ent->s.modelindex = gi.modelindex("models/items/spawngro/tris.md2");
		lifespan = SPAWNGROW_LIFESPAN;
	}

	ent->think = spawngrow_think;

	ent->wait = level.time + lifespan;
	ent->nextthink = level.time + FRAMETIME;

	if (size != 2)
	{
		ent->s.effects |= EF_SPHERETRANS;
	}

	gi.linkentity(ent);
}

void
widowlegs_think(edict_t *self)
{
	vec3_t offset;
	vec3_t point;
	vec3_t f, r, u;

	if (!self)
	{
		return;
	}

	if (self->s.frame == 17)
	{
		VectorSet(offset, 11.77, -7.24, 23.31);
		AngleVectors(self->s.angles, f, r, u);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);
	}

	if (self->s.frame < MAX_LEGSFRAME)
	{
		self->s.frame++;
		self->nextthink = level.time + FRAMETIME;
		return;
	}
	else if (self->wait == 0)
	{
		self->wait = level.time + LEG_WAIT_TIME;
	}

	if (level.time > self->wait)
	{
		AngleVectors(self->s.angles, f, r, u);

		VectorSet(offset, -65.6, -8.44, 28.59);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);

		VectorSet(offset, -1.04, -51.18, 7.04);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib3/tris.md2",
				80 + (int)(random() * 20.0), GIB_METALLIC, point, 0, true);

		G_FreeEdict(self);
		return;
	}

	if ((level.time > (self->wait - 0.5)) && (self->count == 0))
	{
		self->count = 1;
		AngleVectors(self->s.angles, f, r, u);

		VectorSet(offset, 31, -88.7, 10.96);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);

		VectorSet(offset, -12.67, -4.39, 15.68);
		G_ProjectSource2(self->s.origin, offset, f, r, u, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL);

		self->nextthink = level.time + FRAMETIME;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void
Widowlegs_Spawn(vec3_t startpos, vec3_t angles)
{
	edict_t *ent;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);
	VectorCopy(angles, ent->s.angles);
	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "widowlegs";

	ent->s.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
	ent->think = widowlegs_think;

	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);
}

static char *
DynamicStringParse(char *line, char *field, int size, char separator)
{
	char *next_section, *current_section;

	/* search line end */
	current_section = line;
	next_section = strchr(line, separator);
	if (next_section)
	{
		*next_section = 0;
		line = next_section + 1;
	}

	/* copy current line state */
	Q_strlcpy(field, current_section, size);

	return line;
}

static char *
DynamicIntParse(char *line, int *field, char separator)
{
	char *next_section;

	next_section = strchr(line, separator);
	if (next_section)
	{
		*next_section = 0;
		*field = (int)strtol(line, (char **)NULL, 10);
		line = next_section + 1;
	}

	return line;
}

static char *
DynamicFloatParse(char *line, float *field, int size, char separator)
{
	int i;

	for (i = 0; i < size; i++)
	{
		char *next_section, *current_section;

		current_section = line;
		next_section = strchr(line, separator);
		if (next_section)
		{
			*next_section = 0;
			line = next_section + 1;
		}
		field[i] = (float)strtod(current_section, (char **)NULL);
	}
	return line;
}

static char *
DynamicSkipParse(char *line, int size, char separator)
{
	int i;

	for (i = 0; i < size; i++)
	{
		char *next_section;

		next_section = strchr(line, separator);
		if (next_section)
		{
			*next_section = 0;
			line = next_section + 1;
		}
	}
	return line;
}

static int
DynamicSort(const void *p1, const void *p2)
{
	dynamicentity_t *ent1, *ent2;

	ent1 = (dynamicentity_t*)p1;
	ent2 = (dynamicentity_t*)p2;
	return Q_stricmp(ent1->classname, ent2->classname);
}

static void
DynamicSpawnInit(void)
{
	char *buf_ent, *buf_ai, *raw;
	int len_ent, len_ai, curr_pos;

	buf_ent = NULL;
	len_ent = 0;
	buf_ai = NULL;
	len_ai = 0;

	dynamicentities = NULL;
	ndynamicentities = 0;

	/* load the aidata file */
	len_ai = gi.LoadFile("aidata.vsc", (void **)&raw);
	if (len_ai > 1)
	{
		if (len_ai > 4 && !strncmp(raw, "CVSC", 4))
		{
			int i;

			len_ai -= 4;
			buf_ai = malloc(len_ai + 1);
			if (buf_ai)
			{
				memcpy(buf_ai, raw + 4, len_ai);
				for (i = 0; i < len_ai; i++)
				{
					buf_ai[i] = buf_ai[i] ^ 0x96;
				}
				buf_ai[len_ai] = 0;
			}
		}
		gi.FreeFile(raw);
	}

	/* load the file */
	len_ent = gi.LoadFile("models/entity.dat", (void **)&raw);
	if (len_ent > 1)
	{
		buf_ent = malloc(len_ent + 1);
		if (!buf_ent)
		{
			gi.dprintf("%s: cant allocate buffer for entities list\n", __func__);
		}
		else
		{
			memcpy(buf_ent, raw, len_ent);
			buf_ent[len_ent] = 0;
		}

		gi.FreeFile(raw);
	}

	/* aidata definition lines count */
	if (buf_ai)
	{
		char *curr;

		/* get lines count */
		curr = buf_ai;
		while (*curr)
		{
			size_t linesize = 0;

			linesize = strcspn(curr, "\n\r");
			if (*curr &&  *curr != '\n' && *curr != '\r' && *curr != ',')
			{
				ndynamicentities ++;
			}

			curr += linesize;
			if (curr >= (buf_ai + len_ai))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* entitiyty definition lines count */
	if (buf_ent)
	{
		char *curr;

		/* get lines count */
		curr = buf_ent;
		while (*curr)
		{
			size_t linesize = 0;

			linesize = strcspn(curr, "\n\r");
			if (*curr && strncmp(curr, "//", 2) &&
				*curr != '\n' && *curr != '\r' && *curr != ';')
			{
				ndynamicentities ++;
			}
			curr += linesize;
			if (curr >= (buf_ent + len_ent))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	if (ndynamicentities)
	{
		dynamicentities = malloc(ndynamicentities * sizeof(*dynamicentities));
		if (!dynamicentities)
		{
			ndynamicentities = 0;
			free(buf_ent);
			free(buf_ai);
			gi.error("%s: Can't allocate memory for dynamicentities\n");
			return;
		}

		memset(dynamicentities, 0, ndynamicentities * sizeof(*dynamicentities));
	}
	curr_pos = 0;

	if (buf_ai)
	{
		char *curr;

		/* get lines count */
		curr = buf_ai;
		while (*curr)
		{
			size_t linesize = 0;

			if (curr_pos >= ndynamicentities)
			{
				break;
			}

			/* skip empty */
			linesize = strspn(curr, "\n\r\t ");
			curr += linesize;

			/* mark end line */
			linesize = strcspn(curr, "\n\r");
			curr[linesize] = 0;

			if (*curr &&  *curr != '\n' && *curr != '\r' && *curr != ',')
			{
				char *line, scale[MAX_QPATH];

				line = curr;
				line = DynamicStringParse(line, dynamicentities[curr_pos].classname, MAX_QPATH, ',');
				line = DynamicStringParse(line, dynamicentities[curr_pos].model_path,
					sizeof(dynamicentities[curr_pos].model_path), ',');
				/* Skipped: audio file definition */
				line = DynamicSkipParse(line, 1, ',');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].health, ',');
				/*
				 * Skipped:
					 * basehealth
					 * elasticity
				*/
				line = DynamicSkipParse(line, 2, ',');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].mass, ',');
				/* Skipped: angle speed */
				line = DynamicSkipParse(line, 1, ',');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].mins, 3, ',');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].maxs, 3, ',');
				line = DynamicStringParse(line, scale, MAX_QPATH, ',');
				/* parse to 3 floats */
				DynamicFloatParse(scale, dynamicentities[curr_pos].scale, 3, ' ');
				/*
				 * Ignored fields:
					* active distance
					* attack distance
					* jump attack distance
					* upward velocity
				*/
				line = DynamicSkipParse(line, 4, ',');
				line = DynamicFloatParse(line, &dynamicentities[curr_pos].run_speed, 1, ',');
				line = DynamicFloatParse(line, &dynamicentities[curr_pos].walk_speed, 1, ',');
				/*
				 * Ignored fields:
					* attack speed
					* fov
				 */
				line = DynamicSkipParse(line, 2, ',');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].damage_aim, 3, ',');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].damage, ',');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].damage_range, ',');
				/*
				 * Ignored fields:
					* spread x
					* spread z
					* speed
					* distance
					* X weapon2Offset
					* Y weapon2Offset
					* Z weapon2Offset
					* base damage
					* random damage
					* spread x
					* spread z
					* speed
					* distance
					* X weapon3Offset
					* Y weapon3Offset
					* Z weapon3Offset
					* base damage
					* random damage
					* spread x
					* spread z
					* speed
					* distance
					* min attenuation
					* max attenuation
				 */

				/* Fix path */
				Q_replacebackslash(dynamicentities[curr_pos].model_path);

				/* go to next row */
				curr_pos ++;
			}

			curr += linesize;
			if (curr >= (buf_ai + len_ai))
			{
				break;
			}

			/* skip our endline */
			curr++;
		}
		free(buf_ai);
	}

	/* load definitons count */
	if (buf_ent)
	{
		char *curr;

		/* get lines count */
		curr = buf_ent;
		while (*curr)
		{
			size_t linesize = 0;

			if (curr_pos >= ndynamicentities)
			{
				break;
			}

			/* skip empty */
			linesize = strspn(curr, "\n\r\t ");
			curr += linesize;

			/* mark end line */
			linesize = strcspn(curr, "\n\r");
			curr[linesize] = 0;

			if (*curr && strncmp(curr, "//", 2) &&
				*curr != '\n' && *curr != '\r' && *curr != ';')
			{
				char *line;
				char gib_type[MAX_QPATH] = {0};

				line = curr;
				line = DynamicStringParse(line, dynamicentities[curr_pos].classname, MAX_QPATH, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].model_path,
					sizeof(dynamicentities[curr_pos].model_path), '|');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].scale, 3, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].entity_type, MAX_QPATH, '|');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].mins, 3, '|');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].maxs, 3, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].noshadow, MAX_QPATH, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].solidflag, '|');
				line = DynamicFloatParse(line, &dynamicentities[curr_pos].walk_speed, 1, '|');
				line = DynamicFloatParse(line, &dynamicentities[curr_pos].run_speed, 1, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].speed, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].lighting, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].blending, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].target_sequence, MAX_QPATH, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].misc_value, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].no_mip, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].spawn_sequence, MAX_QPATH, '|');
				line = DynamicStringParse(line, dynamicentities[curr_pos].description, MAX_QPATH, '|');
				/* Additional field for cover for color from QUAKED */
				line = DynamicFloatParse(line, dynamicentities[curr_pos].color, 3, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].health, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].mass, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].damage, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].damage_range, '|');
				line = DynamicFloatParse(line, dynamicentities[curr_pos].damage_aim, 3, '|');
				line = DynamicStringParse(line, gib_type, MAX_QPATH, '|');
				line = DynamicIntParse(line, &dynamicentities[curr_pos].gib_health, '|');

				dynamicentities[curr_pos].gib = DynamicSpawnGibFromName(gib_type);

				/* Fix path */
				Q_replacebackslash(dynamicentities[curr_pos].model_path);

				/* go to next row */
				curr_pos ++;
			}
			curr += linesize;
			if (curr >= (buf_ent + len_ent))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}

		free(buf_ent);
	}

	/* save last used position */
	ndynamicentities = curr_pos;

	if (!curr_pos)
	{
		return;
	}

	gi.dprintf("Found %d dynamic definitions\n", ndynamicentities);

	/* sort definitions */
	qsort(dynamicentities, ndynamicentities, sizeof(dynamicentity_t), DynamicSort);
}

gitem_t *
GetDynamicItems(int *count)
{
	size_t i, itemcount;
	gitem_t *items;

	*count = 0;

	if (!ndynamicentities || !dynamicentities)
	{
		return NULL;
	}

	itemcount = 0;

	for (i = 0; i < ndynamicentities; i++)
	{
		if (!strncmp(dynamicentities[i].classname, "item_", 5) ||
			!strncmp(dynamicentities[i].classname, "weapon_", 7) ||
			!strncmp(dynamicentities[i].classname, "key_", 4) ||
			!strncmp(dynamicentities[i].classname, "ammo_", 5))
		{
			itemcount++;
		}
	}

	if (!itemcount)
	{
		return NULL;
	}

	/* allocate */
	items = calloc(itemcount, sizeof(*items));
	if (!items)
	{
		gi.error("Can't allocate dynamic %d items\n", itemcount);
		return NULL;
	}

	itemcount = 0;

	for (i = 0; i < ndynamicentities; i++)
	{
		if (strncmp(dynamicentities[i].classname, "item_", 5) &&
			strncmp(dynamicentities[i].classname, "weapon_", 7) &&
			strncmp(dynamicentities[i].classname, "key_", 4) &&
			strncmp(dynamicentities[i].classname, "ammo_", 5))
		{
			/* looks as not item */
			continue;
		}

		if (!dynamicentities[i].model_path[0])
		{
			/* skip without model */
			continue;
		}

		if (StaticSpawnSearch(dynamicentities[i].classname))
		{
			/* has static spawn function */
			continue;
		}

		/* Could be dynamic item */
		items[itemcount].classname = dynamicentities[i].classname;
		items[itemcount].world_model = dynamicentities[i].model_path;
		items[itemcount].pickup_name = dynamicentities[i].description;
		itemcount++;
	}

	*count = itemcount;
	return items;
}

static int
StaticSort(const void *p1, const void *p2)
{
	spawn_t *ent1, *ent2;

	ent1 = (spawn_t*)p1;
	ent2 = (spawn_t*)p2;
	return Q_stricmp(ent1->name, ent2->name);
}

static void
StaticSpawnInit(void)
{
	const spawn_t *s;

	/* check count of spawn functions */
	for (s = spawns; s->name; s++)
	{
	}

	nstaticentities = s - spawns;

	gi.dprintf("Found %d static definitions\n", nstaticentities);

	/* sort definitions */
	qsort(spawns, nstaticentities, sizeof(spawn_t), StaticSort);
}

void
SpawnInit(void)
{
	StaticSpawnInit();
	DynamicSpawnInit();
}

void
SpawnFree(void)
{
	if (dynamicentities || ndynamicentities)
	{
		gi.dprintf("Free %d dynamic definitions\n", ndynamicentities);
		free(dynamicentities);
	}

	dynamicentities = NULL;
	ndynamicentities = 0;
}

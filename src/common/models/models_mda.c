/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) 1998 Trey Harrison (SiN View)
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
 * The MDA models file format
 *
 * =======================================================================
 */

#include "models.h"

typedef struct {
	char *map;
	char *alphafunc;
	char *depthwrite;
	char *uvgen;
	char *blendmode;
	char *depthfunc;
	char *cull;
	char *rgbgen;
	char *uvmod;
} mda_pass_t;

typedef struct {
	mda_pass_t *passes;
	size_t pass_count;
} mda_skin_t;

typedef struct {
	char *name;
	char *evaluate;
	mda_skin_t *skins;
	size_t skin_count;
} mda_profile_t;

typedef struct {
	char *basemodel;
	vec3_t headtri;
	mda_profile_t *profiles;
	size_t profile_count;
} mda_model_t;

static void
Mod_LoadModel_MDA_Parse_SkipComment(char **curr_buff)
{
	size_t linesize;

	/* skip comment */
	linesize = strcspn(*curr_buff, "\n\r");
	*curr_buff += linesize;
}

static void
Mod_LoadModel_MDA_Parse_Pass(const char *mod_name, char **curr_buff, char *curr_end,
	mda_pass_t *pass)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "map"))
		{
			token = COM_Parse(curr_buff);
			pass->map = strdup(token);
			Q_replacebackslash(pass->map);
		}
		else if (!strcmp(token, "uvmod"))
		{
			size_t linesize;
			char *value;

			linesize = strcspn(*curr_buff, "\n\r");
			value = malloc(linesize + 1);
			memcpy(value, *curr_buff, linesize);
			value[linesize] = 0;
			*curr_buff += linesize;

			pass->uvmod = value;
		}
		else if (!strcmp(token, "alphafunc") ||
				 !strcmp(token, "depthwrite") ||
				 !strcmp(token, "uvgen") ||
				 !strcmp(token, "blendmode") ||
				 !strcmp(token, "depthfunc") ||
				 !strcmp(token, "cull") ||
				 !strcmp(token, "rgbgen"))
		{
			char token_section[MAX_TOKEN_CHARS];

			Q_strlcpy(token_section, token, sizeof(token_section));
			token = COM_Parse(curr_buff);

			if (!strcmp(token_section, "alphafunc"))
			{
				pass->alphafunc = strdup(token);
			}
			else if (!strcmp(token_section, "depthwrite"))
			{
				pass->depthwrite = strdup(token);
			}
			else if (!strcmp(token_section, "uvgen"))
			{
				pass->uvgen = strdup(token);
			}
			else if (!strcmp(token_section, "blendmode"))
			{
				pass->blendmode = strdup(token);
			}
			else if (!strcmp(token_section, "depthfunc"))
			{
				pass->depthfunc = strdup(token);
			}
			else if (!strcmp(token_section, "cull"))
			{
				pass->cull = strdup(token);
			}
			else if (!strcmp(token_section, "rgbgen"))
			{
				pass->rgbgen = strdup(token);
			}
		}
	}
}

static void
Mod_LoadModel_MDA_Parse_Skin(const char *mod_name, char **curr_buff, char *curr_end,
	mda_skin_t *skin)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "pass"))
		{
			mda_pass_t *pass;

			skin->pass_count++;
			skin->passes = realloc(skin->passes, skin->pass_count * sizeof(mda_pass_t));
			pass = &skin->passes[skin->pass_count - 1];
			memset(pass, 0, sizeof(*pass));

			token = COM_Parse(curr_buff);
			if (!token || token[0] != '{')
			{
				return;
			}

			Mod_LoadModel_MDA_Parse_Pass(mod_name, curr_buff, curr_end, pass);
		}
	}
}

static void
Mod_LoadModel_MDA_Parse_Profile(const char *mod_name, char **curr_buff, char *curr_end,
	mda_profile_t *profile)
{
	while (*curr_buff && *curr_buff < curr_end)
	{
		const char *token;

		token = COM_Parse(curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			break;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(curr_buff);
		}
		else if (!strcmp(token, "skin")) {
			mda_skin_t *skin;

			profile->skin_count++;
			profile->skins = realloc(profile->skins, profile->skin_count * sizeof(mda_skin_t));
			skin = &profile->skins[profile->skin_count - 1];
			memset(skin, 0, sizeof(*skin));

			token = COM_Parse(curr_buff);
			if (!token || token[0] != '{')
			{
				return;
			}

			Mod_LoadModel_MDA_Parse_Skin(mod_name, curr_buff, curr_end, skin);
		}
		else if (!strcmp(token, "evaluate"))
		{
			profile->evaluate = strdup(COM_Parse(curr_buff));
		}
	}
}

static void
Mod_LoadModel_MDA_Parse(const char *mod_name, char *curr_buff, char *curr_end,
	mda_model_t *mda)
{
	while (curr_buff)
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!*token)
		{
			continue;
		}

		else if (token[0] == '}')
		{
			/* skip end of section */
			continue;
		}
		else if (token[0] == '#')
		{
			Mod_LoadModel_MDA_Parse_SkipComment(&curr_buff);

		}

		/* found basemodel */
		else if (!strcmp(token, "basemodel"))
		{
			token = COM_Parse(&curr_buff);
			if (!token)
			{
				return;
			}
			mda->basemodel = strdup(token);
			Q_replacebackslash(mda->basemodel);
		}
		else if (!strcmp(token, "headtri"))
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				token = COM_Parse(&curr_buff);
				mda->headtri[i] = (float)strtod(token, (char **)NULL);
			}
		}
		else if (!strcmp(token, "profile"))
		{
			mda_profile_t *profile;

			token = COM_Parse(&curr_buff);
			mda->profile_count++;
			mda->profiles = realloc(mda->profiles, mda->profile_count * sizeof(mda_profile_t));
			profile = &mda->profiles[mda->profile_count - 1];
			memset(profile, 0, sizeof(*profile));

			if (!token || token[0] == '{')
			{
				profile->name = strdup("");
			}
			else
			{
				profile->name = strdup(token);
				token = COM_Parse(&curr_buff);
				if (!token || token[0] != '{')
				{
					return;
				}
			}

			Mod_LoadModel_MDA_Parse_Profile(mod_name, &curr_buff, curr_end, profile);
		}
	}
}

static void
Mod_LoadModel_MDA_Free(mda_model_t *mda)
{
	size_t i;

	free(mda->basemodel);
	for (i = 0; i < mda->profile_count; i++) {
		mda_profile_t *profile;
		size_t j;

		profile = &mda->profiles[i];
		free(profile->name);
		free(profile->evaluate);
		for (j = 0; j < profile->skin_count; j++)
		{
			mda_skin_t *skin;
			size_t k;

			skin = &profile->skins[j];
			for (k = 0; k < skin->pass_count; k++)
			{
				mda_pass_t *pass;

				pass = &skin->passes[k];
				free(pass->map);
				free(pass->alphafunc);
				free(pass->depthwrite);
				free(pass->uvgen);
				free(pass->blendmode);
				free(pass->depthfunc);
				free(pass->cull);
				free(pass->rgbgen);
				free(pass->uvmod);
			}
			free(skin->passes);
		}
		free(profile->skins);
	}
	free(mda->profiles);
}

static void *
Mod_LoadModel_MDA_Text(const char *mod_name, char *curr_buff, size_t len)
{
	mda_model_t mda = {0};
	const char *profile_name;

	profile_name = strrchr(mod_name, '!');
	if (profile_name)
	{
		/* skip ! */
		profile_name += 1;
	}
	else
	{
		profile_name = "";
	}

	Mod_LoadModel_MDA_Parse(mod_name, curr_buff, curr_buff + len, &mda);

	if (mda.basemodel)
	{
		void *extradata, *base;
		int base_size;

		base_size = FS_LoadFile(mda.basemodel, (void **)&base);
		if (base_size <= 0)
		{
			Com_DPrintf("%s: %s No base model for %s\n",
				__func__, mod_name, mda.basemodel);
			Mod_LoadModel_MDA_Free(&mda);
			return NULL;
		}

		/* little bit recursive load */
		extradata = Mod_LoadModelFile(mod_name, base, base_size);
		FS_FreeFile(base);

		/* check skin path */
		if (extradata && mda.profile_count &&
			(((dmdx_t *)extradata)->ident == IDALIASHEADER))
		{
			mda_profile_t *profile = NULL;

			if (profile_name)
			{
				size_t i;

				for (i = 0; i < mda.profile_count; i++)
				{
					if (!strcmp(mda.profiles[i].name, profile_name))
					{
						profile = &mda.profiles[i];
					}
				}
			}

			if (!profile)
			{
				/* use first in list */
				profile = &mda.profiles[0];
			}

			if (profile)
			{
				size_t skins_count, i;
				dmdx_t *pheader;

				pheader = (dmdx_t *)extradata;
				skins_count = Q_min(profile->skin_count, pheader->num_skins);

				for (i = 0; i < skins_count; i++)
				{
					if (profile->skins[i].pass_count)
					{
						char *skin;

						/* Update included model with skin path */
						skin = (char *)pheader + pheader->ofs_skins + i * MAX_SKINNAME;
						if (!strchr(skin, '/') && !strchr(skin, '\\'))
						{
							char skin_path[MAX_QPATH * 2] = {0};
							char *base_skin;

							base_skin = profile->skins[i].passes[0].map;
							Q_strlcpy(skin_path, base_skin, sizeof(skin_path));
							strcpy(strrchr(skin_path, '/') + 1, skin);
							Q_strlcpy(skin, skin_path, MAX_SKINNAME);
						}
					}
				}
			}
		}

		Mod_LoadModel_MDA_Free(&mda);

		return extradata;
	}

	Mod_LoadModel_MDA_Free(&mda);
	return NULL;
}

void *
Mod_LoadModel_MDA(const char *mod_name, const void *buffer, int modfilelen)
{
	void *extradata;
	char *text;

	text = malloc(modfilelen + 1 - 4);
	memcpy(text, (char *)buffer + 4, modfilelen - 4);
	text[modfilelen - 4] = 0;

	extradata = Mod_LoadModel_MDA_Text(mod_name, text, modfilelen - 4);

	free(text);

	return extradata;
}

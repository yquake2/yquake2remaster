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
 * Localization logic.
 *
 * =======================================================================
 */

#include "header/server.h"

typedef struct
{
	char *key;
	char *value;
	char *sound;
} localmessages_t;

static localmessages_t *localmessages = NULL;
static int nlocalmessages = 0;

static int
LocalizationSort(const void *p1, const void *p2)
{
	const localmessages_t *msg1, *msg2;

	msg1 = (localmessages_t*)p1;
	msg2 = (localmessages_t*)p2;
	return Q_stricmp(msg1->key, msg2->key);
}

static char *
LocalizationFileRead(const char *filename, int *len)
{
	byte *raw = NULL;
	char *buf = NULL;

	*len = FS_LoadFile(filename, (void **)&raw);
	if (*len > 1)
	{
		buf = malloc(*len + 1);
		if (!buf)
		{
			Com_Error(ERR_DROP, "%s: can't allocate space for file\n",
				__func__);
			return NULL;
		}

		memcpy(buf, raw, *len);
		buf[*len] = 0;
		FS_FreeFile(raw);
	}

	return buf;
}

void
SV_LocalizationInit(void)
{
	localmessages = NULL;
	nlocalmessages = 0;
}

/* Lookup table for Windows-1252 to Unicode code points (only special range 0x80–0x9F) */
static const unsigned short win1252_table[32] =
{
	0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
	0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
	0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178
};

/* Encode one Unicode code point to UTF-8 */
static size_t
LocalizationEncodeUTF8(unsigned cp, char *out)
{
	if (cp < 0x80)
	{
		out[0] = cp;
		return 1;
	}
	else if (cp < 0x800)
	{
		out[0] = 0xC0 | (cp >> 6);
		out[1] = 0x80 | (cp & 0x3F);
		return 2;
	}
	else if (cp < 0x10000)
	{
		out[0] = 0xE0 | (cp >> 12);
		out[1] = 0x80 | ((cp >> 6) & 0x3F);
		out[2] = 0x80 | (cp & 0x3F);
		return 3;
	}
	else
	{
		out[0] = 0xF0 | (cp >> 18);
		out[1] = 0x80 | ((cp >> 12) & 0x3F);
		out[2] = 0x80 | ((cp >> 6) & 0x3F);
		out[3] = 0x80 | (cp & 0x3F);
		return 4;
	}
}

/* Convert Windows-1252 string to UTF-8 */
static char *
LocalizationConvertWIN1252ToUTF8(char *in_buf)
{
	char *out, *buffer;
	const byte *in;

	buffer = out = malloc(strlen(in_buf) * 4 + 1);
	if (!out)
	{
		Com_DPrintf("Can't alloc translated text\n");
		return in_buf;
	}

	in = (const byte*)in_buf;
	while (*in)
	{
		unsigned cp;

		if ((*in < 0x80) || (*in >= 0xA0))
		{
			/* ASCII and normal Latin-1 range */
			cp = *in;
		}
		else
		{
			/* special mapping */
			cp = win1252_table[*in - 0x80];
		}

		out += LocalizationEncodeUTF8(cp, out);
		in++;
	}

	/* free input buffer */
	free(in_buf);

	*out = '\0';

	out = buffer;
	out = realloc(buffer, strlen(buffer) + 1);
	if (!out)
	{
		Com_DPrintf("Can't realloc translated text\n");
		return buffer;
	}

	return out;
}


static void
SV_LocalizationReload(void)
{
	char *buf_local = NULL, *buf_level = NULL, *buf_strings = NULL;
	int len_local, len_level, len_strings, curr_pos;
	char loc_name[MAX_QPATH];

	if (localmessages)
	{
		return;
	}

	nlocalmessages = 0;

	/* load the localization file */
	snprintf(loc_name, sizeof(loc_name) - 1, "localization/loc_%s.txt", sv_language->string);
	buf_local = LocalizationFileRead(loc_name, &len_local);

	/* load the heretic 2 messages file */
	buf_level = LocalizationFileRead("levelmsg.txt", &len_level);
	if (buf_level)
	{
		buf_level = LocalizationConvertWIN1252ToUTF8(buf_level);
	}

	/* load the hexen 2 messages file */
	buf_strings = LocalizationFileRead("Strings.txt", &len_strings);

	/* localization lines count */
	if (buf_local)
	{
		char *curr;

		/* get lines count */
		curr = buf_local;
		while (*curr)
		{
			size_t linesize = 0;

			linesize = strcspn(curr, "\n\r");
			if (*curr && strncmp(curr, "//", 2) &&
				*curr != '\n' && *curr != '\r')
			{
				nlocalmessages ++;
			}
			curr += linesize;
			if (curr >= (buf_local + len_local))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* heretic 2 lines count */
	if (buf_level)
	{
		char *curr;

		/* get lines count */
		curr = buf_level;
		while (*curr)
		{
			size_t linesize = 0;

			linesize = strcspn(curr, "\n");
			/* skip lines with both endline codes */
			if (*curr && *curr != ';')
			{
				nlocalmessages ++;
			}
			curr += linesize;
			if (curr >= (buf_level + len_level))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* hexen 2 lines count */
	if (buf_strings)
	{
		char *curr;

		/* get lines count */
		curr = buf_strings;
		while (*curr)
		{
			size_t linesize = 0;

			linesize = strcspn(curr, "\n");
			/* skip lines with both endline codes */
			if (*curr)
			{
				nlocalmessages ++;
			}
			curr += linesize;
			if (curr >= (buf_strings + len_strings))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	if (nlocalmessages)
	{
		localmessages = calloc((nlocalmessages + 1), sizeof(*localmessages));
		if (!localmessages)
		{
			Com_Error(ERR_DROP, "%s: can't allocate messages\n",
				__func__);
			return;
		}
	}

	curr_pos = 0;

	/* localization load */
	if (buf_local && localmessages)
	{
		char *curr;

		/* parse lines */
		curr = buf_local;
		while (*curr)
		{
			size_t linesize = 0;

			if (curr_pos >= nlocalmessages)
			{
				break;
			}

			linesize = strcspn(curr, "\n\r");
			curr[linesize] = 0;
			if (*curr && strncmp(curr, "//", 2) &&
				*curr != '\n' && *curr != '\r')
			{
				char *sign;

				sign = strchr(curr, '=');
				/* clean up end of key */
				if (sign)
				{
					char *signend;

					signend = sign - 1;
					while ((*signend == ' ' || *signend == '\t') &&
						   (signend > curr))
					{
						*signend = 0;
						/* go back */
						signend --;
					}
					*sign = 0;
					sign ++;
				}

				/* skip value prefix */
				if (sign && *sign)
				{
					size_t valueskip;

					valueskip = strcspn(sign, " \t");
					sign += valueskip;
					if (*sign)
					{
						sign++;
					}
				}

				/* start real value */
				if (sign && *sign == '"')
				{
					char *currend;

					sign ++;

					currend = sign;
					while (*currend && *currend != '"')
					{
						if (*currend == '\\')
						{
							/* escaped */
							*currend = ' ';
							currend++;
							if (*currend == 'n')
							{
								/* new line */
								*currend = '\n';
							}
						}
						currend++;
					}

					if (*currend == '"')
					{
						/* mark as end of string */
						*currend = 0;
					}

					localmessages[curr_pos].key = malloc(strlen(curr) + 2);
					if (!localmessages[curr_pos].key)
					{
						Com_Error(ERR_DROP, "%s: can't allocate messages key\n",
							__func__);
						break;
					}
					localmessages[curr_pos].key[0] = '$';
					strcpy(localmessages[curr_pos].key + 1, curr);

					localmessages[curr_pos].value = malloc(strlen(sign) + 1);
					if (!localmessages[curr_pos].value)
					{
						Com_Error(ERR_DROP, "%s: can't allocate messages value\n",
							__func__);
						break;
					}
					strcpy(localmessages[curr_pos].value, sign);

					/* ReRelease does not have merged sound files to message */
					localmessages[curr_pos].sound = NULL;
					curr_pos ++;
				}
			}
			curr += linesize;
			if (curr >= (buf_local + len_local))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* heretic 2 translate load */
	if (buf_level && localmessages)
	{
		char *curr;
		int i;

		curr = buf_level;
		i = 1;
		while (*curr)
		{
			char *sign, *currend;
			size_t linesize = 0;

			if (curr_pos >= nlocalmessages)
			{
				break;
			}

			linesize = strcspn(curr, "\n");
			curr[linesize] = 0;
			if (curr[0] != ';')
			{
				/* remove caret back */
				if (curr[linesize - 1] == '\r')
				{
					curr[linesize - 1] = 0;
				}

				sign = strchr(curr, '#');
				/* clean up end of key */
				if (sign)
				{
					*sign = 0;
					sign ++;

					/* replace '\' with '/' in sound path */
					currend = sign;
					while (*currend)
					{
						if (*currend == '\\')
						{
							*currend = '/';
						}

						currend++;
					}
				}

				/* replace @ in message with new line */
				currend = curr;
				while (*currend)
				{
					if (*currend == '@')
					{
						*currend = '\n';
					}

					currend++;
				}

				localmessages[curr_pos].key = malloc(6);
				if (!localmessages[curr_pos].key)
				{
					Com_Error(ERR_DROP, "%s: can't allocate messages key\n",
						__func__);
					break;
				}

				snprintf(localmessages[curr_pos].key, 5, "%d", i);
				localmessages[curr_pos].value = malloc(strlen(curr) + 1);
				if (!localmessages[curr_pos].value)
				{
					Com_Error(ERR_DROP, "%s: can't allocate messages value\n",
						__func__);
					break;
				}
				strcpy(localmessages[curr_pos].value, curr);
				/* Some Heretic message could have no sound effects */
				localmessages[curr_pos].sound = NULL;
				if (sign)
				{
					/* has some sound aligned with message */
					localmessages[curr_pos].sound = malloc(strlen(sign) + 1);
					if (!localmessages[curr_pos].sound)
					{
						Com_Error(ERR_DROP, "%s: can't allocate messages sound\n",
							__func__);
						break;
					}
					strcpy(localmessages[curr_pos].sound, sign);
				}

				curr_pos ++;
			}
			i ++;

			curr += linesize;
			if (curr >= (buf_level + len_level))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* hexen 2 translate load */
	if (buf_strings && localmessages)
	{
		char *curr;
		int i;

		curr = buf_strings;
		i = 1;
		while (*curr)
		{
			char *currend;
			size_t linesize = 0;

			if (curr_pos >= nlocalmessages)
			{
				break;
			}

			linesize = strcspn(curr, "\n");
			curr[linesize] = 0;

			/* remove caret back */
			if (curr[linesize - 1] == '\r')
			{
				curr[linesize - 1] = 0;
			}

			/* replace @ in message with new line */
			currend = curr;
			while (*currend)
			{
				if (*currend == '@')
				{
					*currend = '\n';
				}

				currend++;
			}

			localmessages[curr_pos].key = malloc(6);
			if (!localmessages[curr_pos].key)
			{
				Com_Error(ERR_DROP, "%s: can't allocate messages key\n",
					__func__);
				break;
			}
			snprintf(localmessages[curr_pos].key, 5, "%d", i);

			localmessages[curr_pos].value = malloc(strlen(curr) + 1);
			if (!localmessages[curr_pos].value)
			{
				Com_Error(ERR_DROP, "%s: can't allocate messages value\n",
					__func__);
				break;
			}
			strcpy(localmessages[curr_pos].value, curr);
			/* Some Heretic message could have no sound effects */
			localmessages[curr_pos].sound = NULL;

			curr_pos ++;
			i ++;

			curr += linesize;
			if (curr >= (buf_strings + len_strings))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	/* save last used position */
	nlocalmessages = curr_pos;
	free(buf_strings);
	free(buf_level);
	free(buf_local);

	if (!curr_pos && !localmessages)
	{
		localmessages = calloc(1, sizeof(*localmessages));
		if (!localmessages)
		{
			Com_Error(ERR_DROP, "%s: can't allocate messages\n",
				__func__);
			return;
		}
		return;
	}

	Com_Printf("Found %d translated lines\n", nlocalmessages);

	/* sort messages */
	qsort(localmessages, nlocalmessages, sizeof(localmessages_t), LocalizationSort);
}

void
SV_LocalizationFree(void)
{
	if (localmessages)
	{
		int i;

		for (i = 0; i < nlocalmessages; i++)
		{
			if (localmessages[i].key)
			{
				free(localmessages[i].key);
			}

			if (localmessages[i].value)
			{
				free(localmessages[i].value);
			}

			if (localmessages[i].sound)
			{
				free(localmessages[i].sound);
			}
		}
		free(localmessages);

		Com_Printf("Free %d translated lines\n", nlocalmessages);
	}
	localmessages = NULL;
	nlocalmessages = 0;
}

static int
LocalizationSearch(const char *name)
{
	int start, end;

	start = 0;
	end = nlocalmessages - 1;

	while (start <= end)
	{
		int i, res;

		i = start + (end - start) / 2;

		res = Q_stricmp(localmessages[i].key, name);
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

const char *
SV_LocalizationUIMessage(const char *message, const char *default_message)
{
	SV_LocalizationReload();

	if (!message || !localmessages || !nlocalmessages)
	{
		return default_message;
	}

	if ((message[0] == '$') || /* ReRelease */
		(strspn(message, "1234567890") == strlen(message))) /* Hexen 2 / Heretic 2 */
	{
		int i;

		i = LocalizationSearch(message);
		if (i >= 0)
		{
			return localmessages[i].value;
		}
	}

	return default_message;
}

const char*
SV_LocalizationMessage(const char *message, const char **sound)
{
	SV_LocalizationReload();

	if (!message || !localmessages || !nlocalmessages)
	{
		return message;
	}

	if ((message[0] == '$') || /* ReRelease */
		(strspn(message, "1234567890") == strlen(message))) /* Hexen 2 / Heretic 2 */
	{
		int i;

		i = LocalizationSearch(message);
		if (i >= 0)
		{
			if (sound && localmessages[i].sound)
			{
				*sound = localmessages[i].sound;
			}

			return localmessages[i].value;
		}
	}

	return message;
}

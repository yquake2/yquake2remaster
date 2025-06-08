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

#include "header/local.h"

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
	localmessages_t *msg1, *msg2;

	msg1 = (localmessages_t*)p1;
	msg2 = (localmessages_t*)p2;
	return Q_stricmp(msg1->key, msg2->key);
}

void
LocalizationInit(void)
{
	byte *raw = NULL;
	char *buf_local = NULL, *buf_level = NULL;
	int len_local, len_level, curr_pos;
	char loc_name[MAX_QPATH];

	localmessages = NULL;
	nlocalmessages = 0;

	snprintf(loc_name, sizeof(loc_name) - 1, "localization/loc_%s.txt", g_language->string);
	/* load the localization file */
	len_local = gi.LoadFile(loc_name, (void **)&raw);
	if (len_local > 1)
	{
		buf_local = malloc(len_local + 1);
		memcpy(buf_local, raw, len_local);
		buf_local[len_local] = 0;
		gi.FreeFile(raw);
	}

	/* load the heretic 2 messages file */
	len_level = gi.LoadFile("levelmsg.txt", (void **)&raw);
	if (len_level > 1)
	{
		buf_level = malloc(len_level + 1);
		memcpy(buf_level, raw, len_level);
		buf_level[len_level] = 0;
		gi.FreeFile(raw);
	}

	/* localization lines count */
	if (buf_local)
	{
		char *curr;

		/* get lines count */
		curr = buf_local;
		while(*curr)
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
		while(*curr)
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

	if (nlocalmessages)
	{
		localmessages = malloc(nlocalmessages * sizeof(*localmessages));
		memset(localmessages, 0, nlocalmessages * sizeof(*localmessages));
	}

	curr_pos = 0;

	/* localization load */
	if (buf_local)
	{
		char *curr;

		/* parse lines */
		curr = buf_local;
		while(*curr)
		{
			size_t linesize = 0;

			if (curr_pos == nlocalmessages)
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
					localmessages[curr_pos].key[0] = '$';
					strcpy(localmessages[curr_pos].key + 1, curr);
					localmessages[curr_pos].value = malloc(strlen(sign) + 1);
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
		free(buf_local);
	}

	/* heretic 2 translate load */
	if (buf_level)
	{
		char *curr;
		int i;

		curr = buf_level;
		i = 1;
		while(*curr)
		{
			char *sign, *currend;
			size_t linesize = 0;

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
					while(*currend)
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
				while(*currend)
				{
					if (*currend == '@')
					{
						*currend = '\n';
					}

					currend++;
				}

				localmessages[curr_pos].key = malloc(6);
				snprintf(localmessages[curr_pos].key, 5, "%d", i);
				localmessages[curr_pos].value = malloc(strlen(curr) + 1);
				strcpy(localmessages[curr_pos].value, curr);
				/* Some Heretic message could have no sound effects */
				localmessages[curr_pos].sound = NULL;
				if (sign)
				{
					/* has some sound aligned with message */
					localmessages[curr_pos].sound = malloc(strlen(sign) + 1);
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

		free(buf_level);
	}

	/* save last used position */
	nlocalmessages = curr_pos;

	if (!curr_pos)
	{
		return;
	}

	gi.dprintf("Found %d translated lines\n", nlocalmessages);

	/* sort messages */
	qsort(localmessages, nlocalmessages, sizeof(localmessages_t), LocalizationSort);
}

void
LocalizationFree(void)
{
	if (localmessages && nlocalmessages)
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

		gi.dprintf("Free %d translated lines\n", nlocalmessages);
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

const char*
LocalizationMessage(const char *message, int *sound_index)
{
	if (!message || !localmessages || !nlocalmessages)
	{
		return message;
	}

	if ((message[0] == '$') || /* ReRelease */
		(strspn(message, "1234567890") == strlen(message))) /* Heretic 2 */
	{
		int i;

		i = LocalizationSearch(message);
		if (i >= 0)
		{
			if (sound_index && localmessages[i].sound)
			{
				*sound_index = gi.soundindex(localmessages[i].sound);
			}

			return localmessages[i].value;
		}
	}

	return message;
}

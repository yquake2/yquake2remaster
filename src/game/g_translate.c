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
	char *buf_loc = NULL;
	int len, curr_pos;

	localmessages = NULL;
	nlocalmessages = 0;

	/* load the file */
	len = gi.FS_LoadFile("localization/loc_english.txt", (void **)&raw);
	if (len > 1)
	{
		buf_loc = malloc(len + 1);
		memcpy(buf_loc, raw, len);
		buf_loc[len] = 0;
		gi.FS_FreeFile(raw);
	}

	/* localization lines count */
	if (buf_loc)
	{
		char *curr;

		/* get lines count */
		curr = buf_loc;
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
			if (curr >= (buf_loc + len))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
	}

	if (nlocalmessages)
	{
		localmessages = gi.TagMalloc(nlocalmessages * sizeof(*localmessages), TAG_GAME);
		memset(localmessages, 0, nlocalmessages * sizeof(*localmessages));
	}

	curr_pos = 0;

	/* localization load */
	if (buf_loc)
	{
		char *curr;

		/* parse lines */
		curr = buf_loc;
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

					localmessages[curr_pos].key = gi.TagMalloc(strlen(curr) + 2, TAG_GAME);
					localmessages[curr_pos].key[0] = '$';
					strcpy(localmessages[curr_pos].key + 1, curr);
					localmessages[curr_pos].value = gi.TagMalloc(strlen(sign) + 1, TAG_GAME);
					strcpy(localmessages[curr_pos].value, sign);
					/* ReRelease does not have merged sound files to message */
					localmessages[curr_pos].sound = NULL;
					curr_pos ++;
				}
			}
			curr += linesize;
			if (curr >= (buf_loc + len))
			{
				break;
			}
			/* skip our endline */
			curr++;
		}
		free(buf_loc);
	}

	/* save last used position */
	nlocalmessages = curr_pos;

	if (!curr_pos)
	{
		return;
	}

	/* sort messages */
	qsort(localmessages, nlocalmessages, sizeof(localmessages_t), LocalizationSort);
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
LocalizationMessage(const char *message)
{
	if (!message || !localmessages || !nlocalmessages)
	{
		return message;
	}

	if (message[0] == '$')
	{
		int i;

		i = LocalizationSearch(message);
		if (i >= 0)
		{
			return localmessages[i].value;
		}
	}

	return message;
}

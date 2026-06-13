#!/bin/bash
if [[ "$1" == "" ]]; then
	GAMESRCDIR=.
else
	GAMESRCDIR=$1
fi

echo "/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2011 Knightmare
 * Copyright (C) 2011 Yamagi Burmeister
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
 * Prototypes for every mmove_t in the game.so.
 *
 * =======================================================================
 */"

grep -rhoP 'mmove_t\s+\K[a-zA-Z0-9_]+(?=\s*=\s*{)' "$GAMESRCDIR" | sort -u | grep -v "_static" | \
	gawk '!seen[$0]++ { print gensub(/([_a-zA-Z0-9]+)/, "extern mmove_t \\1;\n", "g") }' | sort -u

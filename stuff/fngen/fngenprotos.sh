#!/bin/bash
echo "/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2011 Yamagi Burmeister
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
 * Prototypes for every function in the game.so.
 *
 * =======================================================================
 */
"

HERE=$(dirname "$0")

$HERE/fnproto.sh "$1" die void "(edict_t *, edict_t *, edict_t *, int, const vec3_t)"
$HERE/fnproto.sh "$1" pain void "(edict_t *, edict_t *, float, int)"
$HERE/fnproto.sh "$1" prethink void "(edict_t *)"
$HERE/fnproto.sh "$1" think void "(edict_t *)"
$HERE/fnproto.sh "$1" touch void "(edict_t *, edict_t *, const cplane_t *, const csurface_t *)"
$HERE/fnproto.sh "$1" use void "(edict_t *, edict_t *, edict_t *)"
$HERE/fnproto.sh "$1" blocked void "(edict_t *, edict_t *)"

$HERE/fnproto.sh "$1" monsterinfo.stand void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.walk void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.run void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.melee void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.attack void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.checkattack qboolean "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.dodge void "(edict_t *, edict_t *, float, trace_t *trace)"
$HERE/fnproto.sh "$1" monsterinfo.idle void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.search void "(edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.sight void "(edict_t *, edict_t *)"
$HERE/fnproto.sh "$1" monsterinfo.blocked qboolean "(edict_t *self, float dist)"
$HERE/fnproto.sh "$1" monsterinfo.duck void "(edict_t *self, float eta)"
$HERE/fnproto.sh "$1" monsterinfo.unduck void "(edict_t *self)"
$HERE/fnproto.sh "$1" monsterinfo.sidestep void "(edict_t *self)"

$HERE/fnproto.sh "$1" moveinfo.endfunc void "(edict_t *)"

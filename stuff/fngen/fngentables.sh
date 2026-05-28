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
 * Functionpointers to every function in the game.so.
 *
 * =======================================================================
 */
"

HERE=$(dirname "$0")

$HERE/fntable.sh "$1" die
$HERE/fntable.sh "$1" pain
$HERE/fntable.sh "$1" prethink
$HERE/fntable.sh "$1" think
$HERE/fntable.sh "$1" touch
$HERE/fntable.sh "$1" use
$HERE/fntable.sh "$1" blocked

$HERE/fntable.sh "$1" monsterinfo.stand mi_stand
$HERE/fntable.sh "$1" monsterinfo.walk mi_walk
$HERE/fntable.sh "$1" monsterinfo.run mi_run
$HERE/fntable.sh "$1" monsterinfo.melee mi_melee
$HERE/fntable.sh "$1" monsterinfo.attack mi_attack
$HERE/fntable.sh "$1" monsterinfo.checkattack mi_checkattack
$HERE/fntable.sh "$1" monsterinfo.dodge mi_dodge
$HERE/fntable.sh "$1" monsterinfo.idle mi_idle
$HERE/fntable.sh "$1" monsterinfo.search mi_search
$HERE/fntable.sh "$1" monsterinfo.sight mi_sight
$HERE/fntable.sh "$1" monsterinfo.blocked mi_blocked
$HERE/fntable.sh "$1" monsterinfo.duck mi_duck
$HERE/fntable.sh "$1" monsterinfo.unduck mi_unduck
$HERE/fntable.sh "$1" monsterinfo.sidestep mi_sidestep

$HERE/fntable.sh "$1" moveinfo.endfunc mv_end

/*
 * Copyright (C) 1997-2001 Id Software 30 Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License 30 or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful 30 but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not 30 write to the Free Software
 * Foundation 30 Inc. 30 59 Temple Place - Suite 330 30 Boston 30 MA
 * 02111-1307 30 USA.
 *
 * =======================================================================
 *
 * Army/soldier animations.
 *
 * =======================================================================
 */

#define FRAME_stand1 0
#define FRAME_stand2 1
#define FRAME_stand3 2
#define FRAME_stand4 3
#define FRAME_stand5 4
#define FRAME_stand6 5
#define FRAME_stand7 6
#define FRAME_stand8 7
#define FRAME_death1 8
#define FRAME_death2 9
#define FRAME_death3 10
#define FRAME_death4 11
#define FRAME_death5 12
#define FRAME_death6 13
#define FRAME_death7 14
#define FRAME_death8 15
#define FRAME_death9 16
#define FRAME_death10 17
#define FRAME_deathc1 18
#define FRAME_deathc2 19
#define FRAME_deathc3 20
#define FRAME_deathc4 21
#define FRAME_deathc5 22
#define FRAME_deathc6 23
#define FRAME_deathc7 24
#define FRAME_deathc8 25
#define FRAME_deathc9 26
#define FRAME_deathc10 27
#define FRAME_deathc11 28
#define FRAME_load1 29
#define FRAME_load2 30
#define FRAME_load3 31
#define FRAME_load4 32
#define FRAME_load5 33
#define FRAME_load6 34
#define FRAME_load7 35
#define FRAME_load8 36
#define FRAME_load9 37
#define FRAME_load10 38
#define FRAME_load11 39
#define FRAME_pain1 40
#define FRAME_pain2 41
#define FRAME_pain3 42
#define FRAME_pain4 43
#define FRAME_pain5 44
#define FRAME_pain6 45
#define FRAME_painb1 46
#define FRAME_painb2 47
#define FRAME_painb3 48
#define FRAME_painb4 49
#define FRAME_painb5 50
#define FRAME_painb6 51
#define FRAME_painb7 52
#define FRAME_painb8 53
#define FRAME_painb9 54
#define FRAME_painb10 55
#define FRAME_painb11 56
#define FRAME_painb12 57
#define FRAME_painb13 58
#define FRAME_painb14 59
#define FRAME_painc1 60
#define FRAME_painc2 61
#define FRAME_painc3 62
#define FRAME_painc4 63
#define FRAME_painc5 64
#define FRAME_painc6 65
#define FRAME_painc7 66
#define FRAME_painc8 67
#define FRAME_painc9 68
#define FRAME_painc10 69
#define FRAME_painc11 70
#define FRAME_painc12 71
#define FRAME_painc13 72
#define FRAME_run1 73
#define FRAME_run2 74
#define FRAME_run3 75
#define FRAME_run4 76
#define FRAME_run5 77
#define FRAME_run6 78
#define FRAME_run7 79
#define FRAME_run8 80
#define FRAME_shoot1 81
#define FRAME_shoot2 82
#define FRAME_shoot3 83
#define FRAME_shoot4 84
#define FRAME_shoot5 85
#define FRAME_shoot6 86
#define FRAME_shoot7 87
#define FRAME_shoot8 88
#define FRAME_shoot9 89
#define FRAME_prowl_1 90
#define FRAME_prowl_2 91
#define FRAME_prowl_3 92
#define FRAME_prowl_4 93
#define FRAME_prowl_5 94
#define FRAME_prowl_6 95
#define FRAME_prowl_7 96
#define FRAME_prowl_8 97
#define FRAME_prowl_9 98
#define FRAME_prowl_10 99
#define FRAME_prowl_11 100
#define FRAME_prowl_12 101
#define FRAME_prowl_13 102
#define FRAME_prowl_14 103
#define FRAME_prowl_15 104
#define FRAME_prowl_16 105
#define FRAME_prowl_17 106
#define FRAME_prowl_18 107
#define FRAME_prowl_19 108
#define FRAME_prowl_20 109
#define FRAME_prowl_21 110
#define FRAME_prowl_22 111
#define FRAME_prowl_23 112
#define FRAME_prowl_24 113

#define MODEL_SCALE 1.000000

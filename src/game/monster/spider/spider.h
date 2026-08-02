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
 * Oblivion Spider animations.
 *
 * =======================================================================
 */

#define FRAME_standA1 0
#define FRAME_standA2 1
#define FRAME_standA3 2
#define FRAME_standA4 3
#define FRAME_standA5 4
#define FRAME_standA6 5
#define FRAME_standA7 6
#define FRAME_standA8 7
#define FRAME_standA9 8
#define FRAME_standA10 9
#define FRAME_standA11 10
#define FRAME_standA12 11
#define FRAME_standA13 12
#define FRAME_standA14 13
#define FRAME_standA15 14
#define FRAME_standA16 15
#define FRAME_standA17 16
#define FRAME_standA18 17
#define FRAME_standA19 18
#define FRAME_standA20 19
#define FRAME_standA21 20
#define FRAME_standA22 21
#define FRAME_standA23 22
#define FRAME_standA24 23
#define FRAME_standA25 24
#define FRAME_standA26 25
#define FRAME_standA27 26
#define FRAME_standA28 27
#define FRAME_standA29 28
#define FRAME_standA30 29
#define FRAME_standA31 30
#define FRAME_standA32 31
#define FRAME_standA33 32
#define FRAME_standA34 33
#define FRAME_standA35 34
#define FRAME_standA36 35
#define FRAME_standA37 36
#define FRAME_standA38 37
#define FRAME_standA39 38
#define FRAME_standA40 39
#define FRAME_standA41 40
#define FRAME_standA42 41
#define FRAME_standA43 42
#define FRAME_standA44 43
#define FRAME_standA45 44
#define FRAME_standA46 45
#define FRAME_standA47 46
#define FRAME_standA48 47
#define FRAME_standA49 48
#define FRAME_standA50 49
#define FRAME_standA51 50
#define FRAME_standA52 51
#define FRAME_standA53 52
#define FRAME_standA54 53
#define FRAME_standA55 54
#define FRAME_walkA1 55
#define FRAME_walkA2 56
#define FRAME_walkA3 57
#define FRAME_walkA4 58
#define FRAME_walkA5 59
#define FRAME_walkA6 60
#define FRAME_walkA7 61
#define FRAME_walkA8 62
#define FRAME_walkA9 63
#define FRAME_walkA10 64
#define FRAME_runA1 65
#define FRAME_runA2 66
#define FRAME_runA3 67
#define FRAME_runA4 68
#define FRAME_runA5 69
#define FRAME_runA6 70
#define FRAME_runA7 71
#define FRAME_runA8 72
#define FRAME_runA9 73
#define FRAME_runA10 74
#define FRAME_runB1 75
#define FRAME_runB2 76
#define FRAME_runB3 77
#define FRAME_runB4 78
#define FRAME_runB5 79
#define FRAME_runB6 80
#define FRAME_attackL1 81
#define FRAME_attackL2 82
#define FRAME_attackL3 83
#define FRAME_attackL4 84
#define FRAME_attackL5 85
#define FRAME_attackR1 86
#define FRAME_attackR2 87
#define FRAME_attackR3 88
#define FRAME_attackR4 89
#define FRAME_attackR5 90
#define FRAME_attackB1 91
#define FRAME_attackB2 92
#define FRAME_attackB3 93
#define FRAME_attackB4 94
#define FRAME_attackB5 95
#define FRAME_attackB6 96
#define FRAME_attackB7 97
#define FRAME_attackB8 98
#define FRAME_meleeA1 99
#define FRAME_meleeA2 100
#define FRAME_meleeA3 101
#define FRAME_meleeA4 102
#define FRAME_meleeA5 103
#define FRAME_meleeB1 104
#define FRAME_meleeB2 105
#define FRAME_meleeB3 106
#define FRAME_meleeB4 107
#define FRAME_meleeB5 108
#define FRAME_meleeB6 109
#define FRAME_meleeB7 110
#define FRAME_painA1 111
#define FRAME_painA2 112
#define FRAME_painA3 113
#define FRAME_painA4 114
#define FRAME_painA5 115
#define FRAME_painA6 116
#define FRAME_painB1 117
#define FRAME_painB2 118
#define FRAME_painB3 119
#define FRAME_painB4 120
#define FRAME_painB5 121
#define FRAME_painB6 122
#define FRAME_painB7 123
#define FRAME_painB8 124
#define FRAME_deathA1 125
#define FRAME_deathA2 126
#define FRAME_deathA3 127
#define FRAME_deathA4 128
#define FRAME_deathA5 129
#define FRAME_deathA6 130
#define FRAME_deathA7 131
#define FRAME_deathA8 132
#define FRAME_deathA9 133
#define FRAME_deathA10 134
#define FRAME_deathA11 135
#define FRAME_deathA12 136
#define FRAME_deathA13 137
#define FRAME_deathA14 138
#define FRAME_deathA15 139
#define FRAME_deathA16 140
#define FRAME_deathA17 141
#define FRAME_deathA18 142
#define FRAME_deathA19 143
#define FRAME_deathA20 144
#define FRAME_deathB1 145
#define FRAME_deathB2 146
#define FRAME_deathB3 147
#define FRAME_deathB4 148
#define FRAME_deathB5 149
#define FRAME_deathB6 150
#define FRAME_deathB7 151
#define FRAME_deathB8 152
#define FRAME_deathB9 153
#define FRAME_deathB10 154
#define FRAME_deathB11 155
#define FRAME_deathB12 156
#define FRAME_deathB13 157
#define FRAME_deathB14 158
#define FRAME_deathB15 159
#define FRAME_deathB16 160
#define FRAME_deathB17 161
#define FRAME_deathB18 162
#define FRAME_deathB19 163
#define FRAME_deathB20 164
#define FRAME_TEMPLATE1 165
#define FRAME_ORIGskin1 166
#define FRAME_skin 167

#define MODEL_SCALE 1.000000

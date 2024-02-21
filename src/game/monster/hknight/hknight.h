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
 * Hknight animations.
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
#define FRAME_stand9 8
#define FRAME_walk1 9
#define FRAME_walk2 10
#define FRAME_walk3 11
#define FRAME_walk4 12
#define FRAME_walk5 13
#define FRAME_walk6 14
#define FRAME_walk7 15
#define FRAME_walk8 16
#define FRAME_walk9 17
#define FRAME_walk10 18
#define FRAME_walk11 19
#define FRAME_walk12 20
#define FRAME_walk13 21
#define FRAME_walk14 22
#define FRAME_walk15 23
#define FRAME_walk16 24
#define FRAME_walk17 25
#define FRAME_walk18 26
#define FRAME_walk19 27
#define FRAME_walk20 28
#define FRAME_run1 29
#define FRAME_run2 30
#define FRAME_run3 31
#define FRAME_run4 32
#define FRAME_run5 33
#define FRAME_run6 34
#define FRAME_run7 35
#define FRAME_run8 36
#define FRAME_pain1 37
#define FRAME_pain2 38
#define FRAME_pain3 39
#define FRAME_pain4 40
#define FRAME_pain5 41
#define FRAME_death1 42
#define FRAME_death2 43
#define FRAME_death3 44
#define FRAME_death4 45
#define FRAME_death5 46
#define FRAME_death6 47
#define FRAME_death7 48
#define FRAME_death8 49
#define FRAME_death9 50
#define FRAME_death10 51
#define FRAME_death11 52
#define FRAME_death12 53
#define FRAME_deathb1 54
#define FRAME_deathb2 55
#define FRAME_deathb3 56
#define FRAME_deathb4 57
#define FRAME_deathb5 58
#define FRAME_deathb6 59
#define FRAME_deathb7 60
#define FRAME_deathb8 61
#define FRAME_deathb9 62
#define FRAME_char_a1 63
#define FRAME_char_a2 64
#define FRAME_char_a3 65
#define FRAME_char_a4 66
#define FRAME_char_a5 67
#define FRAME_char_a6 68
#define FRAME_char_a7 69
#define FRAME_char_a8 70
#define FRAME_char_a9 71
#define FRAME_char_a10 72
#define FRAME_char_a11 73
#define FRAME_char_a12 74
#define FRAME_char_a13 75
#define FRAME_char_a14 76
#define FRAME_char_a15 77
#define FRAME_char_a16 78
#define FRAME_magica1 79
#define FRAME_magica2 80
#define FRAME_magica3 81
#define FRAME_magica4 82
#define FRAME_magica5 83
#define FRAME_magica6 84
#define FRAME_magica7 85
#define FRAME_magica8 86
#define FRAME_magica9 87
#define FRAME_magica10 88
#define FRAME_magica11 89
#define FRAME_magica12 90
#define FRAME_magica13 91
#define FRAME_magica14 92
#define FRAME_magicb1 93
#define FRAME_magicb2 94
#define FRAME_magicb3 95
#define FRAME_magicb4 96
#define FRAME_magicb5 97
#define FRAME_magicb6 98
#define FRAME_magicb7 99
#define FRAME_magicb8 100
#define FRAME_magicb9 101
#define FRAME_magicb10 102
#define FRAME_magicb11 103
#define FRAME_magicb12 104
#define FRAME_magicb13 105
#define FRAME_char_b1 106
#define FRAME_char_b2 107
#define FRAME_char_b3 108
#define FRAME_char_b4 109
#define FRAME_char_b5 110
#define FRAME_char_b6 111
#define FRAME_slice1 112
#define FRAME_slice2 113
#define FRAME_slice3 114
#define FRAME_slice4 115
#define FRAME_slice5 116
#define FRAME_slice6 117
#define FRAME_slice7 118
#define FRAME_slice8 119
#define FRAME_slice9 120
#define FRAME_slice10 121
#define FRAME_smash1 122
#define FRAME_smash2 123
#define FRAME_smash3 124
#define FRAME_smash4 125
#define FRAME_smash5 126
#define FRAME_smash6 127
#define FRAME_smash7 128
#define FRAME_smash8 129
#define FRAME_smash9 130
#define FRAME_smash10 131
#define FRAME_smash11 132
#define FRAME_w_attack1 133
#define FRAME_w_attack2 134
#define FRAME_w_attack3 135
#define FRAME_w_attack4 136
#define FRAME_w_attack5 137
#define FRAME_w_attack6 138
#define FRAME_w_attack7 139
#define FRAME_w_attack8 140
#define FRAME_w_attack9 141
#define FRAME_w_attack10 142
#define FRAME_w_attack11 143
#define FRAME_w_attack12 144
#define FRAME_w_attack13 145
#define FRAME_w_attack14 146
#define FRAME_w_attack15 147
#define FRAME_w_attack16 148
#define FRAME_w_attack17 149
#define FRAME_w_attack18 150
#define FRAME_w_attack19 151
#define FRAME_w_attack20 152
#define FRAME_w_attack21 153
#define FRAME_w_attack22 154
#define FRAME_magicc1 155
#define FRAME_magicc2 156
#define FRAME_magicc3 157
#define FRAME_magicc4 158
#define FRAME_magicc5 159
#define FRAME_magicc6 160
#define FRAME_magicc7 161
#define FRAME_magicc8 162
#define FRAME_magicc9 163
#define FRAME_magicc10 164
#define FRAME_magicc11 165

#define MODEL_SCALE 1.000000

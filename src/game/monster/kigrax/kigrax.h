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
 * Oblivion Kigrax animations.
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
#define FRAME_stand10 9
#define FRAME_stand11 10
#define FRAME_stand12 11
#define FRAME_stand13 12
#define FRAME_stand14 13
#define FRAME_stand15 14
#define FRAME_stand16 15
#define FRAME_stand17 16
#define FRAME_stand18 17
#define FRAME_stand19 18
#define FRAME_stand20 19
#define FRAME_stand21 20
#define FRAME_stand22 21
#define FRAME_stand23 22
#define FRAME_stand24 23
#define FRAME_stand25 24
#define FRAME_stand26 25
#define FRAME_stand27 26
#define FRAME_stand28 27
#define FRAME_standidle1 28
#define FRAME_standidle2 29
#define FRAME_standidle3 30
#define FRAME_standidle4 31
#define FRAME_standidle5 32
#define FRAME_standidle6 33
#define FRAME_standidle7 34
#define FRAME_standidle8 35
#define FRAME_standidle9 36
#define FRAME_standidle10 37
#define FRAME_standidle11 38
#define FRAME_standidle12 39
#define FRAME_standidle13 40
#define FRAME_standidle14 41
#define FRAME_standidle15 42
#define FRAME_standidle16 43
#define FRAME_standidle17 44
#define FRAME_standidle18 45
#define FRAME_standidle19 46
#define FRAME_standidle20 47
#define FRAME_standidle21 48
#define FRAME_settle1 49
#define FRAME_settle2 50
#define FRAME_settle3 51
#define FRAME_settle4 52
#define FRAME_settle5 53
#define FRAME_settle6 54
#define FRAME_settle7 55
#define FRAME_settle8 56
#define FRAME_settle9 57
#define FRAME_settle10 58
#define FRAME_settle11 59
#define FRAME_settle12 60
#define FRAME_walk1 61
#define FRAME_walk2 62
#define FRAME_walk3 63
#define FRAME_walk4 64
#define FRAME_walk5 65
#define FRAME_walk6 66
#define FRAME_walk7 67
#define FRAME_walk8 68
#define FRAME_walk9 69
#define FRAME_walk10 70
#define FRAME_walk11 71
#define FRAME_walk12 72
#define FRAME_walk13 73
#define FRAME_walk14 74
#define FRAME_walk15 75
#define FRAME_walk16 76
#define FRAME_walk17 77
#define FRAME_walk18 78
#define FRAME_walk19 79
#define FRAME_walk20 80
#define FRAME_walk21 81
#define FRAME_walk22 82
#define FRAME_walkidle1 83
#define FRAME_walkidle2 84
#define FRAME_walkidle3 85
#define FRAME_walkidle4 86
#define FRAME_walkidle5 87
#define FRAME_walkidle6 88
#define FRAME_walkidle7 89
#define FRAME_walkidle8 90
#define FRAME_walkidle9 91
#define FRAME_walkidle10 92
#define FRAME_walkidle11 93
#define FRAME_walkidle12 94
#define FRAME_walkidle13 95
#define FRAME_walkidle14 96
#define FRAME_walkidle15 97
#define FRAME_walkidle16 98
#define FRAME_walkidle17 99
#define FRAME_walkidle18 100
#define FRAME_walkidle19 101
#define FRAME_walkidle20 102
#define FRAME_walkidle21 103
#define FRAME_walkidle22 104
#define FRAME_sight1 105
#define FRAME_sight2 106
#define FRAME_sight3 107
#define FRAME_sight4 108
#define FRAME_sight5 109
#define FRAME_sight6 110
#define FRAME_sight7 111
#define FRAME_sight8 112
#define FRAME_sight9 113
#define FRAME_sight10 114
#define FRAME_sight11 115
#define FRAME_sight12 116
#define FRAME_sight13 117
#define FRAME_sight14 118
#define FRAME_sight15 119
#define FRAME_sight16 120
#define FRAME_sight17 121
#define FRAME_run1 122
#define FRAME_run2 123
#define FRAME_run3 124
#define FRAME_run4 125
#define FRAME_run5 126
#define FRAME_run6 127
#define FRAME_run7 128
#define FRAME_run8 129
#define FRAME_run9 130
#define FRAME_run10 131
#define FRAME_run11 132
#define FRAME_run12 133
#define FRAME_run13 134
#define FRAME_run14 135
#define FRAME_run15 136
#define FRAME_run16 137
#define FRAME_run17 138
#define FRAME_pain1 139
#define FRAME_pain2 140
#define FRAME_pain3 141
#define FRAME_pain4 142
#define FRAME_pain5 143
#define FRAME_pain6 144
#define FRAME_pain7 145
#define FRAME_pain8 146
#define FRAME_pain9 147
#define FRAME_pain10 148
#define FRAME_pain11 149
#define FRAME_death1 150
#define FRAME_death2 151
#define FRAME_death3 152
#define FRAME_death4 153
#define FRAME_death5 154
#define FRAME_death6 155
#define FRAME_death7 156
#define FRAME_death8 157
#define FRAME_death9 158
#define FRAME_death10 159
#define FRAME_death11 160
#define FRAME_death12 161
#define FRAME_death13 162
#define FRAME_death14 163
#define FRAME_death15 164
#define FRAME_death16 165
#define FRAME_death17 166
#define FRAME_death18 167
#define FRAME_death19 168
#define FRAME_claw1 169
#define FRAME_claw2 170
#define FRAME_claw3 171
#define FRAME_claw4 172
#define FRAME_claw5 173
#define FRAME_claw6 174
#define FRAME_claw7 175
#define FRAME_claw8 176
#define FRAME_claw9 177
#define FRAME_claw10 178
#define FRAME_claw11 179
#define FRAME_claw12 180
#define FRAME_claw13 181
#define FRAME_claw14 182
#define FRAME_claw15 183
#define FRAME_rake1 184
#define FRAME_rake2 185
#define FRAME_rake3 186
#define FRAME_rake4 187
#define FRAME_rake5 188
#define FRAME_rake6 189
#define FRAME_rake7 190
#define FRAME_rake8 191
#define FRAME_rake9 192
#define FRAME_rake10 193
#define FRAME_rake11 194
#define FRAME_blaster1 195
#define FRAME_blaster2 196
#define FRAME_blaster3 197
#define FRAME_blaster4 198
#define FRAME_blaster5 199
#define FRAME_blaster6 200
#define FRAME_blaster7 201
#define FRAME_blaster8 202
#define FRAME_blaster9 203
#define FRAME_blaster10 204

#define MODEL_SCALE 1.000000

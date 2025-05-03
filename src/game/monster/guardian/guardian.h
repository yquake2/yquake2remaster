/*
 * Copyright (C) 1997-2001 Id Software  Inc.
 * Copyright (c) ZeniMax Media Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License  or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not  write to the Free Software
 * Foundation  Inc.  59 Temple Place - Suite 330  Boston  MA
 * 02111-1307  USA.
 *
 * =======================================================================
 *
 * Guardian animations.
 *
 * =======================================================================
 */

#define FRAME_sleep1 0
#define FRAME_sleep2 1
#define FRAME_sleep3 2
#define FRAME_sleep4 3
#define FRAME_sleep5 4
#define FRAME_sleep6 5
#define FRAME_sleep7 6
#define FRAME_sleep8 7
#define FRAME_sleep9 8
#define FRAME_sleep10 9
#define FRAME_sleep11 10
#define FRAME_sleep12 11
#define FRAME_sleep13 12
#define FRAME_sleep14 13
#define FRAME_death1 14
#define FRAME_death2 15
#define FRAME_death3 16
#define FRAME_death4 17
#define FRAME_death5 18
#define FRAME_death6 19
#define FRAME_death7 20
#define FRAME_death8 21
#define FRAME_death9 22
#define FRAME_death10 23
#define FRAME_death11 24
#define FRAME_death12 25
#define FRAME_death13 26
#define FRAME_death14 27
#define FRAME_death15 28
#define FRAME_death16 29
#define FRAME_death17 30
#define FRAME_death18 31
#define FRAME_death19 32
#define FRAME_death20 33
#define FRAME_death21 34
#define FRAME_death22 35
#define FRAME_death23 36
#define FRAME_death24 37
#define FRAME_death25 38
#define FRAME_death26 39
#define FRAME_atk1_out1 40
#define FRAME_atk1_out2 41
#define FRAME_atk1_out3 42
#define FRAME_atk2_out1 43
#define FRAME_atk2_out2 44
#define FRAME_atk2_out3 45
#define FRAME_atk2_out4 46
#define FRAME_atk2_out5 47
#define FRAME_atk2_out6 48
#define FRAME_atk2_out7 49
#define FRAME_kick_out1 50
#define FRAME_kick_out2 51
#define FRAME_kick_out3 52
#define FRAME_kick_out4 53
#define FRAME_kick_out5 54
#define FRAME_kick_out6 55
#define FRAME_kick_out7 56
#define FRAME_kick_out8 57
#define FRAME_kick_out9 58
#define FRAME_kick_out10 59
#define FRAME_kick_out11 60
#define FRAME_kick_out12 61
#define FRAME_pain1_1 62
#define FRAME_pain1_2 63
#define FRAME_pain1_3 64
#define FRAME_pain1_4 65
#define FRAME_pain1_5 66
#define FRAME_pain1_6 67
#define FRAME_pain1_7 68
#define FRAME_pain1_8 69
#define FRAME_idle1 70
#define FRAME_idle2 71
#define FRAME_idle3 72
#define FRAME_idle4 73
#define FRAME_idle5 74
#define FRAME_idle6 75
#define FRAME_idle7 76
#define FRAME_idle8 77
#define FRAME_idle9 78
#define FRAME_idle10 79
#define FRAME_idle11 80
#define FRAME_idle12 81
#define FRAME_idle13 82
#define FRAME_idle14 83
#define FRAME_idle15 84
#define FRAME_idle16 85
#define FRAME_idle17 86
#define FRAME_idle18 87
#define FRAME_idle19 88
#define FRAME_idle20 89
#define FRAME_idle21 90
#define FRAME_idle22 91
#define FRAME_idle23 92
#define FRAME_idle24 93
#define FRAME_idle25 94
#define FRAME_idle26 95
#define FRAME_idle27 96
#define FRAME_idle28 97
#define FRAME_idle29 98
#define FRAME_idle30 99
#define FRAME_idle31 100
#define FRAME_idle32 101
#define FRAME_idle33 102
#define FRAME_idle34 103
#define FRAME_idle35 104
#define FRAME_idle36 105
#define FRAME_idle37 106
#define FRAME_idle38 107
#define FRAME_idle39 108
#define FRAME_idle40 109
#define FRAME_idle41 110
#define FRAME_idle42 111
#define FRAME_idle43 112
#define FRAME_idle44 113
#define FRAME_idle45 114
#define FRAME_idle46 115
#define FRAME_idle47 116
#define FRAME_idle48 117
#define FRAME_idle49 118
#define FRAME_idle50 119
#define FRAME_idle51 120
#define FRAME_idle52 121
#define FRAME_atk1_in1 122
#define FRAME_atk1_in2 123
#define FRAME_atk1_in3 124
#define FRAME_kick_in1 125
#define FRAME_kick_in2 126
#define FRAME_kick_in3 127
#define FRAME_kick_in4 128
#define FRAME_kick_in5 129
#define FRAME_kick_in6 130
#define FRAME_kick_in7 131
#define FRAME_kick_in8 132
#define FRAME_kick_in9 133
#define FRAME_kick_in10 134
#define FRAME_kick_in11 135
#define FRAME_kick_in12 136
#define FRAME_kick_in13 137
#define FRAME_walk1 138
#define FRAME_walk2 139
#define FRAME_walk3 140
#define FRAME_walk4 141
#define FRAME_walk5 142
#define FRAME_walk6 143
#define FRAME_walk7 144
#define FRAME_walk8 145
#define FRAME_walk9 146
#define FRAME_walk10 147
#define FRAME_walk11 148
#define FRAME_walk12 149
#define FRAME_walk13 150
#define FRAME_walk14 151
#define FRAME_walk15 152
#define FRAME_walk16 153
#define FRAME_walk17 154
#define FRAME_walk18 155
#define FRAME_walk19 156
#define FRAME_wake1 157
#define FRAME_wake2 158
#define FRAME_wake3 159
#define FRAME_wake4 160
#define FRAME_wake5 161
#define FRAME_atk1_spin1 162
#define FRAME_atk1_spin2 163
#define FRAME_atk1_spin3 164
#define FRAME_atk1_spin4 165
#define FRAME_atk1_spin5 166
#define FRAME_atk1_spin6 167
#define FRAME_atk1_spin7 168
#define FRAME_atk1_spin8 169
#define FRAME_atk1_spin9 170
#define FRAME_atk1_spin10 171
#define FRAME_atk1_spin11 172
#define FRAME_atk1_spin12 173
#define FRAME_atk1_spin13 174
#define FRAME_atk1_spin14 175
#define FRAME_atk1_spin15 176
#define FRAME_atk2_fire1 177
#define FRAME_atk2_fire2 178
#define FRAME_atk2_fire3 179
#define FRAME_atk2_fire4 180
#define FRAME_turnl_1 181
#define FRAME_turnl_2 182
#define FRAME_turnl_3 183
#define FRAME_turnl_4 184
#define FRAME_turnl_5 185
#define FRAME_turnl_6 186
#define FRAME_turnl_7 187
#define FRAME_turnl_8 188
#define FRAME_turnl_9 189
#define FRAME_turnl_10 190
#define FRAME_turnl_11 191
#define FRAME_turnr_1 192
#define FRAME_turnr_2 193
#define FRAME_turnr_3 194
#define FRAME_turnr_4 195
#define FRAME_turnr_5 196
#define FRAME_turnr_6 197
#define FRAME_turnr_7 198
#define FRAME_turnr_8 199
#define FRAME_turnr_9 200
#define FRAME_turnr_10 201
#define FRAME_turnr_11 202
#define FRAME_atk2_in1 203
#define FRAME_atk2_in2 204
#define FRAME_atk2_in3 205
#define FRAME_atk2_in4 206
#define FRAME_atk2_in5 207
#define FRAME_atk2_in6 208
#define FRAME_atk2_in7 209
#define FRAME_atk2_in8 210
#define FRAME_atk2_in9 211
#define FRAME_atk2_in10 212
#define FRAME_atk2_in11 213
#define FRAME_atk2_in12 214

#define MODEL_SCALE 1.000000

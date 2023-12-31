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
 * Black Widow (stage 2) animations.
 *
 * =======================================================================
 */

#define FRAME_blackwidow3     	0
#define FRAME_walk01          	1
#define FRAME_walk02          	2
#define FRAME_walk03          	3
#define FRAME_walk04          	4
#define FRAME_walk05          	5
#define FRAME_walk06          	6
#define FRAME_walk07          	7
#define FRAME_walk08          	8
#define FRAME_walk09          	9
#define FRAME_spawn01         	10
#define FRAME_spawn02         	11
#define FRAME_spawn03         	12
#define FRAME_spawn04         	13
#define FRAME_spawn05         	14
#define FRAME_spawn06         	15
#define FRAME_spawn07         	16
#define FRAME_spawn08         	17
#define FRAME_spawn09         	18
#define FRAME_spawn10         	19
#define FRAME_spawn11         	20
#define FRAME_spawn12         	21
#define FRAME_spawn13         	22
#define FRAME_spawn14         	23
#define FRAME_spawn15         	24
#define FRAME_spawn16         	25
#define FRAME_spawn17         	26
#define FRAME_spawn18         	27
#define FRAME_firea01         	28
#define FRAME_firea02         	29
#define FRAME_firea03         	30
#define FRAME_firea04         	31
#define FRAME_firea05         	32
#define FRAME_firea06         	33
#define FRAME_firea07         	34
#define FRAME_fireb01         	35
#define FRAME_fireb02         	36
#define FRAME_fireb03         	37
#define FRAME_fireb04         	38
#define FRAME_fireb05         	39
#define FRAME_fireb06         	40
#define FRAME_fireb07         	41
#define FRAME_fireb08         	42
#define FRAME_fireb09         	43
#define FRAME_fireb10         	44
#define FRAME_fireb11         	45
#define FRAME_fireb12         	46
#define FRAME_tongs01         	47
#define FRAME_tongs02         	48
#define FRAME_tongs03         	49
#define FRAME_tongs04         	50
#define FRAME_tongs05         	51
#define FRAME_tongs06         	52
#define FRAME_tongs07         	53
#define FRAME_tongs08         	54
#define FRAME_pain01          	55
#define FRAME_pain02          	56
#define FRAME_pain03          	57
#define FRAME_pain04          	58
#define FRAME_pain05          	59
#define FRAME_death01         	60
#define FRAME_death02         	61
#define FRAME_death03         	62
#define FRAME_death04         	63
#define FRAME_death05         	64
#define FRAME_death06         	65
#define FRAME_death07         	66
#define FRAME_death08         	67
#define FRAME_death09         	68
#define FRAME_death10         	69
#define FRAME_death11         	70
#define FRAME_death12         	71
#define FRAME_death13         	72
#define FRAME_death14         	73
#define FRAME_death15         	74
#define FRAME_death16         	75
#define FRAME_death17         	76
#define FRAME_death18         	77
#define FRAME_death19         	78
#define FRAME_death20         	79
#define FRAME_death21         	80
#define FRAME_death22         	81
#define FRAME_death23         	82
#define FRAME_death24         	83
#define FRAME_death25         	84
#define FRAME_death26         	85
#define FRAME_death27         	86
#define FRAME_death28         	87
#define FRAME_death29         	88
#define FRAME_death30         	89
#define FRAME_death31         	90
#define FRAME_death32         	91
#define FRAME_death33         	92
#define FRAME_death34         	93
#define FRAME_death35         	94
#define FRAME_death36         	95
#define FRAME_death37         	96
#define FRAME_death38         	97
#define FRAME_death39         	98
#define FRAME_death40         	99
#define FRAME_death41         	100
#define FRAME_death42         	101
#define FRAME_death43         	102
#define FRAME_death44         	103
#define FRAME_dthsrh01        	104
#define FRAME_dthsrh02        	105
#define FRAME_dthsrh03        	106
#define FRAME_dthsrh04        	107
#define FRAME_dthsrh05        	108
#define FRAME_dthsrh06        	109
#define FRAME_dthsrh07        	110
#define FRAME_dthsrh08        	111
#define FRAME_dthsrh09        	112
#define FRAME_dthsrh10        	113
#define FRAME_dthsrh11        	114
#define FRAME_dthsrh12        	115
#define FRAME_dthsrh13        	116
#define FRAME_dthsrh14        	117
#define FRAME_dthsrh15        	118
#define FRAME_dthsrh16        	119
#define FRAME_dthsrh17        	120
#define FRAME_dthsrh18        	121
#define FRAME_dthsrh19        	122
#define FRAME_dthsrh20        	123
#define FRAME_dthsrh21        	124
#define FRAME_dthsrh22        	125
#define MODEL_SCALE				2.000000

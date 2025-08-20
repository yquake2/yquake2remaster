/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * Prototypes for every spawn function in the game.so.
 *
 * =======================================================================
 */

extern void SP_CreateCoopSpots(edict_t * self);
extern void SP_CreateUnnamedSpawn(edict_t * self);
extern void SP_FixCoopSpots(edict_t * self);
extern void SP_choose_cdtrack(edict_t *self);
extern void SP_dm_dball_ball(edict_t * self);
extern void SP_dm_dball_ball_start(edict_t * self);
extern void SP_dm_dball_goal(edict_t * self);
extern void SP_dm_dball_speed_change(edict_t * self);
extern void SP_dm_dball_team1_start(edict_t * self);
extern void SP_dm_dball_team2_start(edict_t * self);
extern void SP_dm_tag_token(edict_t * self);
extern void SP_env_fire ( edict_t * ent ) ;
extern void SP_func_areaportal ( edict_t * ent ) ;
extern void SP_func_button ( edict_t * ent ) ;
extern void SP_func_clock(edict_t * self);
extern void SP_func_conveyor(edict_t * self);
extern void SP_func_door ( edict_t * ent ) ;
extern void SP_func_door_rotating ( edict_t * ent ) ;
extern void SP_func_door_secret ( edict_t * ent ) ;
extern void SP_func_door_secret2 ( edict_t * ent ) ;
extern void SP_func_explosive(edict_t * self);
extern void SP_func_force_wall ( edict_t * ent ) ;
extern void SP_func_killbox ( edict_t * ent ) ;
extern void SP_func_object(edict_t * self);
extern void SP_func_plat ( edict_t * ent ) ;
extern void SP_func_plat2 ( edict_t * ent ) ;
extern void SP_func_rotating ( edict_t * ent ) ;
extern void SP_func_timer(edict_t * self);
extern void SP_func_train(edict_t * self);
extern void SP_func_wall(edict_t * self);
extern void SP_func_water(edict_t * self);
extern void SP_hint_path(edict_t * self);
extern void SP_info_notnull(edict_t * self);
extern void SP_info_null(edict_t * self);
extern void SP_info_player_coop(edict_t * self);
extern void SP_info_player_coop_lava(edict_t * self);
extern void SP_info_player_deathmatch(edict_t * self);
extern void SP_info_player_intermission ( edict_t * ent ) ;
extern void SP_info_player_start(edict_t * self);
extern void SP_info_teleport_destination(edict_t * self);
extern void SP_item_foodcube(edict_t * self);
extern void SP_item_health(edict_t * self);
extern void SP_item_health_large(edict_t * self);
extern void SP_item_health_mega(edict_t * self);
extern void SP_item_health_small(edict_t * self);
extern void SP_light(edict_t * self);
extern void SP_light_mine1 ( edict_t * ent ) ;
extern void SP_light_mine2 ( edict_t * ent ) ;
extern void SP_misc_actor(edict_t * self);
extern void SP_misc_amb4 ( edict_t * ent ) ;
extern void SP_misc_banner ( edict_t * ent ) ;
extern void SP_misc_bigviper ( edict_t * ent ) ;
extern void SP_misc_blackhole ( edict_t * ent ) ;
extern void SP_misc_crashviper ( edict_t * ent ) ;
extern void SP_misc_deadsoldier ( edict_t * ent ) ;
extern void SP_misc_easterchick ( edict_t * ent ) ;
extern void SP_misc_easterchick2 ( edict_t * ent ) ;
extern void SP_misc_eastertank ( edict_t * ent ) ;
extern void SP_misc_explobox(edict_t * self);
extern void SP_misc_gib_arm ( edict_t * ent ) ;
extern void SP_misc_gib_head ( edict_t * ent ) ;
extern void SP_misc_gib_leg ( edict_t * ent ) ;
extern void SP_misc_insane(edict_t * self);
extern void SP_misc_nuke ( edict_t * ent ) ;
extern void SP_misc_nuke_core ( edict_t * ent ) ;
extern void SP_misc_flare ( edict_t * ent ) ;
extern void SP_misc_model ( edict_t * ent ) ;
extern void SP_misc_player_mannequin ( edict_t * ent ) ;
extern void SP_misc_satellite_dish ( edict_t * ent ) ;
extern void SP_misc_strogg_ship ( edict_t * ent ) ;
extern void SP_misc_teleporter ( edict_t * ent ) ;
extern void SP_misc_teleporter_dest ( edict_t * ent ) ;
extern void SP_misc_transport ( edict_t * ent ) ;
extern void SP_misc_viper ( edict_t * ent ) ;
extern void SP_misc_viper_bomb(edict_t * self);
extern void SP_misc_viper_missile(edict_t * self);
extern void SP_monster_arachnid(edict_t * self);
extern void SP_monster_army(edict_t * self);
extern void SP_monster_berserk(edict_t * self);
extern void SP_monster_boss2(edict_t * self);
extern void SP_monster_boss3_stand(edict_t * self);
extern void SP_monster_boss5(edict_t * self);
extern void SP_monster_brain(edict_t * self);
extern void SP_monster_carrier(edict_t * self);
extern void SP_monster_chick(edict_t * self);
extern void SP_monster_chick_heat(edict_t * self);
extern void SP_monster_commander_body(edict_t * self);
extern void SP_monster_demon(edict_t * self);
extern void SP_monster_dog(edict_t * self);
extern void SP_monster_enforcer(edict_t * self);
extern void SP_monster_rotfish(edict_t * self);
extern void SP_monster_fixbot(edict_t * self);
extern void SP_monster_flipper(edict_t * self);
extern void SP_monster_floater(edict_t * self);
extern void SP_monster_flyer(edict_t * self);
extern void SP_monster_gekk(edict_t * self);
extern void SP_monster_gladb(edict_t * self);
extern void SP_monster_gladiator(edict_t * self);
extern void SP_monster_guardian(edict_t * self);
extern void SP_monster_guncmdr(edict_t * self);
extern void SP_monster_gunner(edict_t * self);
extern void SP_monster_hknight(edict_t * self);
extern void SP_monster_hover(edict_t * self);
extern void SP_monster_infantry(edict_t * self);
extern void SP_monster_jorg(edict_t * self);
extern void SP_monster_kamikaze(edict_t * self);
extern void SP_monster_knight(edict_t * self);
extern void SP_monster_makron(edict_t * self);
extern void SP_monster_medic(edict_t * self);
extern void SP_monster_mutant(edict_t * self);
extern void SP_monster_ogre(edict_t * self);
extern void SP_monster_parasite(edict_t * self);
extern void SP_monster_shalrath(edict_t * self);
extern void SP_monster_shambler(edict_t* self);
extern void SP_monster_soldier(edict_t * self);
extern void SP_monster_soldier_h(edict_t * self);
extern void SP_monster_soldier_hypergun(edict_t * self);
extern void SP_monster_soldier_lasergun(edict_t * self);
extern void SP_monster_soldier_light(edict_t * self);
extern void SP_monster_soldier_ripper(edict_t * self);
extern void SP_monster_soldier_ss(edict_t * self);
extern void SP_monster_soldier_x(edict_t * self);
extern void SP_monster_stalker(edict_t * self);
extern void SP_monster_supertank(edict_t * self);
extern void SP_monster_tank(edict_t * self);
extern void SP_monster_tarbaby(edict_t * self);
extern void SP_monster_turret(edict_t * self);
extern void SP_monster_widow(edict_t * self);
extern void SP_monster_widow2(edict_t * self);
extern void SP_monster_wizard(edict_t * self);
extern void SP_monster_zombie(edict_t * self);
extern void SP_npc_timeminder(edict_t * self);
extern void SP_object_big_fire ( edict_t * ent ) ;
extern void SP_object_campfire ( edict_t * ent ) ;
extern void SP_object_flame1 ( edict_t * ent ) ;
extern void SP_object_repair ( edict_t * ent ) ;
extern void SP_path_corner(edict_t * self);
extern void SP_point_combat(edict_t * self);
extern void SP_quake_light_flame(edict_t * self);
extern void SP_rotating_light(edict_t * self);
extern void SP_target_actor(edict_t * self);
extern void SP_target_anger(edict_t * self);
extern void SP_target_autosave(edict_t * self);
extern void SP_target_blacklight ( edict_t * ent ) ;
extern void SP_target_blaster(edict_t * self);
extern void SP_target_camera(edict_t* self);
extern void SP_target_changelevel ( edict_t * ent ) ;
extern void SP_target_character(edict_t * self);
extern void SP_target_crosslevel_target(edict_t * self);
extern void SP_target_crosslevel_trigger(edict_t * self);
extern void SP_target_earthquake(edict_t * self);
extern void SP_target_explosion ( edict_t * ent ) ;
extern void SP_target_goal ( edict_t * ent ) ;
extern void SP_target_gravity (edict_t * self);
extern void SP_target_help ( edict_t * ent ) ;
extern void SP_target_killplayers(edict_t * self);
extern void SP_target_laser(edict_t * self);
extern void SP_target_light(edict_t *self);
extern void SP_target_lightramp(edict_t * self);
extern void SP_target_mal_laser(edict_t * self);
extern void SP_target_music(edict_t * self);
extern void SP_target_orb ( edict_t * ent ) ;
extern void SP_target_secret ( edict_t * ent ) ;
extern void SP_target_sky(edict_t * self);
extern void SP_target_soundfx(edict_t * self);
extern void SP_target_spawner(edict_t * self);
extern void SP_target_speaker ( edict_t * ent ) ;
extern void SP_target_splash(edict_t * self);
extern void SP_target_steam(edict_t * self);
extern void SP_target_string(edict_t * self);
extern void SP_target_temp_entity ( edict_t * ent ) ;
extern void SP_trigger_always ( edict_t * ent ) ;
extern void SP_trigger_counter(edict_t * self);
extern void SP_trigger_disguise(edict_t * self);
extern void SP_trigger_elevator(edict_t * self);
extern void SP_trigger_flashlight(edict_t * self);
extern void SP_trigger_fog(edict_t * self);
extern void SP_trigger_gravity(edict_t * self);
extern void SP_trigger_hurt(edict_t * self);
extern void SP_trigger_key(edict_t * self);
extern void SP_trigger_monsterjump(edict_t * self);
extern void SP_trigger_multiple ( edict_t * ent ) ;
extern void SP_trigger_once ( edict_t * ent ) ;
extern void SP_trigger_push(edict_t * self);
extern void SP_trigger_relay(edict_t * self);
extern void SP_trigger_teleport(edict_t * self);
extern void SP_turret_base(edict_t * self);
extern void SP_turret_breach(edict_t * self);
extern void SP_turret_driver(edict_t * self);
extern void SP_turret_invisible_brain(edict_t * self);
extern void SP_viewthing ( edict_t * ent ) ;
extern void SP_worldspawn ( edict_t * ent ) ;
extern void SP_xatrix_item(edict_t * self);

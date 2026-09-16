# Yamagi Quake II Remaster

[![Coverity Scan](https://img.shields.io/coverity/scan/31780)](https://scan.coverity.com/projects/yquake2-yquake2remaster)
[![Top Language](https://img.shields.io/github/languages/top/yquake2/yquake2remaster.svg)](https://github.com/yquake2/yquake2remaster)
[![Code Size](https://img.shields.io/github/languages/code-size/yquake2/yquake2remaster.svg)](https://github.com/yquake2/yquake2remaster)
[![Release](https://img.shields.io/github/release/yquake2/yquake2remaster.svg)](https://github.com/yquake2/yquake2remaster/releases/latest)
[![Release Date](https://img.shields.io/github/release-date/yquake2/yquake2remaster.svg)](https://github.com/yquake2/yquake2remaster/releases/latest)
[![AUR Version](https://img.shields.io/aur/version/yquake2remaster)](https://aur.archlinux.org/packages/yquake2remaster)
[![Downloads (total)](https://img.shields.io/github/downloads/yquake2/yquake2remaster/total)](https://github.com/yquake2/yquake2remaster/releases/latest)
[![Downloads (latest)](https://img.shields.io/github/downloads/yquake2/yquake2remaster/latest/total.svg)](https://github.com/yquake2/yquake2remaster/releases/latest)
[![Commits](https://img.shields.io/github/commits-since/yquake2/yquake2remaster/latest.svg)](https://github.com/yquake2/yquake2remaster/commits/master)
[![Last Commit](https://img.shields.io/github/last-commit/yquake2/yquake2remaster.svg)](https://github.com/yquake2/yquake2remaster/commits/master)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/coverity.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/coverity.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/linux_aarch64.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/linux_aarch64.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/linux_x86_64.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/linux_x86_64.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/macos.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/macos.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/win_mingw.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/win_mingw.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/win_msvc.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/win_msvc.yml)
[![Build Status](https://github.com/yquake2/yquake2remaster/actions/workflows/codeql.yml/badge.svg)](https://github.com/yquake2/yquake2remaster/actions/workflows/codeql.yml)

This is an experimental fork of Yamagi Quake II with ongoing work to add
support for Quake II Enhanced aka Q2 Remaster(ed). This enhanced version
has a lot non trivial changes, adding support isn't easy and takes time.
Feel free to try this code but you mileage may vary.

Have a look at the yquake2 repository for the "normal" Yamagi Quake II:
<https://github.com/yquake2/yquake2>

* Alpha windows 64 bit [binaries](https://github.com/yquake2/yquake2remaster/releases).
* Saves format is unstable and could change between alpha releases.
* MacOS build is only build tested and run is not checked.
* As benchmark could be used phoronix-test-suite [profile](https://openbenchmarking.org/test/denispauk/yquake2remaster).

### Building from Source

If you downloaded the Source Code (ZIP) directly from GitHub, please note that GitHub does
not include submodules in these archives. To build the project successfully, you have two options:

 * Follow the step-by-step instructions for a [lightweight setup](doc/020_installation.md#minimal-test-run-on-debian)
 * Use CMake without submodules.

### Asset Support

Yamagi Quake II Remaster supports a wide variety of model, texture, sprite, map, sound, and package formats from classic id Tech games and related titles (such as Quake, Quake 2, Half-Life, Hexen 2, Daikatana, SiN, Anachronox, Heretic 2, and Quake 3).

For complete technical specifications and compatibility tables, see [.agents/docs/asset_support.md](.agents/docs/asset_support.md).

### Games:

* Quake 2 ReRelease:
  * SDK: <https://github.com/id-Software/quake2-rerelease-dll>
  * Tech info: <https://bethesda.net/en/article/6NIyBxapXOurTKtF4aPiF4/enhancing-quake-ii>
  * PSX source: <https://www.moddb.com/mods/quake-ii-psx/downloads/quake-ii-psx-10-sources>
  * PSX Mod: <https://www.moddb.com/mods/quake-ii-psx>
* Anachronox:
  * SDK: <https://github.com/hogsy/chronon>
  * SDK: <https://code.idtech.space/ion-storm/anachronox-sdk>
  * Tech info: <https://anachrodox.talonbrave.info/>
* Kingpin:
  * SDK: <https://github.com/QuakeTools/Kingpin-SDK-v1.21>
  * SDK: <https://code.idtech.space/xatrix/kingpin-sdk>
  * Tech info: <https://www.kingpin.info/>
* Daikatana:
  * Info: <http://daikatananews.net/>
* Heretic 2:
  * SDK: <https://www.quaddicted.com/files/idgames2/planetquake/hereticii/files/Ht2Toolkit_v1.06.exe>
  * SDK: <https://code.idtech.space/raven/heretic2-sdk>
  * Tech info: <http://h2vault.infinityfreeapp.com/index.html>
* SiN:
  * Tools: [SiNview](https://web.archive.org/web/20001212060900/http://starbase.neosoft.com:80/~otaku/program.html)
  * Tools: <https://www.moddb.com/games/sin/downloads/sin-modding-tools-and-other-stuff>
  * SDK: <https://github.com/NightDive-Studio/sin-ex-game>
  * SDK: <https://github.com/jimdose/SiN_110_Source>
  * SDK: <https://code.idtech.space/ritual/sin-sdk>
* Dawn of Darkness:
  * Docs: <https://www.moddb.com/mods/dawn-of-darkness1/downloads/dod-mood-scripts-gsm-tutorials-fgd-and-def-file>
  * Demo: [Episode 1](https://www.moddb.com/mods/dawn-of-darkness1/downloads/dawn-of-darkness-episode-1)
* Infinity: The Kai'Ren Threat
  * Demo: [Demo maps](https://www.moddb.com/mods/infinity-the-kairen-threat/downloads/infinity-demo)
* Oblivion:
  * SDK: https://github.com/themuffinator/REBLIVION/
  * Demo: <https://www.celephais.net/oblivion/main.html>
* JaBot:
  * SDK: <https://www.moddb.com/mods/jabotq2/downloads/jabot-q2-v09x-win32-and-linux>
* Additional maps used for check maps support:
  * PSX: <https://www.moddb.com/mods/quake-ii-psx/downloads/quake-ii-psx-10>
  * ReRelease N64 Jam: <https://www.moddb.com/games/quake-2/addons/quake-2-re-release-n64-sp-map-jam>
  * ReRelease Basic Jam: <https://www.moddb.com/games/quake-2/addons/quake-2-re-release-back-to-baseq2ics-jam-1>
  * ReRelease PSX Jam: <https://www.moddb.com/mods/psx-jam-1/downloads/quake-2-re-release-psx-jam-1>
  * ReRelease Warehouse Jam: <https://www.moddb.com/mods/quake-2-re-release-warehouse-jam-1>

### Games check videos:

* 8.71RR15+:

[![Check Oblivion/Ininity state](https://img.youtube.com/vi/uixogpssDSI/hqdefault.jpg)](https://www.youtube.com/watch?v=uixogpssDSI)
[![Check BSP46 state](https://img.youtube.com/vi/dmrfJeqW9HU/hqdefault.jpg)](https://www.youtube.com/watch?v=dmrfJeqW9HU)
[![Check release state](https://img.youtube.com/vi/vFpdbPOcU4A/hqdefault.jpg)](https://www.youtube.com/watch?v=vFpdbPOcU4A)

* 8.61RR15+:

[![Check hologram implementation](https://img.youtube.com/vi/jTaCyx3KCr0/hqdefault.jpg)](https://www.youtube.com/watch?v=jTaCyx3KCr0)

* 8.61RR14+:

[![Check dynamic animation for weapons](https://img.youtube.com/vi/R8crz-ISkeA/hqdefault.jpg)](https://www.youtube.com/watch?v=R8crz-ISkeA)
[![Check infinity mod](https://img.youtube.com/vi/JYw_FEjaPJ4/hqdefault.jpg)](https://www.youtube.com/watch?v=JYw_FEjaPJ4)

* 8.61RR13+:

[![Check dynamic player swim](https://img.youtube.com/vi/8N_TJPsgvC0/hqdefault.jpg)](https://www.youtube.com/watch?v=8N_TJPsgvC0)
[![Check demo maps](https://img.youtube.com/vi/VkhrEOtKeZE/hqdefault.jpg)](https://www.youtube.com/watch?v=VkhrEOtKeZE)
[![Check demo maps](https://img.youtube.com/vi/SHZAEkgUJLg/hqdefault.jpg)](https://www.youtube.com/watch?v=SHZAEkgUJLg)

Checked with:
 * [Q2Test](https://archive.org/details/QuakeII_1020)
 * [Q2Demo](https://deponie.yamagi.org/quake2/idstuff/q2-314-demo-x86.exe)
 * [Q2 Release](https://store.steampowered.com/app/2320/Quake_II/)

* 8.61RR13:

[![Check dynamic frames](https://img.youtube.com/vi/dXJ5vOsYdvQ/hqdefault.jpg)](https://www.youtube.com/watch?v=dXJ5vOsYdvQ)

* 8.52RR13+:

[![Check dynamic frames split](https://img.youtube.com/vi/_CPmh5_TLbk/hqdefault.jpg)](https://www.youtube.com/watch?v=_CPmh5_TLbk)
[![Check dynamic frames groups](https://img.youtube.com/vi/qDSaRBsnh3k/hqdefault.jpg)](https://www.youtube.com/watch?v=qDSaRBsnh3k)
[![Check menu translation and fog](https://img.youtube.com/vi/HHj6m0z0uGA/hqdefault.jpg)](https://www.youtube.com/watch?v=HHj6m0z0uG)

* 8.52RR12+:

[![Quake I + Half Life 1 Demo](https://img.youtube.com/vi/_KY4bQpij0c/hqdefault.jpg)](https://www.youtube.com/watch?v=_KY4bQpij0c)
[![Quake I + Hexen 2](https://img.youtube.com/vi/uU87u1iEBeg/hqdefault.jpg)](https://www.youtube.com/watch?v=uU87u1iEBeg)
[![Heretic 2 Book / Translate](https://img.youtube.com/vi/4Sr_rhYP2lo/hqdefault.jpg)](https://www.youtube.com/watch?v=4Sr_rhYP2lo)

* 8.52RR12:

[![4k+ coordinates + flare](https://img.youtube.com/vi/L6BTWUUVh_k/hqdefault.jpg)](https://www.youtube.com/watch?v=L6BTWUUVh_k)

* 8.51RR12:

[![PSX check](https://img.youtube.com/vi/jAcNMbvrPe4/hqdefault.jpg)](https://www.youtube.com/watch?v=jAcNMbvrPe4)
[![Vault check](https://img.youtube.com/vi/a338jWr6uTc/hqdefault.jpg)](https://www.youtube.com/watch?v=a338jWr6uTc)

* 8.42RR12+:

[![JaBot check](https://img.youtube.com/vi/uW3XDxrjQOU/hqdefault.jpg)](https://www.youtube.com/watch?v=uW3XDxrjQOU)
[![Translation check](https://img.youtube.com/vi/8Tlm8lSY5x8/hqdefault.jpg)](https://www.youtube.com/watch?v=8Tlm8lSY5x8)
[![Anacronox dance sector issue](https://img.youtube.com/vi/PR2_nK7DFJM/hqdefault.jpg)](https://www.youtube.com/watch?v=PR2_nK7DFJM)

* 8.42RR11:

[![First episode](https://img.youtube.com/vi/Ha1FuVXaQSE/hqdefault.jpg)](https://www.youtube.com/watch?v=Ha1FuVXaQSE)
[![Q2DQ2](https://img.youtube.com/vi/6P3wJojExyI/hqdefault.jpg)](https://www.youtube.com/watch?v=6P3wJojExyI)
[![8.42RR11](https://img.youtube.com/vi/ukqBrx80ESM/hqdefault.jpg)](https://www.youtube.com/watch?v=ukqBrx80ESM)

* 8.42RR10:

[![8.42RR10](https://img.youtube.com/vi/obIrzYsNxBY/hqdefault.jpg)](https://www.youtube.com/watch?v=obIrzYsNxBY)

* 8.42RR9:

[![8.42RR9](https://img.youtube.com/vi/N0iHhEDkZFg/hqdefault.jpg)](https://www.youtube.com/watch?v=N0iHhEDkZFg)

* 8.42RR8:

[![8.42RR8](https://img.youtube.com/vi/NJ7T0cdyqk8/hqdefault.jpg)](https://www.youtube.com/watch?v=NJ7T0cdyqk8)

* 8.31RR7:

[![8.31RR7](https://img.youtube.com/vi/VAFs1HtQU_0/hqdefault.jpg)](https://www.youtube.com/watch?v=VAFs1HtQU_0)


### Goals, fully finished goals could be checked in [here](CHANGELOG):

* [ ] ReRelease maps: shows shadow in the middle of model,
* [ ] soft: fix crash with md5 models in player model select and ASAN=1,
* [ ] soft: q64/outpost scale textures unsupported,
* [ ] soft: broken wall light and wall glitch,
* [ ] soft: rework 32bit color cinema workarrounds,
* [ ] soft: support scalled textures for models and walls, and fix
    lighting with remastered maps,
* [ ] soft: use separete texture hi-color buffer for ui in soft render,
* [ ] vulkan: rearange surfaces before render,
* [ ] vulkan: add fog distance effect,
* [ ] soft: add fog distance effect (optional),
* [ ] reuse memory from models cache in renders model list,
* [ ] reuse memory from models cache for bsp,
* [ ] game: fix broken base3 with sorted fields names,
* [ ] game: code has not reset ctf flag on load saves,
* [ ] game: code has reset thirdperson flag on load new level,
* [ ] game: check RealBoundingBox with frame box,
* [ ] game/client: update bound box based on frame number,
* [ ] ReRelease: incorrect dead animation for Arachnid,
* [ ] ReRelease: broken fire effect for Guardian.
* [ ] ReRelease: water in basicsjam1_ziutek,
* [ ] ReRelease: make lightmap textures dynamic n64jam_palmlix,
* [ ] ReRelease: support `textures/*/*.mat load` texture effects,
* [ ] ReRelease: support `textures/*/*_glow.png` load,
* [ ] ReRelease: support `tactile/*/*.bnvib/.wav` feedback load,
* [ ] ReRelease: console `~` incorrectly show multibyte characters,
* [ ] ReRelease: basicsjam1_detrohogga: fix droptofloor startsolid,
* [ ] gl1, gl3, gl4, vk, soft: implement direction of `CS_SHADOWLIGHTS`,
* [ ] gl3, gl4: implement color multiplication and alpha gradient for `misc_flare`,
* [ ] gl3, gl4, vk: fix and port `r_bloom`,
* [ ] gl1, gl3, gl4, vk: apply improvements from gl3 to other renders,
* [ ] soft: implement color multiplication and alpha combine or make black
      parts transparent for `misc_flare`,
* [ ] ReRelease: Add support of `func_eye`,
* [ ] ReRelease: Add support of `info_landmark`,
* [ ] ReRelease: Add support of `info_nav_lock`,
* [ ] ReRelease: Add support of `info_world_text`,
* [ ] ReRelease: Add support of `target_healthbar`,
* [ ] ReRelease: Add support of `target_poi`,
* [ ] ReRelease: Add support of `trigger_coop_relay`,
* [ ] ReRelease: Add support of `trigger_health_relay`,
* [ ] ReRelease: single player support,
* [ ] ReRelease: support effects and additional flags when possible,
* [ ] ReRelease: implement demo protocol based on https://github.com/res2k/q2proto
      and https://github.com/Paril/quake2-rerelease-dll,
* [ ] ReRelease: modified game code support with removed KEX only related code.

### Other games support goals:

* [ ] jabot: make count of nodes/edicts dynamic allocations,
* [ ] Heretic 2: add swim player animation support,
* [ ] Heretic 2: add utf8 colors and fonts support in console,
* [ ] Doom: implement map load logic,
* [ ] Quake 3: finish map load logic,
* [ ] Quake, Half-Life, Hexen 2: fix brush flags,
* [ ] Anachronox: load atd as sprite,
* [ ] Anachronox: fix incorrect scale of `ob_stop-flame` and `ob_wommhill01`,
* [ ] Anachronox: skins load broken with mingw win64 build,
* [ ] Anachronox: CTC entity format,
* [ ] Anachronox: support material load textures/textureinfo.dat,
* [ ] Daikatana: Fix protopod animation,
* [ ] Daikatana/SiN: Fix transparent textures in maps,
* [ ] DoD: fix statusbar `roarke`,
* [ ] Infinity: Add support of `item_radar`,
* [ ] Infinity: Add support of `monster_alienship1`,
* [ ] Infinity: Add support of `monster_grunt1v1`,
* [ ] Infinity: Add support of `monster_grunt1v2`,
* [ ] Infinity: Add support of `monster_grunt2`,
* [ ] Infinity: Add support of `monster_screamer`,
* [ ] Infinity: Add support of `weapon_pistol`,
* [ ] Infinity: Add support of `weapon_6bshot`,
* [ ] Infinity: Add support of `weapon_biggun`,
* [ ] Infinity: Add support of `weapon_blaze`,
* [ ] Infinity: Add support of `weapon_goop`,
* [ ] Infinity: Add support of `weapon_rifle`,
* [ ] Oblivion: Add support of `ammo_dod`,
* [ ] Oblivion: Add support of `ammo_mines`,
* [ ] Oblivion: Add support of `func_rotate_train`,
* [ ] Oblivion: Add support of `info_teleporter_dest`,
* [ ] Oblivion: Add support of `misc_camera`,
* [ ] Oblivion: Add support of `misc_camera_target`,
* [ ] Oblivion: Add support of `misc_screenfader`,
* [ ] Oblivion: Add support of `trigger_misc_camera`,
* [ ] Oblivion: Add support of `weapon_rtdu`,
* [ ] Oblivion: Add dynamic animation based on `activate` for `badass` and `floater`,
* [ ] Dynamic count of entities on client.

### Fixed:

* [x] Support load `.cin` by ffmpeg from `.pak` file
* [x] Support obj waveform model format for debug other formats,
* [x] vulkan: group `it_pic` images,
* [x] Heretic 2: correct obj placeholders,
* [x] windows build: restore build asset on tag,
* [x] Oblivion: Add support of `ammo_detpack`,
* [x] Oblivion: Add support of `weapon_remote_detonator`,
* [x] Oblivion: Add support of `target_rocket`,
* [x] Oblivion: Add support of `target_railgun`,
* [x] Oblivion: Add support of `monster_spider`,
* [x] Oblivion: Add support of `monster_badass`,
* [x] Oblivion: Add support of `monster_cyborg`,
* [x] Oblivion: Add support of `monster_kigrax`,
* [x] Infinity: Add support of `ammo_goop`,
* [x] Infinity: Add support of `ammo_energy`,
* [x] Oblivion: Add support of `ammo_rifleplasma`,
* [x] Half-Life: support mdl,
* [x] Oblivion: Add support of `weapon_hellfury`,
* [x] Oblivion: Add support of `weapon_plasma_pistol`,
* [x] Oblivion: Add support of `weapon_plasma_rifle`,
* [x] Oblivion: Add support of `weapon_deatomizer`,
* [x] renders: add lanczos3 2x scale by `r_scale32bittextures`,
* [x] Oblivion: Add support of `monster_soldier_deatom`,

### Not a goal:

* [ ] Multiplayer protocol support with KEX engine,
* [ ] Support KEX engine features (inventory, compass and so on),
* [ ] [KEX game library support](https://github.com/id-Software/quake2-rerelease-dll).


### Additional requirements:

* ReRelease localization requires `Q2Game.kpf` file in root directory of game. If you
  like to support your language put it to localization/loc_<your language>.txt
  and extend MAX_FONTCODE to your [max symbol code](https://en.wikipedia.org/wiki/List_of_Unicode_characters).
  Used font and language file are defined by `language` and `r_ttffont`, as
  an example could be used fonts like [unifont](https://unifoundry.com/pub/unifont/unifont-15.0.06/font-builds/unifont-15.0.06.ttf).
* Heretic 2 localization requires `levelmsg.txt` in game directory.
* Hexen 2 localization requires `Strings.txt` in game directory.

# Yamagi Quake II

Yamagi Quake II is an enhanced client for id Software's Quake II with
focus on offline and coop gameplay. Both the gameplay and the graphics
are unchanged, but many bugs in the last official release were fixed and
some nice to have features like widescreen support, reliable support for
high framerates, a modern sound backend based upon OpenAL, support for
modern game controllers and a modern OpenGL 3.2 renderer were added.
Unlike most other Quake II source ports Yamagi Quake II is fully 64-bit
clean. It works perfectly on modern processors and operating systems.

This code is built upon Icculus Quake II, which itself is based on Quake
II 3.21. Yamagi Quake II is released under the terms of the GPL version
2. See LICENSE for further information:

* [LICENSE](https://github.com/yquake2/yquake2/blob/master/LICENSE)

Officially supported operating systems are:

* FreeBSD
* Linux
* Windows

Beside theses Yamagi Quake II has community support for MacOS and most
other unixoid operating systems, including NetBSD, OpenBSD and Solaris.


## Addons and partner projects

This repository contains Yamagi Quake II itself. The official addons
have their own repositories:

* [The Reckoning](https://github.com/yquake2/xatrix)
* [Ground Zero](https://github.com/yquake2/rogue)
* [Three Waves Capture The Flag](https://github.com/yquake2/ctf)

Yamagi Quake II Remaster is a project providing optional support for the
assets of Quake II Remaster by Nightdive Studios and has a less
conservative approach in regards to new features. It also lives in it's
own repository:

* [Yamagi Quake II Remaster](https://github.com/yquake2/yquake2remaster)


## Development

Yamagi Quake II is a community driven project and lives from community
involvement. Please report bugs in our issue tracker:

* [Issue Tracker](https://github.com/yquake2/yquake2/issues)

We are always open to code contributions, no matter if they are small
bugfixes or bigger features. However, Yamagi Quake II is a conservative
project with big focus on stability and backward compatibility. We don't
accept breaking changes. When in doubt please open an issue and ask if a
contribution is welcome before putting too much work into it. Open a
pull request to submit code:

* [Pull Requests](https://github.com/yquake2/yquake2/pulls)

Also have a look at our contributors guide:

* [Contributors Guide](https://github.com/yquake2/yquake2/blob/master/doc/080_contributing.md)


## Documentation

Yamagi Quake II has rather extensive documentation covering all relevant
areas from installation and configuration to package building. Have a
look at the documentation index:

* [Documentation Index](https://github.com/yquake2/yquake2/blob/master/doc/010_index.md)


## Releases

Yamagi Quake II releases at an irregular schedule. The official releases
with source code tarballs and prebuild Windows binaries can be found at
the homepage:

* [Homepage](https://www.yamagi.org/quake2/)

Our CI builds **unsupported** Linux, MacOS and Windows binaries at every
commit. The artifacts can be found here:

* [Github Actions](https://github.com/yquake2/yquake2/actions)

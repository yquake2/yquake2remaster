# Yamagi Quake II Remaster

This is an experimental fork of Yamagi Quake II with ongoing work to add
support for Quake II Enhanced aka Q2 Remaster(ed). This enhanced version
has a lot non trivial changes, adding support isn't easy and takes time.
Feel free to try this code but you mileage may vary.

Have a look at the yquake2 repository for the "normal" Yamagi Quake II:
<https://github.com/yquake2/yquake2>

Alpha windows 64 bit [binaries](https://github.com/yquake2/yquake2remaster/releases).
Saves format is unstabled and could change between alpha releases.

State:

* Localization requires `Q2Game.kpf` file in root directory of game,
* GL1/GLES3/GL3/GL4/VK:
  * base1: no known issues,
  * base2: no known issues,
  * mguhub: sometimes broken logic for surface fall in next maps.
* SOFT:
  * base1: broken wall light and wall glitch,
  * base2: broken wall light and wall glitch,
  * q64/outpost: scale textures unsupported,
  * mguhub: broken wall light, sometimes broken logic for surface fall
     in next maps.

Monsters:

* incorrect dead animation for Arachnid,
* broken fire effect for Guardian.

Models support:

| Format | Original Game   | Frame vertex | Meshes   | Comments                                |
| ------ | --------------- | ------------ | -------- | --------------------------------------- |
| mdl    | Quake 1         | 8 bit        | Single   | Unsupported grouped textures            |
| md2    | Quake 2         | 8 bit        | Single   |                                         |
| mda    | Anachronox      | Part of md2  | Single   | No tagged surfaces                      |
| md2    | Anachronox      | 8/10/16 bit  | Single   | No tagged surfaces, unchecked with game |
| mdx    | Kingpin         | 8 bit        | Multiple | No sfx support, unchecked with game     |
| fm     | Heretic 2       | 8 bit        | Multiple |                                         |
| def    | SiN             | Part of sam  | Multiple | Unchecked with game                     |
| dkm    | Daikatana DKM1  | 8 bit        | Multiple | Unchecked with game                     |
| dkm    | Daikatana DKM2  | 10 bit       | Multiple | Unchecked with game                     |
| md3    | Quake 3         | 16 bit       | Multiple | No tags support                         |
| mdr    | EliteForce      | float        | Multiple | No tags support. Uses first LOD only    |
| md5    | Doom 3/Quake 4  | float        | Multiple | Requires md2 for skins                  |
| sbm    | SiN             | Part of sam  | Multiple | Unchecked with game                     |
| sam    | SiN             | 8 bit        | Multiple | Unchecked with game                     |

All models support only single texture for all meshes and frames limit based on game protocol.

Texture support:

| Format | Original Game  | Comments |
| ------ | -------------- | -------- |
| wal    | Quake 2        | 8 bit    |
| wal    | Daikatana      | 8 bit    |
| swl    | SiN            | 8 bit    |
| m8     | Heretic 2      | 8 bit    |
| m32    | Heretic 2      | 24 bit   |
| pcx    | Quake2         | 24 bit   |
| tga    | Quake2         | 24 bit   |
| png    | retexturing    | 24 bit   |
| jpg    | retexturing    | 24 bit   |
| bmp    | Daikatana      | 24 bit   |

Maps support:

| Format | Version | Game                                       |
| ------ | ------- | ------------------------------------------ |
| IBSP   | 39      | Quake 2 / Anachronox / Kingpin / Heretic 2 |
| IBSP   | 41      | Daikatana / SIN                            |
| RBSP   | 1       | SIN                                        |
| QBSP   | 39      | Quake 2 ReRelease                          |
| BSPX   | 39      | Quake 2 ReRelease (Extension to IBSP)      |

Note:

* Non Quake 2 maps are limited mostly view only, and could have issues
   with tranparency or some animations flags and properties.
* If you like support some other maps type, create pull request for Mod_Load2QBSP
   function and provide a link to demo maps.
* Use `maptype 1` before load any Heretic 2 maps, or place game data to `heretic2` directory.
   Look to [maptype_t](src/common/header/cmodel.h#L42) for more info.

Games:

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
* Additional maps used for check maps support:
  * PSX: <https://www.moddb.com/mods/quake-ii-psx/downloads/quake-ii-psx-10>
  * ReRelease N64 Jam: <https://www.moddb.com/games/quake-2/addons/quake-2-re-release-n64-sp-map-jam>
  * ReRelease Basic Jam: <https://www.moddb.com/games/quake-2/addons/quake-2-re-release-back-to-baseq2ics-jam-1>
  * ReRelease PSX Jam: <https://www.moddb.com/mods/psx-jam-1/downloads/quake-2-re-release-psx-jam-1>

Games check videos:

* 8.42RR11:

[![First episode](https://img.youtube.com/vi/Ha1FuVXaQSE/0.jpg)](https://www.youtube.com/watch?v=Ha1FuVXaQSE)
[![Q2DQ2](https://img.youtube.com/vi/6P3wJojExyI/0.jpg)](https://www.youtube.com/watch?v=6P3wJojExyI)
[![8.42RR11](https://img.youtube.com/vi/ukqBrx80ESM/0.jpg)](https://www.youtube.com/watch?v=ukqBrx80ESM)

* 8.42RR10:

[![8.42RR10](https://img.youtube.com/vi/obIrzYsNxBY/0.jpg)](https://www.youtube.com/watch?v=obIrzYsNxBY)

* 8.42RR9:

[![8.42RR9](https://img.youtube.com/vi/N0iHhEDkZFg/0.jpg)](https://www.youtube.com/watch?v=N0iHhEDkZFg)

* 8.42RR8:

[![8.42RR8](https://img.youtube.com/vi/NJ7T0cdyqk8/0.jpg)](https://www.youtube.com/watch?v=NJ7T0cdyqk8)

* 8.31RR7:

[![8.31RR7](https://img.youtube.com/vi/VAFs1HtQU_0/0.jpg)](https://www.youtube.com/watch?v=VAFs1HtQU_0)


Goals, fully finished goals could be checked in [here](CHANGELOG):

* [ ] CTC entity format from Anachronox,
* [ ] ATD texture format from Anachronox,
* [ ] MDA model skin selection by tag,
* [ ] SDEF/MDA dynamicaly allocate list of skins,
* [ ] Support material load textures/textureinfo.dat from Anachronox,
* [ ] Fix invisiable entities in basicsjam1_ziutek,
* [ ] Make lightmap textures dynamic n64jam_palmlix,
* [x] Support textures/*/*.mat load from ReRelease (footstep),
* [ ] Support textures/*/*.mat load from ReRelease texture effects,
* [ ] Support textures/*/*_glow.png load from ReRelease,
* [ ] Support tactile/*/*.bnvib/.wav feedback load from ReRelease,
* [ ] Fix physics with incorrect floor height in psx/base0.bsp,
* [ ] Make pmove_state_t.origin 29.3 (PS_M_ORIGIN) >4k coord values support,
* [ ] Fix statusbar for DoD `roarke`,
* [ ] Group `it_pic` images in vulkan render,
* [ ] Rearange surfaces in vulkan render before render,
* [ ] Fully implement `misc_flare`,
* [ ] Single player ReRelease support,
* [ ] Support effects and additional flags for ReRelease when possible.
* [ ] Use shared model cache in client code insted reimplemnet in each render,
* [ ] Fix transparent textures in Daikatana/SiN maps,
* [ ] Use separete texture hi-color buffer for ui in soft render,
* [ ] Cleanup function declarations in game save code,
* [ ] Fix broken base3 with sorted fields names,
* [ ] Support scalled textures for models and walls in soft render and fix
    lighting with remastered maps,
* [ ] Modified ReRelease game code support with removed KEX only related code.

Not a goal:

* [ ] Multiplayer protocol support with KEX engine,
* [ ] Support KEX engine features (inventary, compass and so on),
* [ ] [KEX game library support](https://github.com/id-Software/quake2-rerelease-dll).

Code tested with such [maps](doc/100_tested_maps.md).

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
contribution in welcome before putting too much work into it. Open a
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

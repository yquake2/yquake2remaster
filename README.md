# Yamagi Quake II Remaster

This is an experimental fork of Yamagi Quake II with ongoing work to add
support for Quake II Enhanced aka Q2 Remaster(ed). This enhanced version
has a lot non trivial changes, adding support isn't easy and takes time.
Feel free to try this code but you mileage may vary.

Have a look at the yquake2 repository for the "normal" Yamagi Quake II:
https://github.com/yquake2/yquake2

State:
 * GL1:
   * base1: correct wall light, broken model light
   * base2: correct wall light, broken model light, broken lift
   * q64/outpost: no known issies
   * mguhub: loaded, transparent walls and broken logic for surface fall in next maps
 * GL3/GLES3:
   * base1: broken wall light, broken model light
   * base2: broken wall light, broken model light, broken lift
   * q64/outpost: no known issies
   * mguhub: loaded, transparent walls and broken logic for surface fall in next maps
 * SOFT:
   * base1: broken wall light, broken model light
   * base2: broken wall light, broken model light, broken lift
   * q64/outpost: no known issies
   * mguhub: loaded, transparent walls and sky and broken logic for surface fall in next maps
 * VK:
   * base1: correct wall light, broken model light
   * base2: correct wall light, broken model light, broken lift
   * q64/outpost: no known issies
   * mguhub: loaded, transparent walls and broken logic for surface fall in next maps

Goals (none of it finished):
  * Single player support,
  * BSPX DECOUPLEDLM light map support (base1),
  * IBSQ map format support (mguhub),
  * MD5 model support,
  * modified ReRelease game code support with removed KEX only related code,
  * RoQ and Theora cinematic videos support.

Bonus goals:
  * MDL/Quake1 model format support,
  * FM/Heretic 2 model format support,
  * Daikatana model/wal/map format support,
  * Cinematic videos support in smk, mpeg format,
  * Use ffmpeg for load any video,
  * Load colormap as 24bit color,
  * Use shared model chache in client code insted reimplemnet in each render,
  * Add debug progress loading code.

Not a goal:
  * multiplayer protocol support with KEX engine,
  * support KEX engine features (inventary, compass and so on),
  * [KEX game library support](https://github.com/id-Software/quake2-rerelease-dll).

Map state:

 * ML - model light issues,
 * WL - wall light issues,
 * MD5 - md5 model load,
 * G - texture glitches,
 * B - can't load.

 | map                                 | gl1.4  | gl3/gles3 | gl4.6 | vk     | soft        |
 | ----------------------------------- | ------ | --------- | ----- | ------ | ----        |
 | maps/badlands.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/base1.bsp                      | ML/MD5 | WL/ML/MD5 | N/A   | ML/MD5 | WL/ML/MD5/G |
 | maps/base2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/base3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/base64.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/biggun.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/boss1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/boss2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/bunk1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/city1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/city2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/city3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/city64.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/command.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/cool1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/bunk_e3.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/fact_e3.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/jail4_e3.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/jail_e3.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/lab_e3.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/mine_e3.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/space_e3.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/ware1a_e3.bsp               | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/ware2_e3.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/e3/waste_e3.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/base3_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/base_ec.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/command_ec.bsp              | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/factx_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/jail_ec.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/kmdm3_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/mine1_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/power_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/space_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ec/waste_ec.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/fact1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/fact2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/fact3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/hangar1.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/hangar2.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/industry.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/jail1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/jail2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/jail3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/jail4.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/jail5.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/lab.bsp                        | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgdm1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1m4.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1m5.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu1trial.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu2m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu2m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu2m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu3m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu3m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu3m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu3m4.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu3secret.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu4m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu4m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu4m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu4trial.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu5m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu5m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu5m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu5trial.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu6m1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu6m2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu6m3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mgu6trial.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mguboss.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mguhub.bsp                     | G      | G         | B     | G      | G           |
 | maps/mine1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mine2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mine3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mine4.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/mintro.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ndctf0.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/baseold.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/city2_4.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/fact1.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/fact2.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/fact3.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/facthub.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/hangarold.bsp              | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/kmdm3.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/pjtrain1.bsp               | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/ware1.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/old/xcommand5.bsp              | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/outbase.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/power1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/power2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2ctf1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2ctf2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2ctf3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2ctf4.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2ctf5.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm3.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm4.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm5.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm6.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm7.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2dm8.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2kctf1.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q2kctf2.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/bio.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/cargo.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/command.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/comm.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/complex.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/conduits.bsp               | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/core.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm10.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm1.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm2.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm3.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm4.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm5.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm6.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm7.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm8.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/dm9.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/geo-stat.bsp               | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/intel.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/jail.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/lab.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/mines.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/orbit.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/organic.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/outpost.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/process.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/rtest.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/ship.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/station.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/q64/storage.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rammo1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rammo2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rbase1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rbase2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rboss.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm10.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm11.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm12.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm13.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm14.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm1.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm2.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm3.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm4.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm5.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm6.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm7.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm8.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rdm9.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/refinery.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rhangar1.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rhangar2.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rlava1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rlava2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rmine1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rmine2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rsewer1.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rsewer2.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rware1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/rware2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/security.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/sewer64.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/space.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/strike.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/base1_flashlight.bsp      | OK     | OK        | OK    | OK     | OK          |
 | maps/test/gekk.bsp                  | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/mals_barrier_test.bsp     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/mals_box.bsp              | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/mals_ladder_test.bsp      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/mals_locked_door_test.bsp | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_health_relay.bsp    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_ladder.bsp          | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_poi.bsp             | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_scaled_monsters.bsp | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_soundstage.bsp      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_steps.bsp           | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/paril_waterlight.bsp      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/skysort.bsp               | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/spbox2.bsp                | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/spbox.bsp                 | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/test_jerry.bsp            | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/test_kaiser.bsp           | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/test/tom_test_01.bsp           | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/train.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/tutorial.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ware1.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/ware2.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/waste1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/waste2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/waste3.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/w_treat.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xcompnd1.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xcompnd2.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm1.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm2.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm3.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm4.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm5.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm6.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xdm7.bsp                       | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xhangar1.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xhangar2.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xintell.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xmoon1.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xmoon2.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xreactor.bsp                   | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xsewer1.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xsewer2.bsp                    | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xship.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xswamp.bsp                     | N/A    | N/A       | N/A   | N/A    | N/A         |
 | maps/xware.bsp                      | N/A    | N/A       | N/A   | N/A    | N/A         |

# Yamagi Quake II


Yamagi Quake II is an enhanced client for id Software's Quake
II with focus on offline and coop gameplay. Both the gameplay and the graphics
are unchanged, but many bugs in the last official release were fixed and some
nice to have features like widescreen support and a modern OpenGL 3.2 renderer
were added. Unlike most other Quake II source ports Yamagi Quake II is fully 64-bit
clean. It works perfectly on modern processors and operating systems. Yamagi
Quake II runs on nearly all common platforms; including FreeBSD, Linux, NetBSD,
OpenBSD, Windows and macOS (experimental).

This code is built upon Icculus Quake II, which itself is based on Quake II
3.21. Yamagi Quake II is released under the terms of the GPL version 2. See the
LICENSE file for further information.

## Documentation

Before asking any question, read through the documentation! The current
version can be found here: [doc/010_index.md](doc/010_index.md)

## Releases

The official releases (including Windows binaries) can be found at our
homepage: https://www.yamagi.org/quake2
**Unsupported** preview builds for Windows can be found at
https://deponie.yamagi.org/quake2/misc/

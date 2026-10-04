# yquake2-build

Use this skill when building, testing, or compiling the Yamagi Quake II Remaster codebase to ensure correct compiler flags, debug configurations, and CMake verification are used.

## Build Commands with Make

### Debug Build with LTO and Ccache (Recommended for Development)
```bash
DEBUG=1 CC="ccache gcc -flto=auto -Wall " make -j 8
```

### Release Build with LTO and Ccache
```bash
CC="ccache gcc -flto=auto -Wall " make -j 8
```

## Build Verification with CMake
To verify that the project configuration and compilation work correctly via CMake:
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j 8
```

## Demo Playback Check

For a parser/playback check using the demos bundled in this repository, build the
debug client and capture a bounded protocol trace:

```sh
CC="ccache gcc -flto=auto -Wall " make -j2 DEBUG=1
timeout --signal=TERM 40s ./debug/quake2 -datadir path/to/quake2/assets \
	+set vid_renderer gl3 \
	+set vid_pauseonfocuslost 1 \
	+set cl_shownet 2 +demomap demo1.dm2 > /tmp/demo1.log 2>&1
```

`demomap` adds the `demos/` directory, so pass only the demo filename. Inspect
the end of `/tmp/demo1.log` for the first unsupported opcode, parse error,
or successful completion. Exit status 124 means the timeout stopped playback;
it is not by itself a parse failure. Repeat with `demo2.dm2`, `rdemo1.dm2`,
`rdemo2.dm2`, `xdemo1.dm2`, `xdemo2.dm2`, and `xdemo3.dm2` to cover the bundled
Quake 2 ReRelease demos.

## Demo Timedemo Benchmark

To measure a complete bundled demo, enable `timedemo` and set `nextdemo` to
`quit`. The client then prints the frame count and average FPS when playback
ends and exits, making the run suitable for a captured benchmark log:

```sh
CC="ccache gcc -flto=auto -Wall " make -j2 DEBUG=1
./debug/quake2 -datadir path/to/quake2/assets \
	+set vid_renderer gl3 +set vid_pauseonfocuslost 1 \
	+set timedemo 1 +set nextdemo quit \
	+demomap demo1.dm2 \
	> /tmp/demo1-timedemo.log 2>&1
```

Check the log for the final `frames, seconds: fps` result and verify playback
reached the end without a parser error. An FPS line can still be printed when a
parse error aborts playback, so that result is not a valid benchmark. `demomap`
prepends `demos/`, so provide only the filename. Repeat one demo at a time with `demo2.dm2`, `rdemo1.dm2`,
`rdemo2.dm2`, `xdemo1.dm2`, `xdemo2.dm2`, or `xdemo3.dm2`. Compare results only
under the same build, renderer, and machine conditions. For parser diagnosis
rather than timing, use the bounded trace command in Demo Playback Check; a
timeout there is expected and does not produce a complete benchmark result.

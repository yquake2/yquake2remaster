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

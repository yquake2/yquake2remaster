YQ2_OSTYPE="`uname -s`"
YQ2_ARCH="`uname -m`"
gcc --pedantic -g -DYQ2OSTYPE=\"$YQ2_OSTYPE\" -DYQ2ARCH=\"$YQ2_ARCH\" \
	src/common/md4.c \
	src/common/shared/shared.c \
	src/backends/unix/shared/hunk.c \
	mdl2obj.c -lm -o mdl2obj || exit
./mdl2obj ./tris.mdl tris_0.obj 0 || exit
./mdl2obj ./tris.mdl tris_10.obj 10 || exit

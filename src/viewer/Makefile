# Detect the OS
ifdef SystemRoot
YQ2_OSTYPE ?= Windows
else
YQ2_OSTYPE ?= $(shell uname -s)
endif

# Special case for MinGW
ifneq (,$(findstring MINGW,$(YQ2_OSTYPE)))
YQ2_OSTYPE := Windows
endif

# Detect the architecture
ifeq ($(YQ2_OSTYPE), Windows)
ifdef MINGW_CHOST
ifeq ($(MINGW_CHOST), x86_64-w64-mingw32)
YQ2_ARCH ?= x86_64
else # i686-w64-mingw32
YQ2_ARCH ?= i386
endif
else # windows, but MINGW_CHOST not defined
ifdef PROCESSOR_ARCHITEW6432
# 64 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITEW6432)
else
# 32 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITECTURE)
endif
endif # windows but MINGW_CHOST not defined
else
ifneq ($(YQ2_OSTYPE), Darwin)
# Normalize some abiguous YQ2_ARCH strings
YQ2_ARCH ?= $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/arm64/aarch64/' -e 's/^arm.*/arm/')
else
YQ2_ARCH ?= $(shell uname -m)
endif
endif

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null || echo -lSDL2)

CFLAGS = -g -Wall $(SDL_CFLAGS)
LDLIBS = -lm -lGL -lGLU -lglut $(SDL_LIBS)
COMMON_FILES = \
	../client/cl_image.c \
	../common/md4.c \
	../common/shared/shared.c \
	../common/cmodels.c \
	../common/threads.c \
	../backends/unix/shared/hunk.c \
	../client/refresh/files/mesh.c \
	../common/models/image.c \
	../common/models/loadfile.c \
	../common/models/mesh.c \
	../common/models/models.c \
	../common/models/models_utils.c \
	../common/models/models_hlmdl.c \
	../common/models/models_image.c \
	../common/models/models_mda.c \
	../common/models/models_mdl.c \
	../common/models/models_md5.c \
	../common/models/models_mdr.c \
	../common/models/models_sdef.c \
	../common/models/sprites.c \
	render.c

COMMON_HEADERS =  \
	../common/header/files.h \
	../common/header/shared.h \
	../client/refresh/files/stb_image.h \
	../client/refresh/files/stb_truetype.h \
	../client/refresh/files/stb_image_resize.h \
	render.h

all: view-quake
clean:
	rm -f *.o view-quake

view-quake: mdlx.c ${COMMON_FILES} ${COMMON_HEADERS} rendergl.c
	gcc -g -DYQ2OSTYPE=\"$(YQ2_OSTYPE)\" -DYQ2ARCH=\"$(YQ2_ARCH)\" ${CFLAGS} ${COMMON_FILES} rendergl.c mdlx.c ${LDLIBS} -o view-quake

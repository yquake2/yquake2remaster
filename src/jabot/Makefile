
BUILD_DEBUG_DIR=debugi386
BUILD_RELEASE_DIR=releasei386

# this nice line comes from the linux kernel makefile
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)

ifneq ($(ARCH),i386)
ifneq ($(ARCH),axp)
ifneq ($(ARCH),ppc)
ifneq ($(ARCH),sparc)
$(error arch $(ARCH) is currently not supported)
endif
endif
endif
endif

CC=gcc
BASE_CFLAGS=-Dstricmp=strcasecmp -I ./  
RELEASE_CFLAGS=$(BASE_CFLAGS) -ffast-math -funroll-loops \
	-fomit-frame-pointer -fexpensive-optimizations
DEBUG_CFLAGS=$(BASE_CFLAGS) -g
LDFLAGS=-ldl -lm

SHLIBEXT=so

SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

TARGETS=$(BUILDDIR)/game$(ARCH).$(SHLIBEXT) \

build_debug:
	@-mkdir $(BUILD_DEBUG_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)"

build_release:
	@-mkdir $(BUILD_RELEASE_DIR)
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS)"

all: build_debug build_release

targets: $(TARGETS)

JABOT_OBJS = \
	$(BUILDDIR)/ai_class_dmbot.o \
	$(BUILDDIR)/ai_class_monster_default.o \
	$(BUILDDIR)/ai_dropnodes.o \
	$(BUILDDIR)/ai_items.o \
	$(BUILDDIR)/ai_links.o \
	$(BUILDDIR)/ai_main.o \
	$(BUILDDIR)/ai_movement.o \
	$(BUILDDIR)/ai_navigation.o \
	$(BUILDDIR)/ai_nodes.o \
	$(BUILDDIR)/ai_tools.o \
	$(BUILDDIR)/ai_weapons.o \
	$(BUILDDIR)/AStar.o \
	$(BUILDDIR)/bot_common.o \
	$(BUILDDIR)/bot_spawn.o \
	$(BUILDDIR)/g_ai.o \
	$(BUILDDIR)/g_chase.o \
	$(BUILDDIR)/g_cmds.o \
	$(BUILDDIR)/g_combat.o \
	$(BUILDDIR)/g_ctf.o \
	$(BUILDDIR)/g_func.o \
	$(BUILDDIR)/g_items.o \
	$(BUILDDIR)/g_main.o \
	$(BUILDDIR)/g_misc.o \
	$(BUILDDIR)/g_monster.o \
	$(BUILDDIR)/g_phys.o \
	$(BUILDDIR)/g_save.o \
	$(BUILDDIR)/g_spawn.o \
	$(BUILDDIR)/g_svcmds.o \
	$(BUILDDIR)/g_target.o \
	$(BUILDDIR)/g_trigger.o \
	$(BUILDDIR)/g_utils.o \
	$(BUILDDIR)/g_weapon.o \
	$(BUILDDIR)/m_move.o \
	$(BUILDDIR)/p_client.o \
	$(BUILDDIR)/p_hud.o \
	$(BUILDDIR)/p_menu.o \
	$(BUILDDIR)/p_trail.o \
	$(BUILDDIR)/p_view.o \
	$(BUILDDIR)/p_weapon.o \
	$(BUILDDIR)/q_shared.o

$(BUILDDIR)/game$(ARCH).$(SHLIBEXT) : $(JABOT_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(JABOT_OBJS)

$(BUILDDIR)/ai_class_dmbot.o :      ai_class_dmbot.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ai_class_monster_default.o :      ai_class_monster_default.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ai_dropnodes.o :      ai_dropnodes.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_items.o :      ai_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/ai_links.o :      ai_links.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_main.o :      ai_main.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_movement.o :      ai_movement.c
	$(DO_SHLIB_CC)	
		
$(BUILDDIR)/ai_navigation.o :      ai_navigation.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_nodes.o :      ai_nodes.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_tools.o :      ai_tools.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/ai_weapons.o :      ai_weapons.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/AStar.o :      AStar.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/bot_common.o :      bot_common.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/bot_spawn.o :      bot_spawn.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/g_ai.o :         g_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_chase.o :      g_chase.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_cmds.o :       g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_combat.o :     g_combat.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/g_ctf.o :       g_ctf.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_func.o :       g_func.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_items.o :      g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_main.o :       g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_misc.o :       g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_monster.o :    g_monster.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_phys.o :       g_phys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_save.o :       g_save.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_spawn.o :      g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_svcmds.o :     g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_target.o :     g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_trigger.o :    g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_utils.o :      g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/g_weapon.o :     g_weapon.c
	$(DO_SHLIB_CC)
		
$(BUILDDIR)/m_move.o :       m_move.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/p_client.o :     p_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/p_hud.o :        p_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/p_menu.o :        p_menu.c
	$(DO_SHLIB_CC)
	
$(BUILDDIR)/p_trail.o :      p_trail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/p_view.o :       p_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/p_weapon.o :     p_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/q_shared.o :     q_shared.c
	$(DO_SHLIB_CC)

######################

clean: clean-debug clean-release

clean-debug:
	$(MAKE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)"

clean-release:
	$(MAKE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(DEBUG_CFLAGS)"

clean2:
	-rm -f $(JABOT_OBJS)


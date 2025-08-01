/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
 * Prototypes witch are shared between the client, the server and the
 * game. This is the main game API, changes here will most likely
 * requiere changes to the game ddl.
 *
 * =======================================================================
 */

#ifndef CO_COMMON_H
#define CO_COMMON_H

#include "shared.h"
#include "crc.h"

#define YQ2VERSION "8.52RR12"
#define BASEDIRNAME "baseq2"

#ifndef YQ2OSTYPE
#error YQ2OSTYPE should be defined by the build system
#endif

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

#ifdef _WIN32
 #define CFGDIR "YamagiQ2"
#else
 #ifndef __HAIKU__
   #define CFGDIR ".yq2"
 #else
   #define CFGDIR "yq2"
 #endif
#endif

#ifndef YQ2ARCH
  #ifdef _MSC_VER
    // Setting YQ2ARCH for VisualC++ from CMake doesn't work when using VS integrated CMake
    // so set it in code instead
    #ifdef YQ2ARCH
      #undef YQ2ARCH
    #endif
    #ifdef _M_X64
      // this matches AMD64 and ARM64EC (but not regular ARM64), but they're supposed to be binary-compatible somehow, so whatever
      #define YQ2ARCH "x86_64"
    #elif defined(_M_ARM64)
      #define YQ2ARCH "arm64"
    #elif defined(_M_ARM)
      #define YQ2ARCH "arm"
    #elif defined(_M_IX86)
      #define YQ2ARCH "x86"
    #else
      // if you're not targeting one of the aforementioned architectures,
      // check https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
      // to find out how to detect yours and add it here - and please send a patch :)
      #error "Unknown CPU architecture!"
      // (for a quick and dirty solution, comment out the previous line, but keep in mind
      //  that savegames may not be compatible with other builds of Yamagi Quake II)
      #define YQ2ARCH "UNKNOWN"
    #endif // _M_X64 etc
  #else // other compilers than MSVC
    #error YQ2ARCH should be defined by the build system
  #endif // _MSC_VER
#endif // YQ2ARCH

/* ================================================================== */

typedef struct sizebuf_s
{
	qboolean allowoverflow;     /* if false, do a Com_Error */
	qboolean overflowed;        /* set to true if the buffer size failed */
	byte *data;
	int maxsize;
	int cursize;
	int readcount;
} sizebuf_t;

void SZ_Init(sizebuf_t *buf, byte *data, int length);
void SZ_Clear(sizebuf_t *buf);
void *SZ_GetSpace(sizebuf_t *buf, int length);
void SZ_Write(sizebuf_t *buf, const void *data, int length);
void SZ_Print(sizebuf_t *buf, const char *data);  /* strcats onto the sizebuf */

/* ================================================================== */

struct usercmd_s;
struct entity_state_s;

size_t MSG_ConfigString_Size(const char *s);
size_t MSG_DeltaEntity_Size(const entity_xstate_t *from, const entity_xstate_t *to,
	qboolean force, qboolean newentity, int protocol);

void MSG_WriteChar(sizebuf_t *sb, int c);
void MSG_WriteByte(sizebuf_t *sb, int c);
void MSG_WriteShort(sizebuf_t *sb, int c);
void MSG_WriteLong(sizebuf_t *sb, int c);
void MSG_WriteFloat(sizebuf_t *sb, float f);
void MSG_WriteString(sizebuf_t *sb, const char *s);
void MSG_WriteCoord(sizebuf_t *sb, float f, int protocol);
void MSG_WritePos(sizebuf_t *sb, const vec3_t pos, int protocol);
void MSG_WriteAngle(sizebuf_t *sb, float f);
void MSG_WriteAngle16(sizebuf_t *sb, float f);
void MSG_WriteConfigString(sizebuf_t *sb, short index, const char *s);
void MSG_WriteDeltaUsercmd(sizebuf_t *sb, const struct usercmd_s *from,
		const struct usercmd_s *cmd);
void MSG_WriteDeltaEntity(const struct entity_xstate_s *from,
		const struct entity_xstate_s *to, sizebuf_t *msg,
		qboolean force, qboolean newentity, int protocol);
void MSG_WriteDir(sizebuf_t *sb, const vec3_t vector);

void MSG_BeginReading(sizebuf_t *sb);

int MSG_ReadChar(sizebuf_t *sb);
int MSG_ReadByte(sizebuf_t *sb);
int MSG_ReadShort(sizebuf_t *sb);
int MSG_ReadLong(sizebuf_t *sb);
float MSG_ReadFloat(sizebuf_t *sb);
char *MSG_ReadString(sizebuf_t *sb);
char *MSG_ReadStringLine(sizebuf_t *sb);

float MSG_ReadCoord(sizebuf_t *sb, int protocol);
void MSG_ReadPos(sizebuf_t *sb, vec3_t pos, int protocol);
float MSG_ReadAngle(sizebuf_t *sb);
float MSG_ReadAngle16(sizebuf_t *sb);
void MSG_ReadDeltaUsercmd(sizebuf_t *sb,
		struct usercmd_s *from,
		struct usercmd_s *cmd);

void MSG_ReadDir(sizebuf_t *sb, vec3_t vector);

void MSG_ReadData(sizebuf_t *sb, void *buffer, int size);

/* ================================================================== */

extern qboolean bigendien;

extern short BigShort(short l);
extern short LittleShort(short l);
extern int BigLong(int l);
extern int LittleLong(int l);
extern float BigFloat(float l);
extern float LittleFloat(float l);

/* ================================================================== */

int COM_Argc(void);
char *COM_Argv(int arg);    /* range and null checked */
void COM_ClearArgv(int arg);
int COM_CheckParm(char *parm);
void COM_AddParm(char *parm);

void COM_Init(void);
void COM_InitArgv(int argc, char **argv);

char *CopyString(const char *in);

/* ================================================================== */

void Info_Print(char *s);

/* PROTOCOL */

/* Quake 2 Release Demos */
#define PROTOCOL_RELEASE_VERSION 26
/* Quake 2 Demo */
#define PROTOCOL_DEMO_VERSION 31
/* Quake 2 Xatrix Demo */
#define PROTOCOL_XATRIX_VERSION 32
/* Quake 2 Network Release */
#define PROTOCOL_R97_VERSION 34
/* ReRelease demo files */
#define PROTOCOL_RR22_VERSION 2022
/* ReRelease network protocol */
#define PROTOCOL_RR23_VERSION 2023
/* Quake 2 Customized Network Release */
#define PROTOCOL_VERSION 2024

/* Quake 2 originaly uses 255 as player model */
#define QII97_PLAYER_MODEL 255

#define IS_QII97_PROTOCOL(x) ( \
	((x) == PROTOCOL_RELEASE_VERSION) || \
	((x) == PROTOCOL_DEMO_VERSION) || \
	((x) == PROTOCOL_XATRIX_VERSION) || \
	((x) == PROTOCOL_R97_VERSION))

/* ========================================= */

#define PORT_MASTER 27900
#define PORT_CLIENT 27901
#define PORT_SERVER 27910

/* ========================================= */

#define UPDATE_BACKUP 16    /* copies of entity_state_t to keep buffered */
#define UPDATE_MASK (UPDATE_BACKUP - 1)

/* server to client */
enum svc_ops_e
{
	svc_bad,

	/* these ops are known to the game dll */
	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	/* the rest are private to the client and server */
	svc_nop,
	svc_disconnect,
	svc_reconnect,
	svc_sound,                  /* <see code> */
	svc_print,                  /* [byte] id [string] null terminated string */
	svc_stufftext,              /* [string] stuffed into client's console buffer, should be \n terminated */
	svc_serverdata,             /* [long] protocol ... */
	svc_configstring,           /* [short] [string] */
	svc_spawnbaseline,
	svc_centerprint,            /* [string] to put in center of the screen */
	svc_download,               /* [short] size [size bytes] */
	svc_playerinfo,             /* variable */
	svc_packetentities,         /* [...] */
	svc_deltapacketentities,    /* [...] */
	svc_frame
};

/* ============================================== */

/* client to server */
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,               /* [[usercmd_t] */
	clc_userinfo,           /* [[userinfo string] */
	clc_stringcmd           /* [string] message */
};

/* ============================================== */

/* plyer_state_t communication */
#define PS_M_TYPE (1 << 0)
#define PS_M_ORIGIN (1 << 1)
#define PS_M_VELOCITY (1 << 2)
#define PS_M_TIME (1 << 3)
#define PS_M_FLAGS (1 << 4)
#define PS_M_GRAVITY (1 << 5)
#define PS_M_DELTA_ANGLES (1 << 6)

#define PS_VIEWOFFSET (1 << 7)
#define PS_VIEWANGLES (1 << 8)
#define PS_KICKANGLES (1 << 9)
#define PS_BLEND (1 << 10)
#define PS_FOV (1 << 11)
#define PS_WEAPONINDEX (1 << 12)
#define PS_WEAPONFRAME (1 << 13)
#define PS_RDFLAGS (1 << 14)

/*============================================== */

/* user_cmd_t communication */

/* ms and light always sent, the others are optional */
#define CM_ANGLE1 (1 << 0)
#define CM_ANGLE2 (1 << 1)
#define CM_ANGLE3 (1 << 2)
#define CM_FORWARD (1 << 3)
#define CM_SIDE (1 << 4)
#define CM_UP (1 << 5)
#define CM_BUTTONS (1 << 6)
#define CM_IMPULSE (1 << 7)

/*============================================== */

/* a sound without an ent or pos will be a local only sound */
#define SND_VOLUME (1 << 0)         /* a byte */
#define SND_ATTENUATION (1 << 1)      /* a byte */
#define SND_POS (1 << 2)            /* three coordinates */
#define SND_ENT (1 << 3)            /* a short 0-2: channel, 3-12: entity */
#define SND_OFFSET (1 << 4)         /* a byte, msec offset from frame start */

#define DEFAULT_SOUND_PACKET_VOLUME 1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

/*============================================== */

/* entity_state_t communication */

/* try to pack the common update flags into the first byte */
#define U_ORIGIN1 (1 << 0)
#define U_ORIGIN2 (1 << 1)
#define U_ANGLE2 (1 << 2)
#define U_ANGLE3 (1 << 3)
#define U_FRAME8 (1 << 4)       /* frame is a byte */
#define U_EVENT (1 << 5)
#define U_REMOVE (1 << 6)       /* REMOVE this entity, don't add it */
#define U_MOREBITS1 (1 << 7)      /* read one additional byte */

/* second byte */
#define U_NUMBER16 (1 << 8)      /* NUMBER8 is implicit if not set */
#define U_ORIGIN3 (1 << 9)
#define U_ANGLE1 (1 << 10)
#define U_MODEL (1 << 11)
#define U_RENDERFX8 (1 << 12)     /* fullbright, etc */
#define U_EFFECTS8 (1 << 14)     /* autorotate, trails, etc */
#define U_MOREBITS2 (1 << 15)     /* read one additional byte */

/* third byte */
#define U_SKIN8 (1 << 16)
#define U_FRAME16 (1 << 17)     /* frame is a short */
#define U_RENDERFX16 (1 << 18)    /* 8 + 16 = 32 */
#define U_EFFECTS16 (1 << 19)     /* 8 + 16 = 32 */
#define U_MODEL2 (1 << 20)      /* weapons, flags, etc */
#define U_MODEL3 (1 << 21)
#define U_MODEL4 (1 << 22)
#define U_MOREBITS3 (1 << 23)     /* read one additional byte */

/* fourth byte */
#define U_OLDORIGIN (1 << 24)
#define U_SKIN16 (1 << 25)
#define U_SOUND (1 << 26)
#define U_SOLID (1 << 27)

/* CMD - Command text buffering and command execution */

/*
 * Any number of commands can be added in a frame, from several different
 * sources.  Most commands come from either keybindings or console line
 * input, but remote servers can also send across commands and entire text
 * files can be execed.
 *
 * The + command line options are also added to the command buffer.
 *
 * The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute
 * ();
 */

#define EXEC_NOW 0          /* don't return until completed */
#define EXEC_INSERT 1       /* insert at current position, but don't run yet */
#define EXEC_APPEND 2       /* add to end of the command buffer */

void Cbuf_Init(void);

/* allocates an initial text buffer that will grow as needed */

void Cbuf_AddText(const char *text);

/* as new commands are generated from the console or keybindings, */
/* the text is added to the end of the command buffer. */

void Cbuf_InsertText(char *text);

/* when a command wants to issue other commands immediately, the text is */
/* inserted at the beginning of the buffer, before any remaining unexecuted */
/* commands. */

void Cbuf_ExecuteText(int exec_when, char *text);

/* this can be used in place of either Cbuf_AddText or Cbuf_InsertText */

void Cbuf_AddEarlyCommands(qboolean clear);

/* adds all the +set commands from the command line */

qboolean Cbuf_AddLateCommands(void);

/* adds all the remaining + commands from the command line */
/* Returns true if any late commands were added, which */
/* will keep the demoloop from immediately starting */

void Cbuf_Execute(void);

/* Pulls off \n terminated lines of text from the command buffer and sends */
/* them through Cmd_ExecuteString.  Stops when the buffer is empty. */
/* Normally called once per frame, but may be explicitly invoked. */
/* Do not call inside a command function! */

void Cbuf_CopyToDefer(void);
void Cbuf_InsertFromDefer(void);

/* These two functions are used to defer any pending commands while a map */
/* is being loaded */

/*=================================================================== */

/*
 * Command execution takes a null terminated string, breaks it into tokens,
 * then searches for a command or variable that matches the first token.
 */

typedef void (*xcommand_t)(void);

void Cmd_Init(void);
void Cmd_Shutdown(void);

void Cmd_AddCommand(const char *cmd_name, xcommand_t function);

/* called by the init functions of other parts of the program to */
/* register commands and functions to call for them. */
/* The cmd_name is referenced later, so it should not be in temp memory */
/* if function is NULL, the command will be forwarded to the server */
/* as a clc_stringcmd instead of executed locally */
void Cmd_RemoveCommand(const char *cmd_name);

qboolean Cmd_Exists(const char *cmd_name);

/* used by the cvar code to check for cvar / command name overlap */

const char *Cmd_CompleteCommand(const char *partial);

const char *Cmd_CompleteMapCommand(const char *partial);

/* attempts to match a partial command for automatic command line completion */
/* returns NULL if nothing fits */

int Cmd_Argc(void);
char *Cmd_Argv(int arg);
char *Cmd_Args(void);

/* The functions that execute commands get their parameters with these */
/* functions. Cmd_Argv () will return an empty string, not a NULL */
/* if arg > argc, so string operations are always safe. */

void Cmd_TokenizeString(char *text, qboolean macroExpand);

/* Takes a null terminated string.  Does not need to be /n terminated. */
/* breaks the string up into arg tokens. */

void Cmd_ExecuteString(char *text);

/* Parses a single line of text into arguments and tries to execute it */
/* as if it was typed at the console */

void Cmd_ForwardToServer(void);

/* adds the current command line as a clc_stringcmd to the client message. */
/* things like godmode, noclip, etc, are commands directed to the server, */
/* so when they are typed in at the console, they will need to be forwarded. */

/* CVAR */

/*
 * cvar_t variables are used to hold scalar or string variables that can be
 * changed or displayed at the console or prog code as well as accessed
 * directly in C code.
 *
 * The user can access cvars from the console in three ways:
 * r_draworder			prints the current value
 * r_draworder 0		sets the current value to 0
 * set r_draworder 0	as above, but creates the cvar if not present
 * Cvars are restricted from having the same names as commands to keep this
 * interface from being ambiguous.
 */

extern cvar_t *cvar_vars;

cvar_t *Cvar_Get(const char *var_name, const char *value, int flags);

/* creates the variable if it doesn't exist, or returns the existing one */
/* if it exists, the value will not be changed, but flags will be ORed in */
/* that allows variables to be unarchived without needing bitflags */

cvar_t *Cvar_Set(const char *var_name, const char *value);

/* will create the variable if it doesn't exist */

cvar_t *Cvar_ForceSet(const char *var_name, const char *value);

/* will set the variable even if NOSET or LATCH */

cvar_t *Cvar_FullSet(const char *var_name, const char *value, int flags);

void Cvar_SetValue(const char *var_name, float value);

/* expands value to a string and calls Cvar_Set */

float Cvar_VariableValue(const char *var_name);

/* returns 0 if not defined or non numeric */

const char *Cvar_VariableString(const char *var_name);

/* returns an empty string if not defined */

char *Cvar_CompleteVariable(char *partial);

/* attempts to match a partial variable name for command line completion */
/* returns NULL if nothing fits */

void Cvar_GetLatchedVars(void);

/* any CVAR_LATCHED variables that have been set will now take effect */

qboolean Cvar_Command(void);

/* called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known */
/* command.  Returns true if the command was a variable reference that */
/* was handled. (print or change) */

void Cvar_WriteVariables(const char *path);

/* appends lines containing "set variable value" for all variables */
/* with the archive flag set to true. */

void Cvar_Init(void);

void Cvar_Fini(void);

char *Cvar_Userinfo(void);

/* returns an info string containing all the CVAR_USERINFO cvars */

char *Cvar_Serverinfo(void);

/* returns an info string containing all the CVAR_SERVERINFO cvars */

extern qboolean userinfo_modified;
/* this is set each time a CVAR_USERINFO variable is changed */
/* so that the client knows to send it to the server */

/* NET */

#define PORT_ANY -1
#define MAX_MSGLEN 32768            /* max length of a message */
#define PACKET_HEADER 10            /* two ints and a short */

typedef enum
{
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX,
	NA_IP6,
	NA_MULTICAST6
} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t type;
	byte ip[16];
	unsigned int scope_id;
	byte ipx[10];

	unsigned short port;
} netadr_t;

void NET_Init(void);
void NET_Shutdown(void);

void NET_Config(qboolean multiplayer);

qboolean NET_GetPacket(netsrc_t sock, netadr_t *net_from,
		sizebuf_t *net_message);
void NET_SendPacket(netsrc_t sock, int length, void *data, netadr_t to);

qboolean NET_CompareAdr(netadr_t a, netadr_t b);
qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b);
qboolean NET_IsLocalAddress(netadr_t adr);
char *NET_AdrToString(netadr_t a);
qboolean NET_StringToAdr(const char *s, netadr_t *a);
void NET_Sleep(int msec);

/*=================================================================== */

#define OLD_AVG 0.99
#define MAX_LATENT 32

typedef struct
{
	qboolean fatal_error;

	netsrc_t sock;

	int dropped;                    /* between last packet and previous */

	int last_received;              /* for timeouts */
	int last_sent;                  /* for retransmits */

	netadr_t remote_address;
	int qport;                      /* qport value to write when transmitting */

	/* sequencing variables */
	int incoming_sequence;
	int incoming_acknowledged;
	int incoming_reliable_acknowledged;         /* single bit */

	int incoming_reliable_sequence;             /* single bit, maintained local */

	int outgoing_sequence;
	int reliable_sequence;                  /* single bit */
	int last_reliable_sequence;             /* sequence number of last send */

	/* reliable staging and holding areas */
	sizebuf_t message;          /* writing buffer to send to server */
	byte message_buf[MAX_MSGLEN - 16];          /* leave space for header */

	/* message is copied to this buffer when it is first transfered */
	int reliable_length;
	byte reliable_buf[MAX_MSGLEN - 16];         /* unacked reliable message */
} netchan_t;

extern netadr_t net_from;
extern sizebuf_t net_message;
extern byte net_message_buffer[MAX_MSGLEN];

void Netchan_Init(void);
void Netchan_Setup(netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

qboolean Netchan_NeedReliable(netchan_t *chan);
void Netchan_Transmit(netchan_t *chan, int length, byte *data);
void Netchan_OutOfBand(int net_socket, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint(int net_socket, netadr_t adr, char *format, ...);
qboolean Netchan_Process(netchan_t *chan, sizebuf_t *msg);

qboolean Netchan_CanReliable(netchan_t *chan);

/* CMODEL */

#include "files.h"

const byte* CM_GetRawMap(int *len);
cmodel_t *CM_LoadMap(const char *name, qboolean clientload, unsigned *checksum);
cmodel_t *CM_InlineModel(const char *name);       /* *1, *2, etc */
int CM_MapSurfacesNum(void);
mapsurface_t* CM_MapSurfaces(int surfnum);

void CM_ModInit(void);
void CM_ModFreeAll(void);

int CM_NumClusters(void);
int CM_NumInlineModels(void);
const char *CM_EntityString(int *size);

/* creates a clipping hull for an arbitrary box */
int CM_HeadnodeForBox(vec3_t mins, vec3_t maxs);

/* returns an ORed contents mask */
int CM_PointContents(vec3_t p, int headnode);
int CM_TransformedPointContents(vec3_t p, int headnode,
		vec3_t origin, vec3_t angles);

trace_t CM_BoxTrace(const vec3_t start, const vec3_t end, const vec3_t mins,
		const vec3_t maxs, int headnode, int brushmask);
trace_t CM_TransformedBoxTrace(const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs, int headnode,
		int brushmask, const vec3_t origin, const vec3_t angles);

byte *CM_ClusterPVS(int cluster);
byte *CM_ClusterPHS(int cluster);

int CM_PointLeafnum(vec3_t p);

/* call with topnode set to the headnode, returns with topnode */
/* set to the first node that splits the box */
int CM_BoxLeafnums(vec3_t mins, vec3_t maxs, int *list,
		int listsize, int *topnode);

int CM_LeafContents(int leafnum);
int CM_LeafCluster(int leafnum);
int CM_LeafArea(int leafnum);

void CM_SetAreaPortalState(int portalnum, qboolean open);
qboolean CM_AreasConnected(int area1, int area2);

int CM_WriteAreaBits(byte *buffer, int area);
qboolean CM_HeadnodeVisible(int headnode, byte *visbits);

void CM_WritePortalState(FILE *f);
int CM_LoadFile(const char *path, void **buffer);

/* Shared Model load code */
int Mod_LoadFile(const char *path, void **buffer);
void Mod_AliasesInit(void);
void Mod_AliasesFreeAll(void);
const dmdxframegroup_t *Mod_GetModelInfo(const char *name, int *num,
	float *mins, float *maxs);
void Mod_GetModelFrameInfo(const char *name, int num, float *mins, float *maxs);
void Mod_LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel);
byte * Mod_LoadEmbededLMP(const char *mod_name, int *width, int *height,
	int *bitsPerPixel);
/* PLAYER MOVEMENT CODE */

extern float pm_airaccelerate;

void Pmove(pmove_t *pmove);
void PmoveEx(pmove_t *pmove, int *origin);

/* FILESYSTEM */

#define SFF_INPACK 0x20

typedef int fileHandle_t;

typedef enum
{
	FS_READ,
	FS_WRITE,
	FS_APPEND
} fsMode_t;

typedef enum
{
	FS_SEEK_CUR,
	FS_SEEK_SET,
	FS_SEEK_END
} fsOrigin_t;

typedef enum
{
	FS_SEARCH_PATH_EXTENSION,
	FS_SEARCH_BY_FILTER,
	FS_SEARCH_FULL_PATH
} fsSearchType_t;

void FS_DPrintf(const char *format, ...);
int FS_FOpenFile(const char *name, fileHandle_t *f, qboolean gamedir_only);
void FS_FCloseFile(fileHandle_t f);
int FS_Read(void *buffer, int size, fileHandle_t f);
int FS_FRead(void *buffer, int size, int count, fileHandle_t f);
void CM_ReadPortalState(fileHandle_t f);
void CL_WriteConfiguration(void);

// returns the filename used to open f, but (if opened from pack) in correct case
// returns NULL if f is no valid handle
const char* FS_GetFilenameForHandle(fileHandle_t f);

char **FS_ListFiles(const char *findname, int *numfiles,
		unsigned musthave, unsigned canthave);
char **FS_ListFiles2(const char *findname, int *numfiles,
		unsigned musthave, unsigned canthave);
void FS_FreeList(char **list, int nfiles);

void FS_InitFilesystem(void);
void FS_ShutdownFilesystem(void);
void FS_BuildGameSpecificSearchPath(const char *dir);
const char *FS_Gamedir(void);
const char *FS_NextPath(const char *prevpath);
int FS_LoadFile(const char *path, void **buffer);
qboolean FS_FileInGamedir(const char *file);
qboolean FS_AddPAKFromGamedir(const char *pak);
const char* FS_GetNextRawPath(const char* lastRawPath);
char **FS_ListMods(int *nummods);

/* a null buffer will just return the file length without loading */
/* a -1 length is not present */

/* properly handles partial reads */

void FS_FreeFile(void *buffer);
void FS_CreatePath(const char *path);

/* MISC */

#define ERR_FATAL 0         /* exit the entire game with a popup window */
#define ERR_DROP 1          /* print to console and disconnect from game */
#define ERR_QUIT 2          /* not an error, just a normal exit */

#define EXEC_NOW 0          /* don't return until completed */
#define EXEC_INSERT 1       /* insert at current position, but don't run yet */
#define EXEC_APPEND 2       /* add to end of the command buffer */

#define PRINT_ALL 0
#define PRINT_DEVELOPER 1   /* only print when "developer 1" */

void Com_BeginRedirect(int target, char *buffer, int buffersize, void (*flush)(int, char *));
void Com_EndRedirect(void);
void Com_Printf(const char *fmt, ...) PRINTF_ATTR(1, 2);
void Com_DPrintf(const char *fmt, ...) PRINTF_ATTR(1, 2);
void Com_VPrintf(int print_level, const char *fmt, va_list argptr); /* print_level is PRINT_ALL or PRINT_DEVELOPER */
void Com_MDPrintf(const char *fmt, ...) PRINTF_ATTR(1, 2);
YQ2_ATTR_NORETURN_FUNCPTR void Com_Error(int code, const char *fmt, ...) PRINTF_ATTR(2, 3);
YQ2_ATTR_NORETURN void Com_Quit(void);

/* Ugly work around for unsupported
 * format specifiers unter mingw. */
#ifdef WIN32
#define YQ2_COM_PRId64 "%I64d"
#define YQ2_COM_PRIdS "%Id"
#else
#define YQ2_COM_PRId64 "%ld"
#define YQ2_COM_PRIdS "%zd"
#endif

// terminate yq2 (with Com_Error()) if VAR is NULL (after malloc() or similar)
// and print message about it
#define YQ2_COM_CHECK_OOM(VAR, ALLOC_FN_NAME, ALLOC_SIZE) \
	if(VAR == NULL) { \
		Com_Error(ERR_FATAL, "%s for " YQ2_COM_PRIdS " bytes failed in %s() (%s == NULL)! Out of Memory?!\n", \
		                     ALLOC_FN_NAME, (size_t)ALLOC_SIZE, __func__, #VAR); }

int Com_ServerState(void);              /* this should have just been a cvar... */
void Com_SetServerState(int state);

unsigned Com_BlockChecksum(const void *buffer, int length);
byte COM_BlockSequenceCRCByte(const byte *base, int length, int sequence);

extern cvar_t *developer;
extern cvar_t *modder;
extern cvar_t *dedicated;
extern cvar_t *host_speeds;
extern cvar_t *log_stats;

/* External entity files. */
extern cvar_t *sv_entfile;

/* Hack for portable client */
extern qboolean is_portable;

/* Hack for external datadir */
extern char datadir[MAX_OSPATH];

/* Hack for external datadir */
extern char cfgdir[MAX_OSPATH];

/* Hack for working 'game' cmd */
extern char userGivenGame[MAX_QPATH];
extern char **mapnames;
extern int nummaps;

extern FILE *log_stats_file;

/* host_speeds times */
extern int time_before_game;
extern int time_after_game;
extern int time_before_ref;
extern int time_after_ref;

void Z_Free(void *ptr);
void *Z_Malloc(int size);           /* returns 0 filled memory */
void *Z_TagMalloc(int size, int tag);
void Z_FreeTags(int tag);

void Qcommon_Init(int argc, char **argv);
void Qcommon_ExecConfigs(qboolean addEarlyCmds);
const char* Qcommon_GetInitialGame(void);
void Qcommon_Shutdown(void);

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

/* this is in the client code, but can be used for debugging from server */
void SCR_DebugGraph(float value, int color);

/* CLIENT / SERVER SYSTEMS */

void CL_Init(void);
void CL_Drop(void);
void CL_Shutdown(void);
void CL_Frame(int packetdelta, int renderdelta, int timedelta, qboolean packetframe, qboolean renderframe);
void Con_Print(char *text);
void SCR_BeginLoadingPlaque(void);

void SV_Init(void);
void SV_Shutdown(char *finalmsg, qboolean reconnect);
void SV_Frame(int usec);

/* Convert protocol */
int P_ConvertConfigStringFrom(int i, int protocol);
int P_ConvertConfigStringTo(int i, int protocol);

/* ======================================================================= */

// Platform specific functions.

// system.c
char *Sys_ConsoleInput(void);
void Sys_ConsoleOutput(char *string);
YQ2_ATTR_NORETURN void Sys_Error(const char *error, ...);
YQ2_ATTR_NORETURN void Sys_Quit(void);
void Sys_Init(void);
char *Sys_GetHomeDir(void);
void Sys_Remove(const char *path);
int Sys_Rename(const char *from, const char *to);
void Sys_RemoveDir(const char *path);
long long Sys_Microseconds(void);
void Sys_Nanosleep(int);
void *Sys_GetProcAddress(void *handle, const char *sym);
void Sys_FreeLibrary(void *handle);
void *Sys_LoadLibrary(const char *path, const char *sym, void **handle);
void *Sys_GetGameAPI(void *parms);
void Sys_UnloadGame(void);
void Sys_GetWorkDir(char *buffer, size_t len);
qboolean Sys_SetWorkDir(char *path);
qboolean Sys_Realpath(const char *in, char *out, size_t size);

// Windows only (system.c)
#ifdef _WIN32
void Sys_RedirectStdout(void);
void Sys_SetHighDPIMode(void);
#endif

// misc.c
const char *Sys_GetBinaryDir(void);
void Sys_SetupFPU(void);

/* ======================================================================= */

void Mods_NamesFinish(void);

#endif

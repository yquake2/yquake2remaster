// Need to shunt all particle textures in 0.5 of a pixel to avoid texture bleed

#define FRAC		(1.0F / 256.0F)
#define	UNIT		(1.0F / 128.0F)

#define	PRT_LT00	((0.0F * UNIT) + FRAC)
#define	PRT_LT04	((4.0F * UNIT) + FRAC)
#define	PRT_LT08	((8.0F * UNIT) + FRAC)
#define	PRT_LT12	((12.0F * UNIT) + FRAC)
#define	PRT_LT16	((16.0F * UNIT) + FRAC)
#define	PRT_LT24	((24.0F * UNIT) + FRAC)
#define	PRT_LT32	((32.0F * UNIT) + FRAC)
#define	PRT_LT40	((40.0F * UNIT) + FRAC)
#define	PRT_LT48	((48.0F * UNIT) + FRAC)
#define	PRT_LT56	((56.0F * UNIT) + FRAC)
#define PRT_LT64	((64.0F * UNIT) + FRAC)
#define PRT_LT72	((72.0F * UNIT) + FRAC)
#define	PRT_LT80	((80.0F * UNIT) + FRAC)
#define	PRT_LT88	((88.0F * UNIT) + FRAC)
#define PRT_LT96	((96.0F * UNIT) + FRAC)
#define PRT_LT112	((112.0F * UNIT) + FRAC)

#define	PRT_RB04	((4.0F * UNIT) - FRAC)
#define	PRT_RB08	((8.0F * UNIT) - FRAC)
#define	PRT_RB12	((12.0F * UNIT) - FRAC)
#define	PRT_RB16	((16.0F * UNIT) - FRAC)
#define	PRT_RB24	((24.0F * UNIT) - FRAC)
#define	PRT_RB32	((32.0F * UNIT) - FRAC)
#define	PRT_RB40	((40.0F * UNIT) - FRAC)
#define	PRT_RB48	((48.0F * UNIT) - FRAC)
#define PRT_RB56	((56.0F * UNIT) - FRAC)
#define	PRT_RB64	((64.0F * UNIT) - FRAC)
#define	PRT_RB72	((72.0F * UNIT) - FRAC)
#define	PRT_RB80	((80.0F * UNIT) - FRAC)
#define	PRT_RB88	((88.0F * UNIT) - FRAC)
#define PRT_RB96	((96.0F * UNIT) - FRAC)
#define PRT_RB112	((112.0F * UNIT) - FRAC)
#define PRT_RB128	((128.0F * UNIT) - FRAC)

typedef struct tex_coords_s
{
	float		lx;
	float		ty;
	float		rx;
	float		by;
}	tex_coords_t;

// Note:  Coordinates are described in 16x16 cells because there are the most common
static tex_coords_t part_TexCoords[NUM_PARTICLE_TYPES] =
{
	// Small 4x4 particles in the upper left (0,0)
	PRT_LT00, PRT_LT00, PRT_RB04, PRT_RB04,				//	PART_4x4_WHITE,
	PRT_LT04, PRT_LT00, PRT_RB08, PRT_RB04,				//	PART_4x4_BLUE,
	PRT_LT08, PRT_LT00, PRT_RB12, PRT_RB04,				//	PART_4x4_RED,
	PRT_LT12, PRT_LT00, PRT_RB16, PRT_RB04,				//	PART_4x4_GREEN,

	PRT_LT00, PRT_LT04, PRT_RB04, PRT_RB08,				//	PART_4x4_CYAN,
	PRT_LT04, PRT_LT04, PRT_RB08, PRT_RB08,				//	PART_4x4_YELLOW,
	PRT_LT08, PRT_LT04, PRT_RB12, PRT_RB08,				//	PART_4x4_MAGENTA,
	PRT_LT12, PRT_LT04, PRT_RB16, PRT_RB08,				//	PART_4x4_ORANGE,

	PRT_LT00, PRT_LT08, PRT_RB04, PRT_RB12,				//	PART_4x4_BLUE2,
	PRT_LT04, PRT_LT08, PRT_RB08, PRT_RB12,				//	PART_4x4_BLUE3,
	PRT_LT08, PRT_LT08, PRT_RB12, PRT_RB12,				//	PART_4x4_UNUSED1,
	PRT_LT12, PRT_LT08, PRT_RB16, PRT_RB12,				//	PART_4x4_UNUSED2,

	PRT_LT00, PRT_LT12, PRT_RB04, PRT_RB16,				//	PART_4x4_BLOOD1,
	PRT_LT04, PRT_LT12, PRT_RB08, PRT_RB16,				//	PART_4x4_BLOOD2,
	PRT_LT08, PRT_LT12, PRT_RB12, PRT_RB16,				//	PART_4x4_GREENBLOOD1,
	PRT_LT12, PRT_LT12, PRT_RB16, PRT_RB16,				//	PART_4x4_GREENBLOOD2,

	// Unused 16x16 section, to right of above 16x16 section (1,0)
  	PRT_LT16, PRT_LT00, PRT_RB24, PRT_RB08,				//	PART_8x8_BUBBLE,
	PRT_LT24, PRT_LT00, PRT_RB32, PRT_RB08,				//	PART_8x8_BLOOD,
	PRT_LT16, PRT_LT08, PRT_RB24, PRT_RB16,				//	PART_8x8_GLOBBIT1,
	PRT_LT24, PRT_LT08, PRT_RB32, PRT_RB16,				//	PART_8x8_GLOBBIT2,

	// Two sections, directly below the above two sections (0,1) and (1,1)
	PRT_LT00, PRT_LT16, PRT_RB16, PRT_RB32,				//	PART_16x16_MIST,
	PRT_LT16, PRT_LT16, PRT_RB32, PRT_RB32,				//	PART_16x16_GLOB,

	// Next 32x32 section, to the left of the above 32x32 section (2,0), (3,0), (2,1) and (3,1)
	PRT_LT32, PRT_LT00, PRT_RB48, PRT_RB16,				//	PART_16x16_STAR,
	PRT_LT48, PRT_LT00,	PRT_RB64, PRT_RB16,				//	PART_16x16_WATERDROP,
	PRT_LT32, PRT_LT16,	PRT_RB48, PRT_RB32,				//	PART_16x16_LIGHTNING,
	PRT_LT48, PRT_LT16,	PRT_RB64, PRT_RB32,				//	PART_16x16_BLOOD,

	// Two 32x32 sections, below the above two 32x32 sections (0,2) and (2,2)
	PRT_LT00, PRT_LT32,	PRT_RB32, PRT_RB64,				//	PART_32x32_STEAM,
	PRT_LT32, PRT_LT32, PRT_RB64, PRT_RB64,				//	PART_32x32_WFALL,

	// Three 32x32 sections, in the upper right 1/4 of the page (4,0) (6,0) (4,2)
	PRT_LT64, PRT_LT00,	PRT_RB96,  PRT_RB32,			//	PART_32x32_FIRE0,
	PRT_LT96, PRT_LT00,	PRT_RB128, PRT_RB32,			//	PART_32x32_FIRE1,
	PRT_LT64, PRT_LT32,	PRT_RB96,  PRT_RB64,			//	PART_32x32_FIRE2,

	// The single remaining 32x32 section, in the lower right of the above one (6,2)
	PRT_LT96,	PRT_LT32,	PRT_RB112, PRT_RB48,		//	PART_16x16_SPARK_B,
	PRT_LT112,	PRT_LT32,	PRT_RB128, PRT_RB48,		//	PART_16x16_SPARK_R,
	PRT_LT96,	PRT_LT48,	PRT_RB112, PRT_RB64,		//	PART_16x16_SPARK_G,
	PRT_LT112,	PRT_LT48,	PRT_RB128, PRT_RB64,		//	PART_16x16_SPARK_Y,

	// The third 32x32 section down, on the left (0,4)
	PRT_LT00, PRT_LT64,	PRT_RB32,  PRT_RB96,			//	PART_32x32_FIREBALL,     // The same spot,
	PRT_LT00, PRT_LT64,	PRT_RB32,  PRT_RB96,			//	PART_32x32_BLACKSMOKE,   // the fireball is additive, the smoke is not.

	// The next 32x32 section to the right of the above cell (2,4), (3,4), (2,5) and (3,5)
	PRT_LT32, PRT_LT64,	PRT_RB48,  PRT_RB80,			//	PART_16x16_SPARK_C
	// These are little 8x8 critters
	PRT_LT48, PRT_LT64,	PRT_RB56,  PRT_RB72,			//	PART_8x8_RED_X,
	PRT_LT56, PRT_LT64,	PRT_RB64,  PRT_RB72,			//	PART_8x8_RED_CIRCLE
	PRT_LT48, PRT_LT72,	PRT_RB56,  PRT_RB80,			//	PART_8x8_GREEN_X
	PRT_LT56, PRT_LT72,	PRT_RB64,  PRT_RB80,			//	PART_8x8_GREEN_CIRCLE

	PRT_LT32, PRT_LT80,	PRT_RB40,  PRT_RB88,			//	PART_8x8_BLUE_X
	PRT_LT40, PRT_LT80,	PRT_RB48,  PRT_RB88,			//	PART_8x8_BLUE_CIRCLE
	PRT_LT32, PRT_LT88,	PRT_RB40,  PRT_RB96,			//	PART_8x8_CYAN_X
	PRT_LT40, PRT_LT88,	PRT_RB48,  PRT_RB96,			//	PART_8x8_CYAN_CIRCLE

	PRT_LT48, PRT_LT80,	PRT_RB56,  PRT_RB88,			//	PART_8x8_RED_DIAMOND,
	PRT_LT56, PRT_LT80,	PRT_RB64,  PRT_RB88,			//	PART_8x8_GREEN_DIAMOND,
	PRT_LT48, PRT_LT88,	PRT_RB56,  PRT_RB96,			//	PART_8x8_BLUE_DIAMOND,
	PRT_LT56, PRT_LT88,	PRT_RB64,  PRT_RB96,			//	PART_8x8_CYAN_DIAMOND,

	// Bottom two 32x32 sections, lower left (0,6) and (2,6)
	PRT_LT00, PRT_LT96,	PRT_RB32,  PRT_RB128,			//	PART_32x32_BUBBLE,
	PRT_LT32, PRT_LT96,	PRT_RB64,  PRT_RB128,			//	PART_32x32_ALPHA_GLOBE,

	// Upper left of lower right quadrant (4,4)
	PRT_LT64, PRT_LT64,	PRT_RB80,  PRT_RB80, 			//	PART_16x16_SPARK_I
	PRT_LT80, PRT_LT64,	PRT_RB96,  PRT_RB80, 			//	PART_16x16_BLUE_PUFF
	PRT_LT64, PRT_LT80,	PRT_RB80,  PRT_RB96, 			//	PART_16x16_UNUSED
	PRT_LT80, PRT_LT80,	PRT_RB96,  PRT_RB96, 			//	PART_16x16_ORANGE_PUFF

	// Remaining two 32x32's in lower right quadrant (6,4) & (4,6)
	PRT_LT96, PRT_LT64,	PRT_RB128, PRT_RB96,			//	PART_32x32_BLOOD
	PRT_LT64, PRT_LT96,	PRT_RB96,  PRT_RB128, 			//	PART_32x32_GREENBLOOD

	// Remaining four 16x16's in lower right quadrant (6,6), (6, 7), (7, 6) and (7,7)
	PRT_LT96,  PRT_LT96, PRT_RB112, PRT_RB112,			//	PART_16x16_FIRE1,
	PRT_LT112, PRT_LT96, PRT_RB128, PRT_RB112,			//	PART_16x16_FIRE2,
	PRT_LT96,  PRT_LT112, PRT_RB112, PRT_RB128,			//	PART_16x16_FIRE3,
	PRT_LT112, PRT_LT112, PRT_RB128, PRT_RB128			//	PART_16x16_GREENBLOOD,
};

// end

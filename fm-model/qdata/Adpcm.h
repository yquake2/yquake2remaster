/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
**
** Modded 10/3/98
** John Scott
*/

typedef struct adpcm_state_s
{
    short	in_valprev;			// Previous output value
    short	in_index;	 		// Index into stepsize table
    short	out_valprev; 		// Previous output value
    short	out_index;			// Index into stepsize table
	int		count;				// Number of sample counts
}	adpcm_state_t;

typedef struct adpcm_s
{
	adpcm_state_t	state;
	char			adpcm[0x10000];
}	adpcm_t;

void adpcm_coder(short [], adpcm_t *);
void adpcm_decoder(adpcm_t *, short []);

// end

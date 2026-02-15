/*-
 * Copyright (c) 2012-2016 Yamagi Burmeister
 *               2015-2017 Daniel Gibson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h> // chdir()
#include <sys/stat.h> // mkdir()


#include <assert.h>


enum {
	PAK_MODE_Q2, // standard Quake/Quake2 pak
	PAK_MODE_SIN,
	PAK_MODE_DK,

	_NUM_PAK_MODES
};

static int pak_mode = PAK_MODE_Q2;

static const char* PAK_MODE_NAMES[_NUM_PAK_MODES] = {
	"Quake(2)",  // PAK_MODE_Q2
	"Sin",       // PAK_MODE_SIN
	"Daikatana", // PAK_MODE_DK
};

static const int HDR_LEN[_NUM_PAK_MODES] = {
	64,  // PAK_MODE_Q2
	128, // PAK_MODE_SIN
	72,  // PAK_MODE_DK
};

static const int DIR_FILENAME_LEN[_NUM_PAK_MODES] = {
	56,  // PAK_MODE_Q2
	120, // PAK_MODE_SIN
	56,  // PAK_MODE_DK
};

/* Holds the pak header */
struct
{
	char signature[4];
	int dir_offset;
	int dir_length;
} header;


/* A directory entry */
typedef struct
{
	char file_name[120];
	int file_pos;
	int file_length; // in case of is_compressed: size after decompression

	// the following two are only used by daikatana
	int compressed_length; // size of compressed data in the pak
	int is_compressed; // 0: uncompressed, else compressed

} directory;

/*
 * Creates a directory tree.
 *
 *  *s -> The path to create. The last
 *        part (the file itself) is
 *        ommitted
 */
static void
mktree(const char *s)
{
	char dir[128] = {0};
	int i;
	int sLen = strlen(s);

	strncpy(dir, s, sizeof(dir)-1);

	for(i=1; i<sLen; ++i)
	{
		if(dir[i] == '/')
		{
			dir[i] = '\0'; // this ends the string at this point and allows creating the directory up to here

#if _WIN32
			mkdir(dir);
#else
			mkdir(dir, 0700);
#endif

			dir[i] = '/'; // restore the / so we can can go on to create next directory
		}
	}
}

/*
 * Reads the pak file header and
 * stores it into the global struct
 * "header".
 *
 *  *fd -> A file descriptor holding
 *         the pack to be read.
 */
static int
read_header(FILE *fd)
{
	if (fread(header.signature, 4, 1, fd) != 1)
	{
		perror("Could not read the pak file header");
		return 0;
	}

	if (fread(&header.dir_offset, 4, 1, fd) != 1)
	{
		perror("Could not read the pak file header");
		return 0;
	}

	if (fread(&header.dir_length, 4, 1, fd) != 1)
	{
		perror("Could not read the pak file header");
		return 0;
	}

	// TODO: we could convert the ints to platform endianess now

	if (strncmp(header.signature, "SPAK", 4) == 0)
	{
		pak_mode = PAK_MODE_SIN;
	}
	else if (strncmp(header.signature, "PACK", 4) != 0)
	{
		fprintf(stderr, "Not a pak file\n");
		return 0;
	}

	int direntry_len = HDR_LEN[pak_mode];

	// Note that this check is not reliable, it could pass and it could still be the wrong kind of pak!
	if ((header.dir_length % direntry_len) != 0)
	{
		const char* curmode = PAK_MODE_NAMES[pak_mode];
		const char* othermode = (pak_mode != PAK_MODE_DK) ? "Daikatana" : "Quake(2)";
		fprintf(stderr, "Corrupt pak file - maybe it's not %s format but %s format?\n", curmode, othermode);
		if(pak_mode != PAK_MODE_DK)
			fprintf(stderr, "If this is a Daikatana .pak file, try adding '-dk' to command-line!\n");
		else
			fprintf(stderr, "Are you sure this is a Daikatana .pak file? Try removing '-dk' from command-line!\n");

		return 0;
	}

	return 1;
}

static int
read_dir_entry(directory* entry, FILE* fd)
{
	if(fread(entry->file_name, DIR_FILENAME_LEN[pak_mode], 1, fd) != 1) return 0;
	if(fread(&(entry->file_pos), 4, 1, fd) != 1) return 0;
	if(fread(&(entry->file_length), 4, 1, fd) != 1) return 0;

	if(pak_mode == PAK_MODE_DK)
	{
		if(fread(&(entry->compressed_length), 4, 1, fd) != 1) return 0;
		if(fread(&(entry->is_compressed), 4, 1, fd) != 1) return 0;
	}
	else
	{
		entry->compressed_length = 0;
		entry->is_compressed = 0;
	}

	// TODO: we could convert the ints to platform endianess now

	return 1;
}

/*
 * Reads the directory of a pak file
 * into a linked list and returns
 * a pointer to the first element.
 *
 *  *fd -> a file descriptor holding
 *         holding the pak to be read
 */
static directory *
read_directory(FILE *fd, int listOnly, int* num_entries)
{
	int i;
	int direntry_len = HDR_LEN[pak_mode];
	int num_dir_entries = header.dir_length / direntry_len;
	directory* dir = calloc(num_dir_entries, sizeof(directory));

	if(dir == NULL)
	{
		perror("Couldn't allocate memory");
		return NULL;
	}

	/* Navigate to the directory */
	fseek(fd, header.dir_offset, SEEK_SET);

	for (i = 0; i < num_dir_entries; ++i)
	{
		directory* cur = &dir[i];

		if (!read_dir_entry(cur, fd))
		{
			perror("Could not read directory entry");
			*num_entries = 0;
			free(dir);
			return NULL;
		}

		if(listOnly)
		{
			printf("%s (%d bytes", cur->file_name, cur->file_length);

			if((pak_mode == PAK_MODE_DK) && cur->is_compressed)
				printf(", %d compressed", cur->compressed_length);

			printf(")\n");
		}
	}

	*num_entries = num_dir_entries;
	return dir;
}

static void
extract_compressed(FILE* in, directory *d)
{
	FILE *out;

	if ((out = fopen(d->file_name, "w")) == NULL)
	{
		perror("Couldn't open outputfile");
		return;
	}

	unsigned char *in_buf;

	if ((in_buf = malloc(d->compressed_length)) == NULL)
	{
		perror("Couldn't allocate memory");
		return;
	}

	unsigned char *out_buf;

	if ((out_buf = calloc(1, d->file_length)) == NULL)
	{
		perror("Couldn't allocate memory");
		return;
	}

	fseek(in, d->file_pos, SEEK_SET);
	fread(in_buf, d->compressed_length, 1, in);

	int read = 0;
	int written = 0;

	while (read < d->compressed_length)
	{
		unsigned char x = in_buf[read];
		++read;

		// x + 1 bytes of uncompressed data
		if (x < 64)
		{
			memmove(out_buf + written, in_buf + read, x + 1);

			read += x + 1;
			written += x + 1;
		}
		// x - 62 zeros
		else if (x < 128)
		{
			memset(out_buf + written, 0, x - 62);

			written += x - 62;
		}
		// x - 126 times the next byte
		else if (x < 192)
		{
			memset(out_buf + written, in_buf[read], x - 126);

			++read;
			written += x - 126;
		}
		// Reference previously uncompressed data
		else if (x < 254)
		{
			memmove(out_buf + written, (out_buf + written) - ((int)in_buf[read] + 2), x - 190);

			++read;
			written += x - 190;
		}
		// Terminate
		else if (x == 255)
		{
			break;
		}
	}

	fwrite(out_buf, d->file_length, 1, out);
	fclose(out);

	free(in_buf);
	free(out_buf);
}

static void
extract_raw(FILE* in, directory *d)
{
	FILE* out = fopen(d->file_name, "w");
	if (out == NULL)
	{
		perror("Could open the outputfile");
		return;
	}

	// just copy the data from the .pak to the output file (in chunks for speed)
	int bytes_left = d->file_length;
	char buf[2048];

	fseek(in, d->file_pos, SEEK_SET);

	while(bytes_left >= sizeof(buf))
	{
		fread(buf, sizeof(buf), 1, in);
		fwrite(buf, sizeof(buf), 1, out);
		bytes_left -= sizeof(buf);
	}
	if(bytes_left > 0)
	{
		fread(buf, bytes_left, 1, in);
		fwrite(buf, bytes_left, 1, out);
	}

	fclose(out);
}

/*
 * Extract the files from a pak.
 *
 *  *d -> a pointer to the first element
 *        of the pak directory
 *
 *  *fd -> a file descriptor holding
 *         the pak to be extracted
 */
static void
extract_files(FILE *fd, directory *dirs, int num_entries)
{
	int i;

	for(i=0; i<num_entries; ++i)
	{
		directory* d = &dirs[i];
	    mktree(d->file_name);

		if(d->is_compressed)
		{
			assert((pak_mode == PAK_MODE_DK) && "Only Daikatana paks contain compressed files!");
			extract_compressed(fd, d);
		}
		else
		{
			extract_raw(fd, d);
		}
	}
}

static void
printUsage(const char* argv0)
{

	fprintf(stderr, "Extractor for Quake/Quake2 (and compatible) and Daikatana .pak and Sin .sin files\n");

	fprintf(stderr, "Usage: %s [-l] [-dk] [-o output dir] pakfile\n", argv0);
	fprintf(stderr, "       -l     don't extract, just list contents\n");
	fprintf(stderr, "       -dk    Daikatana pak format (Quake is default, Sin is detected automatically)\n");
	fprintf(stderr, "       -o     directory to extract to\n");
}

int
main(int argc, char *argv[])
{
	directory *d = NULL;
	FILE *fd = NULL;
	const char* filename = NULL;
	int list_only = 0;
	int i = 0;
	int num_entries = 0;

	/* Correct usage? */
	if (argc < 2)
	{
		printUsage(argv[0]);
		exit(-1);
	}

	const char* out_dir = NULL;

	for(i=1; i<argc; ++i)
	{
		const char* arg = argv[i];
		if(strcmp(arg, "-l") == 0) list_only = 1;
		else if(strcmp(arg, "-dk") == 0) pak_mode = PAK_MODE_DK;
		else if(strcmp(arg, "-o") == 0)
		{
			++i; // go to next argument (should be out_dir)
			if(i == argc || argv[i][0] == '-') // no further argument/next argument option?
			{
				fprintf(stderr, "!! -o must be followed by output dir !!\n");
				printUsage(argv[0]);
				exit(-1);
			}
			out_dir = argv[i];
		}
		else
		{
			if(filename != NULL) // we already set a filename, wtf
			{
				fprintf(stderr, "!! Illegal argument '%s' (or too many filenames) !!\n", arg);
				printUsage(argv[0]);
				exit(-1);
			}
			filename = arg;
		}
	}

	if (filename == NULL)
	{
		fprintf(stderr, "!! No filename given !!\n");
		printUsage(argv[0]);
		exit(-1);
	}

	/* Open the pak file */
	fd = fopen(filename, "r");
	if (fd == NULL)
	{
		fprintf(stderr, "Could not open the pak file '%s': %s\n", filename, strerror(errno));
		exit(-1);
	}

	/* Read the header */
    if (!read_header(fd))
	{
		fclose(fd);
		exit(-1);
	}

	/* Read the directory */
	d = read_directory(fd, list_only, &num_entries);
	if (d == NULL)
	{
		fclose(fd);
		exit(-1);
	}

	if (out_dir != NULL)
	{
		if (chdir(out_dir) != 0)
		{
			perror("Could not cd to output dir");
			exit(-1);
		}
	}

	if (!list_only)
	{
		/* And now extract the files */
		extract_files(fd, d, num_entries);
	}

	/* cleanup */
	fclose(fd);

	free(d);

	return 0;
}


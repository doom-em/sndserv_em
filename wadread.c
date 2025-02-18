// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wadread.c,v 1.3 1997/01/30 19:54:23 b1 Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// $Log: wadread.c,v $
// Revision 1.3  1997/01/30 19:54:23  b1
// Final reformatting run. All the remains (ST, W, WI, Z).
//
// Revision 1.2  1997/01/21 19:00:10  b1
// First formatting run:
//  using Emacs cc-mode.el indentation for C++ now.
//
// Revision 1.1  1997/01/19 17:22:51  b1
// Initial check in DOOM sources as of Jan. 10th, 1997
//
//
// DESCRIPTION:
//	WAD and Lump I/O, the second.
//	This time for soundserver only.
//	Welcome to Department of Redundancy Department. Again :-).
//
//-----------------------------------------------------------------------------


static const char rcsid[] = "$Id: wadread.c,v 1.3 1997/01/30 19:54:23 b1 Exp $";



#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "soundsrv.h"
#include "wadread.h"


int*		sfxlengths;

typedef struct wadinfo_struct
{
    char	identification[4];		                 
    int		SNDSERV_numlumps;
    int		infotableofs;

} wadinfo_t;

typedef struct filelump_struct
{
    int		filepos;
    int		size;
    char	name[8];

} filelump_t;

typedef struct lumpinfo_struct
{
    int		handle;
    int		filepos;
    int		size;
    char	name[8];

} lumpinfo_t;



lumpinfo_t*	SNDSERV_lumpinfo;		                                
int		SNDSERV_numlumps;

void**		SNDSERV_lumpcache;


#define strcmpi strcasecmp


//
// Something new.
// This version of w_wad.c does handle endianess.
//
#ifndef __BIG_ENDIAN__

#define LONG(x) (x)
#define SHORT(x) (x)

#else

#define LONG(x) ((long)SNDSERV_SwapLONG((unsigned long) (x)))
#define SHORT(x) ((short)SNDSERV_SwapSHORT((unsigned short) (x)))

unsigned long SNDSERV_SwapLONG(unsigned long x)
{
    return
	(x>>24)
	| ((x>>8) & 0xff00)
	| ((x<<8) & 0xff0000)
	| (x<<24);
}

unsigned short SNDSERV_SwapSHORT(unsigned short x)
{
    return
	(x>>8) | (x<<8);
}

#endif



// Way too many of those...
static void SNDSERV_derror(char* msg)
{
    fprintf(stderr, "\nwadread error: %s\n", msg);
    exit(-1);
}

#ifndef __EMSCRIPTEN__
void strupr (char *s)
{
    while (*s)
	*s++ = toupper(*s);
}
#endif
// why lol


/* int filelength (int handle)
{
    struct stat	fileinfo;
  
    if (fstat (handle,&fileinfo) == -1)
	fprintf (stderr, "Error fstating\n");

    return fileinfo.st_size;
}*/



void SNDSERV_openwad(char* wadname)
{

    int		wadfile;
    int		tableoffset;
    int		tablelength;
    int		tablefilelength;
    int		i;
    wadinfo_t	header;
    filelump_t*	filetable;

    // open and read the wadfile header
    wadfile = open(wadname, O_RDONLY);

    if (wadfile < 0)
	SNDSERV_derror("Could not open wadfile");

    read(wadfile, &header, sizeof header);

    if (strncmp(header.identification, "IWAD", 4))
	SNDSERV_derror("wadfile has weirdo header");

    SNDSERV_numlumps = LONG(header.SNDSERV_numlumps);
    tableoffset = LONG(header.infotableofs);
    tablelength = SNDSERV_numlumps * sizeof(lumpinfo_t);
    tablefilelength = SNDSERV_numlumps * sizeof(filelump_t);
    SNDSERV_lumpinfo = (lumpinfo_t *) malloc(tablelength);
    filetable = (filelump_t *) ((char*)SNDSERV_lumpinfo + tablelength - tablefilelength);

    // get the SNDSERV_lumpinfo table
    lseek(wadfile, tableoffset, SEEK_SET);
    read(wadfile, filetable, tablefilelength);

    // process the table to make the endianness right and shift it down
    for (i=0 ; i<SNDSERV_numlumps ; i++)
    {
	strncpy(SNDSERV_lumpinfo[i].name, filetable[i].name, 8);
	SNDSERV_lumpinfo[i].handle = wadfile;
	SNDSERV_lumpinfo[i].filepos = LONG(filetable[i].filepos);
	SNDSERV_lumpinfo[i].size = LONG(filetable[i].size);
	// fprintf(stderr, "lump [%.8s] exists\n", SNDSERV_lumpinfo[i].name);
    }

}

void*
SNDSERV_loadlump
( char*		lumpname,
  int*		size )
{

    int		i;
    void*	lump;

    for (i=0 ; i<SNDSERV_numlumps ; i++)
    {
	if (!strncasecmp(SNDSERV_lumpinfo[i].name, lumpname, 8))
	    break;
    }

    if (i == SNDSERV_numlumps)
    {
	// fprintf(stderr,
	//   "Could not find lumpname [%s]\n", lumpname);
	lump = 0;
    }
    else
    {
	lump = (void *) malloc(SNDSERV_lumpinfo[i].size);
	lseek(SNDSERV_lumpinfo[i].handle, SNDSERV_lumpinfo[i].filepos, SEEK_SET);
	read(SNDSERV_lumpinfo[i].handle, lump, SNDSERV_lumpinfo[i].size);
	*size = SNDSERV_lumpinfo[i].size;
    }

    return lump;

}

void*
SNDSERV_getsfx
( char*		sfxname,
  int*		len )
{

    unsigned char*	sfx;
    unsigned char*	paddedsfx;
    int			i;
    int			size;
    int			paddedsize;
    char		name[20];

    sprintf(name, "ds%s", sfxname);

    sfx = (unsigned char *) SNDSERV_loadlump(name, &size);

    // pad the sound effect out to the mixing buffer size
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;
    paddedsfx = (unsigned char *) realloc(sfx, paddedsize+8);
    for (i=size ; i<paddedsize+8 ; i++)
	paddedsfx[i] = 128;

    *len = paddedsize;
    return (void *) (paddedsfx + 8);

}

// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: soundsrv.c,v 1.3 1997/01/29 22:40:44 b1 Exp $
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
// $Log: soundsrv.c,v $
// Revision 1.3  1997/01/29 22:40:44  b1
// Reformatting, S (sound) module files.
//
// Revision 1.2  1997/01/21 19:00:07  b1
// First formatting run:
//  using Emacs cc-mode.el indentation for C++ now.
//
// Revision 1.1  1997/01/19 17:22:50  b1
// Initial check in DOOM sources as of Jan. 10th, 1997
//
//
// DESCRIPTION:
//	UNIX soundserver, run as a separate process,
//	 started by DOOM program.
//	Originally conceived fopr SGI Irix,
//	 mostly used with Linux voxware.
//
//-----------------------------------------------------------------------------


static const char rcsid[] = "$Id: soundsrv.c,v 1.3 1997/01/29 22:40:44 b1 Exp $";



#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <SDL2/SDL.h>


#include "sounds.h"
#include "soundsrv.h"
#ifdef DOOM
#include "../w_wad.h"
#else
#include "wadread.h"
#endif


// set this to whatever. I honestly couldn't care less...
#define SNDSERV_PIPE "/tmp/sndserv_em_pipe"


//
// Department of Redundancy Department.
//
typedef struct wadinfo_struct
{
    // should be IWAD
    char	identification[4];	
    int		numlumps;
    int		infotableofs;
    
} wadinfo_t;


typedef struct filelump_struct
{
    int		filepos;
    int		size;
    char	name[8];
    
} filelump_t;


// an internal time keeper
static int	mytime = 0;

// number of sound effects
int 		numsounds;

// longest sound effect
int 		longsound;

// SNDSERV_lengths of all sound effects
int 		SNDSERV_lengths[NUMSFX];

// mixing buffer
signed short	SNDSERV_mixbuffer[SNDSERV_MIXBUFFERSIZE];

// file descriptor of sfx device
int		sfxdevice;			

// file descriptor of music device
int 		musdevice;			

// the SNDSERV_channel data pointers
unsigned char*	SNDSERV_channels[8];

// the SNDSERV_channel step amount
unsigned int	SNDSERV_channelstep[8];

// 0.16 bit remainder of last step
unsigned int	SNDSERV_channelstepremainder[8];

// the SNDSERV_channel data end pointers
unsigned char*	SNDSERV_channelsend[8];

// time that the SNDSERV_channel started playing
int		SNDSERV_channelstart[8];

// the SNDSERV_channel handles
int 		SNDSERV_channelhandles[8];

// the SNDSERV_channel left volume lookup
int*		SNDSERV_channelleftvol_lookup[8];

// the SNDSERV_channel right volume lookup
int*		SNDSERV_channelrightvol_lookup[8];

// sfx id of the playing sound effect
int		SNDSERV_channelids[8];			

int		snd_verbose=1;

int		SNDSERV_steptable[256];

int		SNDSERV_vol_lookup[128*256];

static void SNDSERV_derror(char* msg)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(-1);
}

int SNDSERV_mix(void)
{

    register int		dl;
    register int		dr;
    register unsigned int	sample;
    
    signed short*		leftout;
    signed short*		rightout;
    signed short*		leftend;
    
    int				step;

    leftout = SNDSERV_mixbuffer;
    rightout = SNDSERV_mixbuffer+1;
    step = 2;

    leftend = SNDSERV_mixbuffer + SAMPLECOUNT*step;

    // mix into the mixing buffer
    while (leftout != leftend)
    {

	dl = 0;
	dr = 0;

	if (SNDSERV_channels[0])
	{
	    sample = *SNDSERV_channels[0];
	    dl += SNDSERV_channelleftvol_lookup[0][sample];
	    dr += SNDSERV_channelrightvol_lookup[0][sample];
	    SNDSERV_channelstepremainder[0] += SNDSERV_channelstep[0];
	    SNDSERV_channels[0] += SNDSERV_channelstepremainder[0] >> 16;
	    SNDSERV_channelstepremainder[0] &= 65536-1;

	    if (SNDSERV_channels[0] >= SNDSERV_channelsend[0])
		SNDSERV_channels[0] = 0;
	}

	if (SNDSERV_channels[1])
	{
	    sample = *SNDSERV_channels[1];
	    dl += SNDSERV_channelleftvol_lookup[1][sample];
	    dr += SNDSERV_channelrightvol_lookup[1][sample];
	    SNDSERV_channelstepremainder[1] += SNDSERV_channelstep[1];
	    SNDSERV_channels[1] += SNDSERV_channelstepremainder[1] >> 16;
	    SNDSERV_channelstepremainder[1] &= 65536-1;

	    if (SNDSERV_channels[1] >= SNDSERV_channelsend[1])
		SNDSERV_channels[1] = 0;
	}

	if (SNDSERV_channels[2])
	{
	    sample = *SNDSERV_channels[2];
	    dl += SNDSERV_channelleftvol_lookup[2][sample];
	    dr += SNDSERV_channelrightvol_lookup[2][sample];
	    SNDSERV_channelstepremainder[2] += SNDSERV_channelstep[2];
	    SNDSERV_channels[2] += SNDSERV_channelstepremainder[2] >> 16;
	    SNDSERV_channelstepremainder[2] &= 65536-1;

	    if (SNDSERV_channels[2] >= SNDSERV_channelsend[2])
		SNDSERV_channels[2] = 0;
	}
	
	if (SNDSERV_channels[3])
	{
	    sample = *SNDSERV_channels[3];
	    dl += SNDSERV_channelleftvol_lookup[3][sample];
	    dr += SNDSERV_channelrightvol_lookup[3][sample];
	    SNDSERV_channelstepremainder[3] += SNDSERV_channelstep[3];
	    SNDSERV_channels[3] += SNDSERV_channelstepremainder[3] >> 16;
	    SNDSERV_channelstepremainder[3] &= 65536-1;

	    if (SNDSERV_channels[3] >= SNDSERV_channelsend[3])
		SNDSERV_channels[3] = 0;
	}
	
	if (SNDSERV_channels[4])
	{
	    sample = *SNDSERV_channels[4];
	    dl += SNDSERV_channelleftvol_lookup[4][sample];
	    dr += SNDSERV_channelrightvol_lookup[4][sample];
	    SNDSERV_channelstepremainder[4] += SNDSERV_channelstep[4];
	    SNDSERV_channels[4] += SNDSERV_channelstepremainder[4] >> 16;
	    SNDSERV_channelstepremainder[4] &= 65536-1;

	    if (SNDSERV_channels[4] >= SNDSERV_channelsend[4])
		SNDSERV_channels[4] = 0;
	}
	
	if (SNDSERV_channels[5])
	{
	    sample = *SNDSERV_channels[5];
	    dl += SNDSERV_channelleftvol_lookup[5][sample];
	    dr += SNDSERV_channelrightvol_lookup[5][sample];
	    SNDSERV_channelstepremainder[5] += SNDSERV_channelstep[5];
	    SNDSERV_channels[5] += SNDSERV_channelstepremainder[5] >> 16;
	    SNDSERV_channelstepremainder[5] &= 65536-1;

	    if (SNDSERV_channels[5] >= SNDSERV_channelsend[5])
		SNDSERV_channels[5] = 0;
	}
	
	if (SNDSERV_channels[6])
	{
	    sample = *SNDSERV_channels[6];
	    dl += SNDSERV_channelleftvol_lookup[6][sample];
	    dr += SNDSERV_channelrightvol_lookup[6][sample];
	    SNDSERV_channelstepremainder[6] += SNDSERV_channelstep[6];
	    SNDSERV_channels[6] += SNDSERV_channelstepremainder[6] >> 16;
	    SNDSERV_channelstepremainder[6] &= 65536-1;

	    if (SNDSERV_channels[6] >= SNDSERV_channelsend[6])
		SNDSERV_channels[6] = 0;
	}
	if (SNDSERV_channels[7])
	{
	    sample = *SNDSERV_channels[7];
	    dl += SNDSERV_channelleftvol_lookup[7][sample];
	    dr += SNDSERV_channelrightvol_lookup[7][sample];
	    SNDSERV_channelstepremainder[7] += SNDSERV_channelstep[7];
	    SNDSERV_channels[7] += SNDSERV_channelstepremainder[7] >> 16;
	    SNDSERV_channelstepremainder[7] &= 65536-1;

	    if (SNDSERV_channels[7] >= SNDSERV_channelsend[7])
		SNDSERV_channels[7] = 0;
	}

	// Has been char instead of short.
	// if (dl > 127) *leftout = 127;
	// else if (dl < -128) *leftout = -128;
	// else *leftout = dl;

	// if (dr > 127) *rightout = 127;
	// else if (dr < -128) *rightout = -128;
	// else *rightout = dr;
	
	if (dl > 0x7fff)
	    *leftout = 0x7fff;
	else if (dl < -0x8000)
	    *leftout = -0x8000;
	else
	    *leftout = dl;

	if (dr > 0x7fff)
	    *rightout = 0x7fff;
	else if (dr < -0x8000)
	    *rightout = -0x8000;
	else
	    *rightout = dr;

	leftout += step;
	rightout += step;

    }
    return 1;
}



void
SNDSERV_grabdata
( int		c,
  char**	v )
{
    int		i;
    char*	name;
    char*	doom1wad;
    char*	doomwad;
    char*	doomuwad;
    char*	doom2wad;
    char*	doom2fwad;
    // Now where are TNT and Plutonia. Yuck.
    
    //	char *home;
    char*	doomwaddir;

    doomwaddir = getenv("DOOMWADDIR");

    if (!doomwaddir)
	doomwaddir = ".";

    doom1wad = malloc(strlen(doomwaddir)+1+9+1);
    sprintf(doom1wad, "%s/doom1.wad", doomwaddir);

    doom2wad = malloc(strlen(doomwaddir)+1+9+1);
    sprintf(doom2wad, "%s/doom2.wad", doomwaddir);

    doom2fwad = malloc(strlen(doomwaddir)+1+10+1);
    sprintf(doom2fwad, "%s/doom2f.wad", doomwaddir);
    
    doomuwad = malloc(strlen(doomwaddir)+1+8+1);
    sprintf(doomuwad, "%s/doomu.wad", doomwaddir);
    
    doomwad = malloc(strlen(doomwaddir)+1+8+1);
    sprintf(doomwad, "%s/doom.wad", doomwaddir);

    //	home = getenv("HOME");
    //	if (!home)
    //	  SNDSERV_derror("Please set $HOME to your home directory");
    //	sprintf(basedefault, "%s/.doomrc", home);


    for (i=1 ; i<c ; i++)
    {
	if (!strcmp(v[i], "-quiet"))
	{
	    snd_verbose = 0;
	}
    }

    numsounds = NUMSFX;
    longsound = 0;

    if (! access(doom2fwad, R_OK) )
	name = doom2fwad;
    else if (! access(doom2wad, R_OK) )
	name = doom2wad;
    else if (! access(doomuwad, R_OK) )
	name = doomuwad;
    else if (! access(doomwad, R_OK) )
	name = doomwad;
    else if (! access(doom1wad, R_OK) )
	name = doom1wad;
    // else if (! access(DEVDATA "doom2.wad", R_OK) )
    //   name = DEVDATA "doom2.wad";
    //   else if (! access(DEVDATA "doom.wad", R_OK) )
    //   name = DEVDATA "doom.wad";
    else
    {
	fprintf(stderr, "Could not find wadfile anywhere\n");
	exit(-1);
    }

#ifdef DOOM
	openwad(
#else
	SNDSERV_openwad(
#endif
		name
	);
    if (snd_verbose)
	fprintf(stderr, "loading from [%s]\n", name);

    for (i=1 ; i<NUMSFX ; i++)
    {
	if (!SNDSERV_S_sfx[i].link)
	{
	    SNDSERV_S_sfx[i].data = 
#ifdef DOOM
		getsfx(
#else
		SNDSERV_getsfx(
#endif
			SNDSERV_S_sfx[i].name,
			&SNDSERV_lengths[i]
		);
	    if (longsound < SNDSERV_lengths[i]) longsound = SNDSERV_lengths[i];
	} else {
	    SNDSERV_S_sfx[i].data = SNDSERV_S_sfx[i].link->data;
	    SNDSERV_lengths[i] = SNDSERV_lengths[(SNDSERV_S_sfx[i].link - SNDSERV_S_sfx)/sizeof(sfxinfo_t)];
	}
	// test only
	//  {
	//  int fd;
	//  char name[10];
	//  sprintf(name, "sfx%d", i);
	//  fd = open(name, O_WRONLY|O_CREAT, 0644);
	//  write(fd, SNDSERV_S_sfx[i].data, SNDSERV_lengths[i]);
	//  close(fd);
	//  }
    }

}

static struct timeval		last={0,0};
//static struct timeval		now;

static struct timezone		whocares;

void SNDSERV_updatesounds(void)
{

    SNDSERV_mix();
    SNDSERV_SubmitOutputBuffer(SNDSERV_mixbuffer, SAMPLECOUNT);

}

int
SNDSERV_addsfx
( int		sfxid,
  int		volume,
  int		step,
  int		seperation )
{
    static unsigned short	handlenums = 0;
 
    int		i;
    int		rc = -1;
    
    int		oldest = mytime;
    int		oldestnum = 0;
    int		slot;
    int		rightvol;
    int		leftvol;

    // play these sound effects
    //  only one at a time
    if ( sfxid == sfx_sawup
	 || sfxid == sfx_sawidl
	 || sfxid == sfx_sawful
	 || sfxid == sfx_sawhit
	 || sfxid == sfx_stnmov
	 || sfxid == sfx_pistol )
    {
	for (i=0 ; i<8 ; i++)
	{
	    if (SNDSERV_channels[i] && SNDSERV_channelids[i] == sfxid)
	    {
		SNDSERV_channels[i] = 0;
		break;
	    }
	}
    }

    for (i=0 ; i<8 && SNDSERV_channels[i] ; i++)
    {
	if (SNDSERV_channelstart[i] < oldest)
	{
	    oldestnum = i;
	    oldest = SNDSERV_channelstart[i];
	}
    }

    if (i == 8)
	slot = oldestnum;
    else
	slot = i;

    SNDSERV_channels[slot] = (unsigned char *) SNDSERV_S_sfx[sfxid].data;
    SNDSERV_channelsend[slot] = SNDSERV_channels[slot] + SNDSERV_lengths[sfxid];

    if (!handlenums)
	handlenums = 100;
    
    SNDSERV_channelhandles[slot] = rc = handlenums++;
    SNDSERV_channelstep[slot] = step;
    SNDSERV_channelstepremainder[slot] = 0;
    SNDSERV_channelstart[slot] = mytime;

    // (range: 1 - 256)
    seperation += 1;

    // (x^2 seperation)
    leftvol =
	volume - (volume*seperation*seperation)/(256*256);

    seperation = seperation - 257;

    // (x^2 seperation)
    rightvol =
	volume - (volume*seperation*seperation)/(256*256);	

    // sanity check
    if (rightvol < 0 || rightvol > 127)
	SNDSERV_derror("rightvol out of bounds");
    
    if (leftvol < 0 || leftvol > 127)
	SNDSERV_derror("leftvol out of bounds");
    
    // get the proper lookup table piece
    //  for this volume level
    SNDSERV_channelleftvol_lookup[slot] = &SNDSERV_vol_lookup[leftvol*256];
    SNDSERV_channelrightvol_lookup[slot] = &SNDSERV_vol_lookup[rightvol*256];

    SNDSERV_channelids[slot] = sfxid;

    return rc;

}


void SNDSERV_outputushort(int num)
{

    static unsigned char	buff[5] = { 0, 0, 0, 0, '\n' };
    static char*		badbuff = "xxxx\n";

    // outputs a 16-bit # in hex or "xxxx" if -1.
    if (num < 0)
    {
	write(1, badbuff, 5);
    }
    else
    {
	buff[0] = num>>12;
	buff[0] += buff[0] > 9 ? 'a'-10 : '0';
	buff[1] = (num>>8) & 0xf;
	buff[1] += buff[1] > 9 ? 'a'-10 : '0';
	buff[2] = (num>>4) & 0xf;
	buff[2] += buff[2] > 9 ? 'a'-10 : '0';
	buff[3] = num & 0xf;
	buff[3] += buff[3] > 9 ? 'a'-10 : '0';
	write(1, buff, 5);
    }
}

void SNDSERV_initdata(void)
{

    int		i;
    int		j;
    
    int*	SNDSERV_steptablemid = SNDSERV_steptable + 128;

    for (i=0 ;
	 i<sizeof(SNDSERV_channels)/sizeof(unsigned char *) ;
	 i++)
    {
	SNDSERV_channels[i] = 0;
    }
    
    gettimeofday(&last, &whocares);

    for (i=-128 ; i<128 ; i++)
	SNDSERV_steptablemid[i] = pow(2.0, (i/64.0))*65536.0;

    // generates volume lookup tables
    //  which also turn the unsigned samples
    //  into signed samples
    // for (i=0 ; i<128 ; i++)
    // for (j=0 ; j<256 ; j++)
    // SNDSERV_vol_lookup[i*256+j] = (i*(j-128))/127;
    
    for (i=0 ; i<128 ; i++)
	for (j=0 ; j<256 ; j++)
	    SNDSERV_vol_lookup[i*256+j] = (i*(j-128)*256)/127;

}




void SNDSERV_quit(void)
{
    SNDSERV_ShutdownMusic();
    SNDSERV_ShutdownSound();
#ifndef DOOM
	SDL_Quit();
#endif
    exit(0);
}



fd_set		fdset;
fd_set		scratchset;



int
#ifndef DOOM
main
#else
SNDSERV_main
#endif
( int		c,
  char**	v )
{

    printf("Linuxxdoom Soundserver (Emscripten SDL2 Port)\n");
	int 	pipe_fd;
    int		done = 0;
    int		rc;
    int		nrc;
    int		sndnum;
    int		handle = 0;
    
    unsigned char	commandbuf[10];
    struct timeval	zerowait = { 0, 0 };

    
    int 	step;
    int 	vol;
    int		sep;
    
    int		i;
    int		waitingtofinish=0;
#ifdef DOOM
	// pipe of amaze
	mkfifo(SNDSERV_PIPE, 0666);

	pipe_fd = open(SNDSERV_PIPE, O_RDONLY);
    if (pipe_fd == -1) {
        perror("open");
        return 1;
    }

#endif
    // get sound data
    SNDSERV_grabdata(c, v);

    // init any data
    SNDSERV_initdata();

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "could not initialise audio: %s\n", SDL_GetError());
        return 1;
    }

    SNDSERV_InitSound(11025, 16);

    SNDSERV_InitMusic();

    if (snd_verbose)
	fprintf(stderr, "ready\n");
    
    // parse commands and play sounds until done
    FD_ZERO(&fdset);
    FD_SET(0, &fdset);

    while (!done)
    {
	mytime++;

	if (!waitingtofinish)
	{
	    do {
		scratchset = fdset;
		rc = select(FD_SETSIZE, &scratchset, 0, 0, &zerowait);

		if (rc > 0)
		{
		    //	fprintf(stderr, "select is true\n");
		    // got a command
		    nrc = read(
#ifdef DOOM
				pipe_fd,
#else
				0,
#endif
				commandbuf, 1);

		    if (!nrc)
		    {
			done = 1;
			rc = 0;
		    }
		    else
		    {
			if (snd_verbose)
			    fprintf(stderr, "cmd: %c", commandbuf[0]);

			switch (commandbuf[0])
			{
			  case 'p':
			    // play a new sound effect
			    read(0, commandbuf, 9);

			    if (snd_verbose)
			    {
				commandbuf[9]=0;
				fprintf(stderr, "%s\n", commandbuf);
			    }

			    commandbuf[0] -=
				commandbuf[0]>='a' ? 'a'-10 : '0';
			    commandbuf[1] -=
				commandbuf[1]>='a' ? 'a'-10 : '0';
			    commandbuf[2] -=
				commandbuf[2]>='a' ? 'a'-10 : '0';
			    commandbuf[3] -=
				commandbuf[3]>='a' ? 'a'-10 : '0';
			    commandbuf[4] -=
				commandbuf[4]>='a' ? 'a'-10 : '0';
			    commandbuf[5] -=
				commandbuf[5]>='a' ? 'a'-10 : '0';
			    commandbuf[6] -=
				commandbuf[6]>='a' ? 'a'-10 : '0';
			    commandbuf[7] -=
				commandbuf[7]>='a' ? 'a'-10 : '0';

			    //	p<snd#><step><vol><sep>
			    sndnum = (commandbuf[0]<<4) + commandbuf[1];
			    step = (commandbuf[2]<<4) + commandbuf[3];
			    step = SNDSERV_steptable[step];
			    vol = (commandbuf[4]<<4) + commandbuf[5];
			    sep = (commandbuf[6]<<4) + commandbuf[7];

			    handle = SNDSERV_addsfx(sndnum, vol, step, sep);
			    // returns the handle
			    //	SNDSERV_outputushort(handle);
			    break;
			    
			  case 'q':
			    read(0, commandbuf, 1);
			    waitingtofinish = 1; rc = 0;
			    break;
			    
			  case 's':
			  {
			      int fd;
			      read(0, commandbuf, 3);
			      commandbuf[2] = 0;
			      fd = open((char*)commandbuf, O_CREAT|O_WRONLY, 0644);
			      commandbuf[0] -= commandbuf[0]>='a' ? 'a'-10 : '0';
			      commandbuf[1] -= commandbuf[1]>='a' ? 'a'-10 : '0';
			      sndnum = (commandbuf[0]<<4) + commandbuf[1];
			      write(fd, SNDSERV_S_sfx[sndnum].data, SNDSERV_lengths[sndnum]);
			      close(fd);
			  }
			  break;
			  
			  default:
			    fprintf(stderr, "Did not recognize command\n");
			    break;
			}
		    }
		}
		else if (rc < 0)
		{
		    SNDSERV_quit();
		}
	    } while (rc > 0);
	}

	SNDSERV_updatesounds();

	if (waitingtofinish)
	{
	    for(i=0 ; i<8 && !SNDSERV_channels[i] ; i++);
	    
	    if (i==8)
		done=1;
	}

    }
	close(pipe_fd);

    SNDSERV_quit();
    return 0;
}


#ifdef DOOM
void* SNDSERV_mainthread(void* unused) {
    char* args[] = { "sndserver", "-quiet", NULL };
    SNDSERV_main(2, args);
    return NULL;
}
#endif

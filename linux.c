// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: linux.c,v 1.3 1997/01/26 07:45:01 b1 Exp $
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
// $Log: linux.c,v $
// Revision 1.3  1997/01/26 07:45:01  b1
// 2nd formatting run, fixed a few warnings as well.
//
// Revision 1.2  1997/01/21 19:00:01  b1
// First formatting run:
//  using Emacs cc-mode.el indentation for C++ now.
//
// Revision 1.1  1997/01/19 17:22:45  b1
// Initial check in DOOM sources as of Jan. 10th, 1997
//
//
// DESCRIPTION:
//	UNIX, soundserver for Linux i386.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: linux.c,v 1.3 1997/01/26 07:45:01 b1 Exp $";


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "soundsrv.h"



void I_InitMusic(void)
{
    // stupid linuxxdoom bastards too lazy to implement the DOOM music
}

void
I_InitSound
( int	samplerate,
  int	samplesize )
{

    // Nice and simple initialise, compared to all the other shit

    if (Mix_OpenAudio(samplerate, AUDIO_S16SYS, 2, 1024) < 0) {
        fprintf(stderr, "Mix_OpenAudio Error: %s\n", Mix_GetError());
        return;
    }

    Mix_AllocateChannels(16);
    Mix_Resume(-1);

    printf("sound is ready\n");

}

void
I_SubmitOutputBuffer
( void*	samples,
  int	samplecount )
{
    Mix_Chunk *chunk = Mix_QuickLoad_RAW(samples, samplecount * 2);

    if (chunk) {
        Mix_PlayChannel(-1, chunk, 0);
        Mix_FreeChunk(chunk);
    }
}

void I_ShutdownSound(void)
{

    Mix_CloseAudio();

}

void I_ShutdownMusic(void)
{
}

// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: soundsrv.h,v 1.3 1997/01/29 22:40:44 b1 Exp $
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
// $Log: soundsrv.h,v $
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
//	UNIX soundserver, separate process. 
//
//-----------------------------------------------------------------------------

#ifndef __SNDSERVER_H__
#define __SNDSERVER_H__

#define SAMPLECOUNT		512
#define SNDSERV_MIXBUFFERSIZE	(SAMPLECOUNT*2*2)
#define SPEED			11025

#ifdef DOOM
int SNDSERV_main
( int		c,
  char**	v );

void* SNDSERV_mainthread(void* unused);
#endif


void SNDSERV_InitMusic(void);

void
SNDSERV_InitSound
( int		samplerate,
  int		samplesound );

void
SNDSERV_SubmitOutputBuffer
( void*		samples,
  int		samplecount );

void SNDSERV_ShutdownSound(void);
void SNDSERV_ShutdownMusic(void);

#endif

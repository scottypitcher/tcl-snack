/* 
 * Copyright (C) 1997-2000 Kare Sjolander <kare@speech.kth.se>
 *
 * This file is part of the Snack sound extension for Tcl/Tk.
 * The latest version can be found at http://www.speech.kth.se/snack/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SNACK_AUDIO
#define _SNACK_AUDIO

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HPUX
#  include <Alib.h>
#endif

#ifdef Solaris
#  include <sys/types.h>
#  include <sys/file.h>
#  include <sys/ioctl.h>
#  include <sys/fcntl.h>
#  include <stropts.h>
#  include <sys/errno.h>
#  include <sys/audioio.h>
#  include <errno.h>
#  include <sys/filio.h>
#endif

#ifdef WIN
#  include "windows.h"
#  include "mmsystem.h"
#  include "mmreg.h"
#endif

#ifdef IRIX
#  include <audio.h>
#endif

#ifdef MAC_SNACK
#  include <Sound.h>
#  include <SoundInput.h>
#  ifndef rate44khz
#    define rate44khz ((unsigned long)(44100.0*65536))
#  endif /* !rate44khz */
/* How many buffers to maintain (2 is enough) */
#define NBUFS 2
/* The duration in seconds desired for each buffer */
/*#define DFLT_BUFTIME (0.0625) *//* i.e. frq/16, the favorite transfer size of the system */
#define DFLT_BUFTIME (0.25)	/* seems to work much better on the mac */
/* The number of SPBRecord calls to overlap.  I *think* this *has* to be zero */
#define INBUF_OVERLAP (0)

#endif /* MAC_SNACK */

typedef struct ADesc {

#ifdef HPUX
  Audio    *audio;
  ATransID  transid;
  int       Socket;
  int       last;
  int       convert;
#endif

#ifdef Linux
  int afd;
  int count;
  /*  int frag_size;*/
  double time;
  double timep;
  int freq;
  int convert;
#endif

#ifdef Solaris
  int afd;
  audio_info_t ainfo;
  double time;
  double timep;
  int freq;
  int convert;
  short *convBuf;
  int convSize;
#endif

#ifdef WIN
  int curr;
  int freq;
  int shortRead;
#endif

#ifdef IRIX
  ALport   port;
  ALconfig config;
  unsigned long long startfn;
  int count;
#endif

#ifdef MAC_SNACK
  /* Fields for handling output */
  SndChannelPtr schn;
  SndCommand	  scmd;
  SndDoubleBufferHeader  dbh;
  SndDoubleBufferPtr	   bufs[NBUFS]; /* the two double buffers */
  int currentBuf;	/* our own track of which buf is current */
  int bufsIssued;	/* For record: how many bufs have been set going */
  int bufsCompleted;	/* For record: how many bufs have completed */
  int bufFull[NBUFS];
  long     bufFrames;	    /* number of frames allocated per buffer */
  int running;	/* flag as to whether we have started yet */
  int pause;    /* flag that we are paused (used on input only?) */
  /* data for the callbacks */
  void     *data;	    /* pointer to the base of the sampled data */
  long     totalFrames;   /* how many frames there are */
  long     doneFrames;    /* how many we have already copied */
  /* Fields for input */
  long inRefNum;	    /* MacOS reference to input channel */
  SPBPtr spb[NBUFS];	    /* ptr to the parameter blocks for recording */
  /* debug stats */
  int completedblocks;
  int underruns;
#endif /* MAC_SNACK */

  int bytesPerSample;
  int nChannels;
  int mode;
  int debug;

} ADesc;

extern int  SnackAudioOpen(ADesc *A, Tcl_Interp *interp, int mode, int freq,
			   int channels, int encoding);
extern int  SnackAudioClose(ADesc *A);
extern int  SnackAudioPause(ADesc *A);
extern void SnackAudioResume(ADesc *A);
extern void SnackAudioFlush(ADesc *A);
extern void SnackAudioPost(ADesc *A);
extern int  SnackAudioRead(ADesc *A, void *buf, int nSamples);
extern int  SnackAudioWrite(ADesc *A, void *buf, int nSamples);
extern int  SnackAudioReadable(ADesc *A);
extern int  SnackAudioPlayed(ADesc *A);
extern int  SnackAudioWriteable(ADesc *A);

extern void SnackAudioGetFormats(char *buf, int n);
extern void SnackAudioGetFrequencies(char *buf, int n);

extern void ASetRecGain(int gain);
extern void ASetPlayGain(int gain);
extern int  AGetRecGain();
extern int  AGetPlayGain();

extern void SnackMixerOpen();
extern void SnackMixerGetInputJacks(char *buf, int n);
extern void SnackMixerGetOutputJacks(char *buf, int n);
extern void SnackMixerGetInputJack(char *buf, int n);
extern int  SnackMixerSetInputJack(Tcl_Interp *interp, char *jack, char *status);
extern void SnackMixerGetOutputJack(char *buf, int n);
extern void SnackMixerSetOutputJack(char *jack, char *status);
extern void SnackMixerGetNumChannels(char *mixer, char *buf, int n);
extern void SnackMixerGetVolume(char *mixer, int channel, char *buf, int n);
extern void SnackMixerSetVolume(char *mixer, int channel, int volume);
extern void SnackMixerGetMixers(char *buf, int n);
extern void SnackMixerLinkJacks(Tcl_Interp *interp, char *jack, Tcl_Obj *var);
extern void SnackMixerLinkVolume(Tcl_Interp *interp, char *mixer, int n,
			Tcl_Obj *CONST objv[]);
extern void SnackMixerUpdateVars(Tcl_Interp *interp);

#define RECORD 1
#define PLAY   2

#define SNACK_MONO   1
#define SNACK_STEREO 2
#define SNACK_QUAD   4

#define LIN16      1
#define ALAW       2
#define MULAW      3
#define LIN8OFFSET 4
#define LIN8       5

#define CAPABLEN 100

/*#ifdef Linux
extern short Snack_Alaw2Lin(unsigned char a_val);
extern short Snack_Mulaw2Lin(unsigned char u_val);
extern unsigned char Snack_Lin2Alaw(short pcm_val);
extern unsigned char Snack_Lin2Mulaw(short pcm_val);
#endif*/
extern double SnackCurrentTime();

typedef struct MixerLink {
  char *mixer;
  char *mixerVar;
  char *jack;
  char *jackVar;
  int channel;
} MixerLink;

#define VOLBUFSIZE 20
#define JACKBUFSIZE 40

#ifdef __cplusplus
}
#endif

#endif /* _SNACK_AUDIO */

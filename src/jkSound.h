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

#ifndef _SNACK_SOUND
#define _SNACK_SOUND

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#  include "windows.h"
#  include "mmsystem.h"
#  define strncasecmp strnicmp
#  define strcasecmp  stricmp
#  define EXPORT(a,b) __declspec(dllexport) a b
#else
#  define EXPORT(a,b) a b
#endif

#include "tcl.h"

typedef void (updateProc)(ClientData clientData, int flag);

typedef struct jkCallback {
  updateProc *proc;
  ClientData clientData;
  struct jkCallback *next;
  int id;
} jkCallback;

typedef enum {
  SNACK_NATIVE,
  SNACK_BIGENDIAN,
  SNACK_LITTLEENDIAN
} SnackEndianness;

typedef struct SnackLinkedFileInfo {
  Tcl_Channel linkCh;
  unsigned char *buffer;
  int filePos;
  struct Sound *sound;
} SnackLinkedFileInfo;

typedef struct Sound {
  int    sampfreq;
  int    sampformat;
  int    sampsize;
  int    nchannels;
  int    length;
  int    maxlength;
  short  maxsamp;
  short  minsamp;
  int    abmax;
  short  **blocks;
  int    maxblks;
  int    nblks;
  int    nPlayed;
  int    startPos;
  int    totLen;
  int    grab;
  int    active;
  short  *tmpbuf;
  int    swap;
  int    storeType;
  int    headSize;
  int    skipBytes;
  int    buffersize;
  Tcl_Channel    recchan;
  Tcl_Channel    playchan;
  Tcl_Interp     *interp;
  Tcl_Obj        *cmdPtr;
  char           *fcname;
  Tcl_TimerToken ptoken;
  Tcl_TimerToken rtoken;
  jkCallback *firstCB;
  char *fileType;
  int blockingPlay;
  int debug;
  int destroy;
  int guessFormat;
  Tcl_Channel rwchan;
  int inByteOrder;
  int firstNRead;
  int guessFrequency;
  int forceFormat;
  unsigned int userFlag; /* User flags, for new file formats, etc */
  char *userData;        /* User data pointer */
  int itemRefCnt;
  int validStart;
  SnackLinkedFileInfo linkInfo;
  /*  Tcl_Obj *monitorCmdPtr;*/

} Sound;

#define IDLE    0
#define READ    1
#define WRITE   2
#define GRABBED 3
#define PAUSED  4

#ifndef _MSC_VER
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif
/*
extern int  Snack_AddCallback(Sound *s, updateProc *proc, ClientData cd);
extern void Snack_RemoveCallback(Sound *s, int id);
extern void Snack_ExecCallbacks(Sound *s, int flag);*/

#define SNACK_NEW_SOUND	 1
#define SNACK_MORE_SOUND 2

extern int Snack_SoundCmd(ClientData cdata, Tcl_Interp *interp,
			  int objc, Tcl_Obj *CONST objv[]);
extern int Snack_AudioCmd(ClientData cdata, Tcl_Interp *interp,
			  int objc, Tcl_Obj *CONST objv[]);
extern void Snack_SoundDeleteCmd(ClientData cdata);
extern void Snack_AudioDeleteCmd(ClientData cdata);
/*
extern void Snack_WriteLog(char *s);
extern void Snack_WriteLogInt(char *s, int n);
*/
#define MAXNBLKS 200
#define SEXP 18
#define CEXP (SEXP+1)
#define SBLKSIZE (1<<SEXP)
#define CBLKSIZE (1<<CEXP)

#define SSAMPLE(s, i) (s)->blocks[(i)>>SEXP][((i)&(SBLKSIZE-1))]
#define UCSAMPLE(s, i) ((unsigned char **)(s)->blocks)[(i)>>CEXP][((i)&(CBLKSIZE-1))]
#define CSAMPLE(s, i) ((char **)(s)->blocks)[(i)>>CEXP][((i)&(CBLKSIZE-1))]

#define Snack_SetSample(s, c, i, val) if ((s)->sampformat == LIN16) { \
                                     SSAMPLE((s),(i)*(s)->nchannels+(c)) = (val); \
                                   } else if ((s)->sampformat == MULAW) { \
                                     UCSAMPLE((s),(i)*(s)->nchannels+(c)) = Snack_Lin2Mulaw(val); \
                                   } else if ((s)->sampformat == ALAW) { \
                                     UCSAMPLE((s),(i)*(s)->nchannels+(c)) = Snack_Lin2Alaw(val); \
                                   } else if ((s)->sampformat==LIN8OFFSET) { \
                                     UCSAMPLE((s),(i)*(s)->nchannels+(c)) = (unsigned char)(val); \
                                   } else if ((s)->sampformat == LIN8) { \
                                     UCSAMPLE((s),(i)*(s)->nchannels+(c)) = (unsigned char)(val); \
                                   }
#define Snack_GetSample(s, c, i) (\
   ((s)->sampformat == LIN16) ? SSAMPLE((s), (i)*(s)->nchannels+(c)): \
   (((s)->sampformat == MULAW) ? Snack_Mulaw2Lin(UCSAMPLE(s, (i)*(s)->nchannels+(c))): \
   ((((s)->sampformat == ALAW) ? Snack_Alaw2Lin(UCSAMPLE(s, (i)*(s)->nchannels+(c))): \
   (((((s)->sampformat == LIN8OFFSET) ? CSAMPLE(s, (i)*(s)->nchannels+(c)): UCSAMPLE(s, (i)*(s)->nchannels+(c)))))))))

#define Snack_SetLength(s, len) (s)->length = (len)
#define Snack_GetLength(s)      (s)->length
#define Snack_SetFrequency(s, freq) (s)->sampfreq = (freq)
#define Snack_GetFrequency(s)       (s)->sampfreq
#define Snack_SetSampleFormat(s, fmt) (s)->sampformat = (fmt)
#define Snack_GetSampleFormat(s)      (s)->sampformat
#define Snack_SetNumChannels(s, nchan) (s)->nchannels = (nchan)
#define Snack_GetNumChannels(s)        (s)->nchannels
#define Snack_GetBytesPerSample(s) (s)->sampsize
#define Snack_SetBytesPerSample(s, n) (s)->sampsize = (n)
#define Snack_GetDebugFlag(s) (s)->debug
#define Snack_SetHeaderSize(s, size) (s)->headSize = (size)
#define Snack_GetSoundFilename(s) (s)->fcname
#define Snack_GetSoundStatus(s) (s)->active

#define SOUND_IN_MEMORY 0
#define SOUND_IN_CHANNEL  1
#define SOUND_IN_FILE     2

typedef int (soundCmd)(Sound *s, Tcl_Interp *interp, int objc,
		     Tcl_Obj *CONST objv[]);
typedef int (audioCmd)(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
typedef void *Snack_CmdProc;
typedef void (soundDelCmd)(Sound *s);
typedef void (audioDelCmd)();
typedef void *Snack_DelCmdProc;
/*
extern int Snack_AddSubCmd(int snackCmd, char *cmdName, 
				    Snack_CmdProc *cmdProc,
				    Snack_DelCmdProc *delCmdProc);*/
#define SNACK_SOUND_CMD	1
#define SNACK_AUDIO_CMD	2
/*extern Sound *Snack_GetSound(Tcl_Interp *interp, char *name);*/
extern void Snack_StopSound(Sound *s, Tcl_Interp *interp);
/*extern int Snack_ResizeSoundStorage(Sound *s, int len);*/

#define HEADBUF 1024
/*
extern short Snack_Alaw2Lin(unsigned char a_val);
extern short Snack_Mulaw2Lin(unsigned char u_val);
extern unsigned char Snack_Lin2Alaw(short pcm_val);
extern unsigned char Snack_Lin2Mulaw(short pcm_val);
extern void Snack_UpdateExtremes(Sound *s, int start, int end, int flag);
extern void Snack_DeleteSound(Sound *s);*/
extern char *LoadSound(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj,
		       int startpos, int endpos);
extern int SaveSound(Sound *s, Tcl_Interp *interp, char *filename,
		     Tcl_Obj *obj, int startpos, int len, char *type);
extern int GetChannels(Tcl_Interp *interp, Tcl_Obj *obj, int *nchannels);
extern int GetFormat(Tcl_Interp *interp, Tcl_Obj *obj, 
		     int *sampformat, int *sampsize);
/*extern short Snack_SwapShort(short s);*/
extern long SwapLong(long l);
extern void ByteSwapSound(Sound *s);
extern void SwapIfBE(Sound *s);
extern void SwapIfLE(Sound *s);
extern int GetHeader(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj);
extern void PutHeader(Sound *s);
extern int WriteLELong(Tcl_Channel ch, long l);
extern int WriteBELong(Tcl_Channel ch, long l);
extern int SetFcname(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj);
extern char *NameGuessFileType(char *s);
extern void AddSnackNativeFormats();
extern int GetFileFormat(Tcl_Interp *interp, Tcl_Obj *obj, char **filetype);
typedef char *(guessFileTypeProc)(char *buf, int len);
typedef int  (getHeaderProc)(Sound *s, Tcl_Interp *interp, Tcl_Channel ch,
			     Tcl_Obj *obj, char *buf);
typedef char *(extensionFileTypeProc)(char *buf);
typedef int  (putHeaderProc)(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len);
typedef int  (openProc)(Sound *s, Tcl_Interp *interp,Tcl_Channel *ch,char *mode);
typedef int  (closeProc)(Sound *s, Tcl_Interp *interp, Tcl_Channel *ch);
typedef int  (readSamplesProc)(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, char *ibuf, char *obuf, int len);
typedef int  (writeSamplesProc)(Sound *s, Tcl_Channel ch, Tcl_Obj *obj,
				char *buf, int len);
typedef int  (seekProc)(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, int pos);

typedef struct jkFileFormat {
  char *formatName;
  guessFileTypeProc     *guessProc;
  getHeaderProc         *getHeaderProc;
  extensionFileTypeProc *extProc;
  putHeaderProc         *putHeaderProc;
  openProc              *openProc;
  closeProc             *closeProc;
  readSamplesProc       *readProc;
  writeSamplesProc      *writeProc;
  seekProc              *seekProc;
  struct jkFileFormat   *next;
} jkFileFormat;

/*extern int Snack_AddFileFormat(char *formatName,
			       guessFileTypeProc *guessProc,
			       getHeaderProc *getHeadProc,
			       extensionFileTypeProc *extProc,
			       putHeaderProc *putHeadProc,
			       openProc *openProc,
			       closeProc *closeProc,
			       readSamplesProc *readSamplesProc,
			       writeSamplesProc *writeSamplesProc,
			       seekProc *seekProc);
extern int SnackOpenFile(openProc *openProc, Sound *s, Tcl_Interp *interp,
			 Tcl_Channel *ch, char *mode);
extern int SnackSeekFile(seekProc *seekProc, Sound *s, Tcl_Interp *interp,
			 Tcl_Channel ch, int pos);
extern int SnackCloseFile(closeProc *closeProc, Sound *s, Tcl_Interp *interp,
			  Tcl_Channel *ch);*/
extern int GuessFormat(Sound *s, unsigned char *buf, int len);
extern char *GuessFileType(char *buf, int len, int eof);
/*extern Sound *Snack_NewSound(int frequency, int format, int nchannels);
extern void Snack_PutSoundData(Sound *s, int pos, void *buf, int nBytes);
extern void Snack_GetSoundData(Sound *s, int pos, void *buf, int nBytes);*/
extern int lengthCmd(Sound *s, Tcl_Interp *interp, int objc,
		     Tcl_Obj *CONST objv[]);
extern int insertCmd(Sound *s, Tcl_Interp *interp, int objc,
		     Tcl_Obj *CONST objv[]);
extern int cropCmd(Sound *s, Tcl_Interp *interp, int objc,
		   Tcl_Obj *CONST objv[]);
extern int copyCmd(Sound *s, Tcl_Interp *interp, int objc,
		   Tcl_Obj *CONST objv[]);
extern int appendCmd(Sound *s, Tcl_Interp *interp, int objc,
		     Tcl_Obj *CONST objv[]);
extern int concatenateCmd(Sound *s, Tcl_Interp *interp, int objc,
			  Tcl_Obj *CONST objv[]);
extern int cutCmd(Sound *s, Tcl_Interp *interp, int objc,
		  Tcl_Obj *CONST objv[]);
extern int convertCmd(Sound *s, Tcl_Interp *interp, int objc,
		      Tcl_Obj *CONST objv[]);
extern int fftCmd(Sound *s, Tcl_Interp *interp, int objc,
		  Tcl_Obj *CONST objv[]);

#define QUE_STRING "QUE"
#define RAW_STRING "RAW"
#define WAV_STRING "WAV"
#define AIFF_STRING "AIFF"
#define SMP_STRING "SMP"
#define AU_STRING "AU"
#define SD_STRING "SD"
#define MP3_STRING "MP3"

extern char *GuessMP3File(char *buf, int len);
extern int GetMP3Header(Sound *s, Tcl_Interp *interp, Tcl_Channel ch,
			Tcl_Obj *obj, char *buf);
extern int ReadMP3Samples(Sound *s, Tcl_Interp *interp, Tcl_Channel ch,
			  char *ibuf, char *obuf, int len);
extern int SeekMP3File(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, int pos);

#define NMAX 4096
#define NMIN 8
#define SNACK_HAMMING 1

extern void Snack_InitWindow(float *hamwin, int winlen, int fftlen, int type);

extern int  GetChannel(Tcl_Interp *interp, char *str, int nchannels,
		       int *channel);

extern int  Snack_InitFFT(int n);

extern void Snack_DBPowerSpectrum(float *x);

extern double SnackCurrentTime();

extern char *Snack_InitStubs (Tcl_Interp *interp, char *version, int exact);

extern int pitchCmd(Sound *s, Tcl_Interp *interp, int objc,
		    Tcl_Obj *CONST objv[]);

extern int reverseCmd(Sound *s, Tcl_Interp *interp, int objc,
		      Tcl_Obj *CONST objv[]);

#define ITEMBUFFERSIZE 100000

extern short GetSample(SnackLinkedFileInfo *infoPtr, int index);

extern int OpenLinkedFile(Sound *s, SnackLinkedFileInfo *infoPtr);

extern void CloseLinkedFile(SnackLinkedFileInfo *infoPtr);

extern void Snack_ExitProc(ClientData clientData);

extern int Snack_ProgressCallback(Sound *s, Tcl_Interp *interp, char *type,
				  double fraction);
/*
 * Include the public function declarations that are accessible via
 * the stubs table.
 */

#include "snackDecls.h"


#ifdef __cplusplus
}
#endif

#endif /* _SNACK_SOUND */

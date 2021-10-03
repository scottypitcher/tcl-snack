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

#include <stdlib.h>
#include "jkSound.h"
#include "jkAudIO.h"
#include <string.h>

#if defined Linux || defined WIN || defined _LITTLE_ENDIAN
#  define LE
#endif

struct jkFileFormat *snackFileFormats = NULL;

extern int useOldObjAPI;

int
Snack_AddFileFormat(char *formatName, guessFileTypeProc *guessProc,
		    getHeaderProc *getHeadProc, extensionFileTypeProc *extProc,
		    putHeaderProc *putHeadProc, openProc *openProc,
		    closeProc *closeProc, readSamplesProc *readProc,
		    writeSamplesProc *writeProc, seekProc *seekProc)
{
  jkFileFormat *ff = (jkFileFormat *) ckalloc(sizeof(jkFileFormat));

  if (ff == NULL) {
    return TCL_ERROR;
  }
  ff->formatName = formatName;
  ff->guessProc  = guessProc;
  ff->getHeaderProc = getHeadProc;
  ff->extProc    = extProc;
  ff->putHeaderProc = putHeadProc;
  ff->openProc   = openProc;
  ff->closeProc  = closeProc;
  ff->readProc   = readProc;
  ff->writeProc  = writeProc;
  ff->seekProc   = seekProc;
  ff->next       = snackFileFormats;
  snackFileFormats = ff;

  return TCL_OK;
}

static char *
GuessWavFile(char *buf, int len)
{
  if (len < 21) return(QUE_STRING);
  if (strncasecmp("RIFF", buf, strlen("RIFF")) == 0) {
    if (buf[20] == 85) {
      return(MP3_STRING);
    }
    if (strncasecmp("WAVE", &buf[8], strlen("WAVE")) == 0) {
      return(WAV_STRING);
    }
  }
  return(NULL);
}

static char *
GuessAuFile(char *buf, int len)
{
  if (len < 4) return(QUE_STRING);
  if (strncmp(".snd", buf, strlen(".snd")) == 0) {
    return(AU_STRING);
  }
  return(NULL);
}

static char *
GuessAiffFile(char *buf, int len)
{
  if (len < 20) return(QUE_STRING);
  if (strncasecmp("FORM", buf, strlen("FORM")) == 0) {
    if (strncasecmp("AIFF", &buf[8], strlen("AIFF")) == 0) {
      return(AIFF_STRING);
    }
  }
  return(NULL);
}

static char *
GuessSmpFile(char *buf, int len)
{
  int i, end = len - strlen("file=samp");

  for (i = 0; i < end; i++) {
    if (strncasecmp("file=samp", &buf[i], strlen("file=samp")) == 0) {
      return(SMP_STRING);
    }
  }
  if (len < HEADBUF/2) return(QUE_STRING);
  return(NULL);
}

static char *
GuessSdFile(char *buf, int len)
{
  if (len < 20) return(QUE_STRING);
  if (buf[16] == 0 && buf[17] == 0 && buf[18] == 106 && buf[19] == 26) {
    return(SD_STRING);
  }
  return(NULL);
}

static char *
GuessCSLFile(char *buf, int len)
{
  if (len < 8) return(QUE_STRING);
  if (strncmp("FORMDS16", buf, strlen("FORMDS16")) == 0) {
    return(CSL_STRING);
  }
  return(NULL);
}

static char *
GuessRawFile(char *buf, int len)
{
  return(RAW_STRING);  
}

char *
GuessFileType(char *buf, int len, int eof)
{
  jkFileFormat *ff;
  int flag = 0;

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    char *type = (ff->guessProc)(buf, len);
    if (type == NULL) {
      /* guessProc can't recognize this header */
    } else if (strcmp(type, QUE_STRING) == 0) {
      flag = 1; /* guessProc needs more bytes in order to decide */
    } else if (strcmp(type, RAW_STRING) != 0) {
      return(type);
    }
  }

  /* Don't decide yet if there's more header bytes to be had */

  if (flag && !eof) {
    return(QUE_STRING);
  }

  /* No guessProc recognized this header => guess RAW format */

  return(RAW_STRING);
}

static int
ExtCmp(char *s1, char *s2)
{
  int l1 = strlen(s1);
  int l2 = strlen(s2);

  return(strncasecmp(s1, &s2[l2 - l1], l1));
}

static char *
ExtSmpFile(char *s)
{
  if (ExtCmp(".smp", s) == 0) {
    return(SMP_STRING);
  }
  return(NULL);
}

static char *
ExtWavFile(char *s)
{
  if (ExtCmp(".wav", s) == 0) {
    return(WAV_STRING);
  }
  return(NULL);
}

static char *
ExtAuFile(char *s)
{
  if (ExtCmp(".au", s) == 0 || ExtCmp(".snd", s) == 0) {
    return(AU_STRING);
  }
  return(NULL);
}

static char *
ExtAiffFile(char *s)
{
  if (ExtCmp(".aif", s) == 0 || ExtCmp(".aiff", s) == 0) {
    return(AIFF_STRING);
  }
  return(NULL);
}

static char *
ExtSdFile(char *s)
{
  if (ExtCmp(".sd", s) == 0) {
    return(SD_STRING);
  }
  return(NULL);
}

static char *
ExtCSLFile(char *s)
{
  if (ExtCmp(".nsp", s) == 0) {
    return(CSL_STRING);
  }
  return(NULL);
}

char *
NameGuessFileType(char *s)
{
  jkFileFormat *ff;
  
  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (ff->extProc != NULL) {
      char *type = (ff->extProc)(s);
      if (type != NULL) {
	return(type);
      }
    }
  }
  return(RAW_STRING); 
}
/*
static short
ReadBEShort(Tcl_Channel ch)
{
  short ts;

  Tcl_Read(ch, (char *) &ts, sizeof(short));

#ifdef LE
  ts = Snack_SwapShort(ts);
#endif

  return(ts);
}

static short
ReadLEShort(Tcl_Channel ch)
{
  short ts;

  Tcl_Read(ch, (char *) &ts, sizeof(short));

#ifndef LE
  ts = Snack_SwapShort(ts);
#endif

  return(ts);
}

static long
ReadBELong(Tcl_Channel ch)
{
  long tl;

  Tcl_Read(ch, (char *) &tl, sizeof(long));

#ifdef LE
  tl = SwapLong(tl);
#endif

  return(tl);
}

static long
ReadLELong(Tcl_Channel ch)
{
  long tl;

  Tcl_Read(ch, (char *) &tl, sizeof(long));

#ifndef LE
  tl = SwapLong(tl);
#endif

  return(tl);
}
*/
static int
WriteLEShort(Tcl_Channel ch, short s)
{
  short ts = s;

#ifndef LE
  ts = Snack_SwapShort(ts);
#endif

  return(Tcl_Write(ch, (char *) &ts, sizeof(short)));
}

int
WriteLELong(Tcl_Channel ch, long l)
{
  long tl = l;

#ifndef LE
  tl = SwapLong(tl);
#endif

  return(Tcl_Write(ch, (char *) &tl, sizeof(long)));
}

static int
WriteBEShort(Tcl_Channel ch, short s)
{
  short ts = s;

#ifdef LE
  ts = Snack_SwapShort(ts);
#endif

  return(Tcl_Write(ch, (char *) &ts, sizeof(short)));
}

int
WriteBELong(Tcl_Channel ch, long l)
{
  long tl = l;

#ifdef LE
  tl = SwapLong(tl);
#endif

  return(Tcl_Write(ch, (char *) &tl, sizeof(long)));
}
  
static long
GetLELong(char *buf, int pos)
{
  long tl;

  memcpy(&tl, &buf[pos], sizeof(long));

#ifndef LE
  tl = SwapLong(tl);
#endif

  return(tl);
}

static short
GetLEShort(char *buf, int pos)
{
  short ts;
  char *p;
  short *q;

  p = &buf[pos];
  q = (short *) p;
  ts = *q;

#ifndef LE
  ts = Snack_SwapShort(ts);
#endif

  return(ts);
}

static long
GetBELong(char *buf, int pos)
{
  long tl;

  memcpy(&tl, &buf[pos], sizeof(long));

#ifdef LE
  tl = SwapLong(tl);
#endif

  return(tl);
}

static short
GetBEShort(char *buf, int pos)
{
  short ts;
  char *p;
  short *q;

  p = &buf[pos];
  q = (short *) p;
  ts = *q;

#ifdef LE
  ts = Snack_SwapShort(ts);
#endif

  return(ts);
}

static void
PutBELong(char *buf, int pos, long l)
{
  long tl = l;

#ifdef LE
  tl = SwapLong(tl);
#endif

  memcpy(&buf[pos], &tl, sizeof(long));
}

static void
PutBEShort(char *buf, int pos, short s)
{
  short ts = s;
  char *p;
  short *q;

  p = &buf[pos];
  q = (short *) p;

#ifdef LE
  ts = Snack_SwapShort(ts);
#endif

  *q = ts;
}

/* Note: pos must be a multiple of 4 */

static void
PutLELong(char *buf, int pos, long l)
{
  long tl = l;
  char *p;
  long *q;

  p = &buf[pos];
  q = (long *) p;

#ifndef LE
  tl = SwapLong(tl);
#endif

  *q = tl;
}

static void
PutLEShort(char *buf, int pos, short s)
{
  short ts = s;
  char *p;
  short *q;

  p = &buf[pos];
  q = (short *) p;

#ifndef LE
  ts = Snack_SwapShort(ts);
#endif

  *q = ts;
}

static int
ReadSound(readSamplesProc *readProc, Sound *s, Tcl_Interp *interp,
	  Tcl_Channel ch, Tcl_Obj *obj, int endpos)
{
  int n = 0, tot, totrlen = 0, res;

  if (s->length > 0) {
    if (endpos < 0 || (s->length - 1) < endpos) {
      endpos = s->length - 1;
    }
    s->length = endpos - (s->startPos / (s->sampsize * s->nchannels)) + 1;
    if (s->length < 0) s->length = 0;
    if (Snack_ResizeSoundStorage(s, s->length) != TCL_OK) {
      s->length = 0;
      Tcl_AppendResult(interp, "Memory allocation failed", NULL);
      return TCL_ERROR;
    }
  }

  if (s->length == -1) {
    tot = 1 << 30;
  } else {
    tot = s->length * s->sampsize * s->nchannels;
  }
  Snack_ProgressCallback(s->cmdPtr, interp, "Read sound", 0.0);
  while (tot > 0) {
    int size = min(tot, CBLKSIZE), rlen;

    if (s->length == -1) {
      if (Snack_ResizeSoundStorage(s, s->maxlength+1) != TCL_OK) {
	s->length = 0;
	Tcl_AppendResult(interp, "Memory allocation failed", NULL);
	return TCL_ERROR;
      }
    }
    if (ch != NULL) {
      if (readProc == NULL) {
	rlen = Tcl_Read(ch, (char *) s->blocks[n], size);
      } else {
	rlen = (readProc)(s, interp, ch, NULL, (char *) s->blocks[n], size);
      }
      if (rlen < 0) {
	Tcl_AppendResult(interp, "Error reading data", NULL);
	return TCL_ERROR;
      }
      totrlen += rlen;
      if (rlen < size) {
	break;
      }
    } else {
      unsigned char *ptr = NULL;
      if (useOldObjAPI) {
	ptr = (unsigned char *) obj->bytes;
      } else {
#ifdef TCL_81_API
	ptr = Tcl_GetByteArrayFromObj(obj, NULL);
#endif
      }
      if (readProc == NULL) {
	memcpy(s->blocks[n], &ptr[n*CBLKSIZE+s->headSize + s->startPos], size);
	totrlen += size;
      } else {
	rlen = (readProc)(s, interp, ch, (char *) ptr, (char *) s->blocks[n], size);
	totrlen += rlen;
	if (rlen < size) {
	  break;
	}
      }
    }
    tot -= size;
    n++;
    res = Snack_ProgressCallback(s->cmdPtr,
				 interp, "Read sound", (double) totrlen / 
				(s->length * s->sampsize * s->nchannels));
    if (res != TCL_OK) {
      Snack_ResizeSoundStorage(s, 0);
      s->length = 0;
      return TCL_ERROR;
    }
  }
  if ((double) totrlen / (s->length * s->sampsize * s->nchannels) < 1.0) {
    Snack_ProgressCallback(s->cmdPtr, interp, "Read sound", 1.0);
  }
  if (s->length * s->sampsize * s->nchannels != totrlen) {
    s->length = totrlen / (s->sampsize * s->nchannels);
  }
  if (s->length == -1) {
    s->length = totrlen / (s->sampsize * s->nchannels);
  }

  if (s->debug == 1) Snack_WriteLogInt("ReadSound", s->length);

  return TCL_OK;
}

static int
WriteSound(writeSamplesProc *writeProc, Sound *s, Tcl_Interp *interp,
	   Tcl_Channel ch, Tcl_Obj *obj, int startpos, int len, int hdsize)
{
  int i = 0, j;
  short sh;

  if (s->debug == 1) Snack_WriteLog("Enter WriteSound\n");

  if (s->inByteOrder == SNACK_NATIVE && s->swap) {
#ifdef LE
    s->inByteOrder = SNACK_BIGENDIAN;
#else
    s->inByteOrder = SNACK_LITTLEENDIAN;
#endif
  }

  startpos *= s->nchannels;
  len      *= s->nchannels;

  if (ch != NULL) {
    Snack_ProgressCallback(s->cmdPtr, interp, "Write sound", 0.0);
    if (writeProc == NULL) {
      if (s->sampsize == 2) {
	for (i = startpos; i < startpos + len; i++) {
	  switch (s->inByteOrder) {
	  case SNACK_NATIVE:
	    sh = SSAMPLE(s, i);
	    if (Tcl_Write(ch, (char *) &sh, 2) == -1) return TCL_ERROR;
	    break;
	  case SNACK_BIGENDIAN:
	    if (WriteBEShort(ch, SSAMPLE(s, i)) == -1) return TCL_ERROR;
	    break;
	  case SNACK_LITTLEENDIAN:
	    if (WriteLEShort(ch, SSAMPLE(s, i)) == -1) return TCL_ERROR;
	    break;
	  }
	  if ((i % 100000) == 99999) {
	    int res = Snack_ProgressCallback(s->cmdPtr, interp, "Write sound",
					     (double)(i-startpos)/len);
	    if (res != TCL_OK) {
	      return TCL_ERROR;
	    }
	  }
	}
      } else {
	for (i = startpos; i < startpos + len; i++) {
	  unsigned char c = UCSAMPLE(s, i);
	  if (Tcl_Write(ch, (char *)&c, 1) == -1) return TCL_ERROR;
	  if ((i % 100000) == 99999) {
	    int res = Snack_ProgressCallback(s->cmdPtr, interp, "Write sound",
					     (double) (i-startpos)/len);
	    if (res != TCL_OK) {
	      return TCL_ERROR;
	    }
	  }
	}
      }
    } else {
      int tot = len, n = 0;
      
      while (tot > 0) {
	int size = min(tot, CBLKSIZE);
	(writeProc)(s, ch, obj, (char *)s->blocks[n], size);
	tot -= size;
	n++;
      }
    }
    Snack_ProgressCallback(s->cmdPtr, interp, "Write sound", 1.0);
  } else {
    unsigned char *p = NULL;
    
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, hdsize + len * s->sampsize);
      p = (unsigned char *) &obj->bytes[hdsize];
    } else {
#ifdef TCL_81_API
      p = Tcl_SetByteArrayLength(obj, hdsize +len * s->sampsize);
      p = &p[hdsize];
#endif
    }
    if (s->sampsize == 2) {
      short *sp = (short *) p;
      
      for (i = startpos, j = 0; i < startpos + len; i++, j++) {
	switch (s->inByteOrder) {
	case SNACK_NATIVE:
	  sp[j] = SSAMPLE(s, i);
	  break;
	case SNACK_BIGENDIAN:
	  sh = SSAMPLE(s, i);
#ifdef LE
	  sh = Snack_SwapShort(sh);
#endif
	  sp[j] = sh;
	  break;
	case SNACK_LITTLEENDIAN:
	  sh = SSAMPLE(s, i);
#ifndef LE
	  sh = Snack_SwapShort(sh);
#endif
	  sp[j] = sh;
	  break;
	}
      }
    } else {
      for (i = startpos, j = hdsize; i < startpos + len; i++, j++) {
	unsigned char c = UCSAMPLE(s, i);
	
	p[j] = c;
      }
    }
  }
  if (s->debug == 1) Snack_WriteLog("Exit WriteSound\n");

  return TCL_OK;
}
#define NFIRSTSAMPLES 80000
#define DEFAULT_MULAW_FREQ 8000
#define DEFAULT_ALAW_FREQ 8000
#define DEFAULT_LIN8OFFSET_FREQ 11025
#define DEFAULT_LIN8_FREQ 11025

typedef enum {
  GUESS_LIN16,
  GUESS_LIN16S,
  GUESS_ALAW,
  GUESS_MULAW,
  GUESS_LIN8OFFSET,
  GUESS_LIN8
} sampleEncoding;

#define GUESS_FFT_LENGTH 512
#define SNACK_DEFAULT_GFWINTYPE SNACK_WIN_HAMMING

int
GuessFormat(Sound *s, unsigned char *buf, int len) {
  int i, j, format;
  float energyLIN16 = 0.0, energyLIN16S = 0.0;
  float energyMULAW = 0.0, energyALAW = 0.0;
  float energyLIN8  = 0.0, energyLIN8O = 0.0, minEnergy;
  float fft[GUESS_FFT_LENGTH];
  float totfft[GUESS_FFT_LENGTH];
  float hamwin[GUESS_FFT_LENGTH];
  double toterg = 0.0, cmperg = 0.0, minBin = 0.0;

  if (s->debug == 1) Snack_WriteLogInt("Enter GuessFormat", len);

  /*
    Byte order and sample encoding detection suggested by David van Leeuwen
    */
  
  for (i = 0; i < len / 2; i++) {
    short sampleLIN16  = ((short *)buf)[i];
    short sampleLIN16S = Snack_SwapShort(sampleLIN16);
    short sampleMULAW  = Snack_Mulaw2Lin(buf[i]);
    short sampleALAW   = Snack_Alaw2Lin(buf[i]);
    short sampleLIN8O  = (char)(buf[i] ^ 128) << 8;
    short sampleLIN8   = (char)buf[i] << 8;

    energyLIN16  += (float) sampleLIN16  * (float) sampleLIN16;
    energyLIN16S += (float) sampleLIN16S * (float) sampleLIN16S;
    energyMULAW  += (float) sampleMULAW  * (float) sampleMULAW;
    energyALAW   += (float) sampleALAW   * (float) sampleALAW;
    energyLIN8O  += (float) sampleLIN8O  * (float) sampleLIN8O;
    energyLIN8   += (float) sampleLIN8   * (float) sampleLIN8;
  }
  
  format = GUESS_LIN16;
  minEnergy = energyLIN16;
  
  if (energyLIN16S < minEnergy) {
    format = GUESS_LIN16S;
    minEnergy = energyLIN16S;
  }
  if (energyALAW < minEnergy) {
    format = GUESS_ALAW;
    minEnergy = energyALAW;
  }
  if (energyMULAW < minEnergy) {
    format = GUESS_MULAW;
    minEnergy = energyMULAW;
  }
  if (energyLIN8O < minEnergy) {
    format = GUESS_LIN8OFFSET;
    minEnergy = energyLIN8O;
  }
  if (energyLIN8 < minEnergy) {
    format = GUESS_LIN8;
    minEnergy = energyLIN8;
  }
  
  switch (format) {
  case GUESS_LIN16:
    s->swap = 0;
    if (s->sampsize == 1) {
      s->length /= 2;
    }
    s->sampformat = LIN16;
    s->sampsize = 2;
    break;
  case GUESS_LIN16S:
    s->swap = 1;
    if (s->sampsize == 1) {
      s->length /= 2;
    }
    s->sampformat = LIN16;
    s->sampsize = 2;
    break;
  case GUESS_ALAW:
    if (s->sampsize == 2) {
      s->length *= 2;
    }
    s->sampformat = ALAW;
    s->sampsize = 1;
    if (s->guessFrequency) {
      s->sampfreq = DEFAULT_ALAW_FREQ;
    }
    break;
  case GUESS_MULAW:
    if (s->sampsize == 2) {
      s->length *= 2;
    }
    s->sampformat = MULAW;
    s->sampsize = 1;
    if (s->guessFrequency) {
      s->sampfreq = DEFAULT_MULAW_FREQ;
    }
    break;
  case GUESS_LIN8OFFSET:
    if (s->sampsize == 2) {
      s->length *= 2;
    }
    s->sampformat = LIN8OFFSET;
    s->sampsize = 1;
    if (s->guessFrequency) {
      s->sampfreq = DEFAULT_LIN8OFFSET_FREQ;
    }
    break;
  case GUESS_LIN8:
    if (s->sampsize == 2) {
      s->length *= 2;
    }
    s->sampformat = LIN8;
    s->sampsize = 1;
    if (s->guessFrequency) {
      s->sampfreq = DEFAULT_LIN8_FREQ;
    }
    break;
  }

  if (s->guessFrequency && s->sampformat == LIN16) {
    for (i = 0; i < GUESS_FFT_LENGTH; i++) {
      totfft[i] = 0.0;
    }
    Snack_InitFFT(GUESS_FFT_LENGTH);
    Snack_InitWindow(hamwin, GUESS_FFT_LENGTH, GUESS_FFT_LENGTH / 2,
		     SNACK_DEFAULT_GFWINTYPE);
    for (i = 0; i < (len / s->sampsize) / (GUESS_FFT_LENGTH + 1); i++) {
      for (j = 0; j < GUESS_FFT_LENGTH; j++) {
	short sample  = ((short *)buf)[j + i * (GUESS_FFT_LENGTH / 2)];
	if (s->swap) {
	  sample = Snack_SwapShort(sample);
	}
	fft[j] = (float) sample * hamwin[j];
      }
      Snack_DBPowerSpectrum(fft);
      for (j = 0; j < GUESS_FFT_LENGTH / 2; j++) {
	totfft[j] += fft[j];
      }
    }
    for (i = 0; i < GUESS_FFT_LENGTH / 2; i++) {
      if (totfft[i] < minBin) minBin = totfft[i];
    }
    for (i = 0; i < GUESS_FFT_LENGTH / 2; i++) {
      toterg += (totfft[i] - minBin);
    }
    for (i = 0; i < GUESS_FFT_LENGTH / 2; i++) {
      cmperg += (totfft[i] - minBin);
      if (cmperg > toterg / 2.0) break;
    }

    if (i > 100) {
      /* Silence, don't guess */
    } else if (i > 64) {
      s->sampfreq = 8000;
    } else if (i > 46) {
      s->sampfreq = 11025;
    } else if (i > 32) {
      s->sampfreq = 16000;
    } else if (i > 23) {
      s->sampfreq = 22050;
    } else if (i > 16) {
      s->sampfreq = 32000;
    } else if (i > 11) {
      s->sampfreq = 44100;
    }
  }

  if (s->debug == 1) Snack_WriteLogInt("Exit GuessFormat", s->sampformat);
  
  return TCL_OK;
}

static int
GetRawHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	     char *buf)
{
  if (s->debug == 1) Snack_WriteLog("\tReading RAW header\n");

  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    s->length = (Tcl_Tell(ch) - s->skipBytes) / (s->sampsize * s->nchannels);
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      s->length = (obj->length  - s->skipBytes) / (s->sampsize * s->nchannels);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      s->length = (length - s->skipBytes) / (s->sampsize * s->nchannels);
#endif
    }
  }
  s->headSize = s->skipBytes;

  return TCL_OK;
}

static int
PutRawHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  return(0);
}

#define NIST_HEADERSIZE 1024

static int
GetSmpHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	     char *buf)
{
  char s1[100], s2[100];
  int i = 0, cont = 1;

  if (s->debug == 1) Snack_WriteLog("\tReading SMP header\n");

  if (s->firstNRead < NIST_HEADERSIZE) {
    if (Tcl_Read(ch, (char *)&buf[s->firstNRead],
		 NIST_HEADERSIZE-s->firstNRead) < 0) {
      return TCL_ERROR; 
    }
  }

  do {
    sscanf(&buf[i], "%s", s1);
    if (strncmp(s1, "sftot", 5) == 0) {
      sscanf(&buf[i+6], "%d", &s->sampfreq);
      if (s->debug == 1) Snack_WriteLogInt("Setting freq", s->sampfreq);
    } else if (strncmp(s1, "msb", 3) == 0) {
      sscanf(&buf[i+4], "%s", s2);
      if (s->debug == 1) { Snack_WriteLog(s2); Snack_WriteLog(" byte order\n"); }
    } else if (strncmp(s1, "nchans", 6) == 0) {
      sscanf(&buf[i+7], "%d", &s->nchannels);
      if (s->debug == 1) Snack_WriteLogInt("Setting number of channels", s->nchannels);
    } else if (buf[i] == 0) {
      cont = 0;
    }
    while (buf[i] != 10 && buf[i] != 0) i++;
    i++;
  } while (cont);

  s->sampformat = LIN16;
  s->sampsize = 2;
  s->swap = 0;

  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    s->length = (Tcl_Tell(ch) - NIST_HEADERSIZE) / (s->sampsize * s->nchannels);
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      s->length = (obj->length - NIST_HEADERSIZE) / (s->sampsize * s->nchannels);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      s->length = (length - NIST_HEADERSIZE) / (s->sampsize * s->nchannels);
#endif
    }
  }
  s->headSize = NIST_HEADERSIZE;
  if (strcmp(s2, "first") == 0) {
#ifdef LE
    SwapIfLE(s);
#endif
  } else {
#ifndef LE
    SwapIfBE(s);
#endif
  }

  return TCL_OK;
}

static int
PutSmpHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  int i = 0;
  char buf[HEADBUF];

  i += (int) sprintf(&buf[i], "file=samp\r\n");
  i += (int) sprintf(&buf[i], "sftot=%d\r\n", s->sampfreq);
#ifdef LE
  i += (int) sprintf(&buf[i], "msb=last\r\n");
#else
  i += (int) sprintf(&buf[i], "msb=first\r\n");
#endif
  i += (int) sprintf(&buf[i], "nchans=%d\r\n", s->nchannels);
  i += (int) sprintf(&buf[i], "preemph=none\r\nborn=snack\r\n=\r\n%c%c ", 0,4);

  for (;i < NIST_HEADERSIZE; i++) buf[i] = 0;

  if (ch != NULL) {
    if (Tcl_Write(ch, buf, NIST_HEADERSIZE) == -1) return TCL_ERROR;    
  } else {
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, NIST_HEADERSIZE);
      memcpy(obj->bytes, buf, NIST_HEADERSIZE);
    } else {
#ifdef TCL_81_API
      unsigned char *p = Tcl_SetByteArrayLength(obj, NIST_HEADERSIZE);
      memcpy(p, buf, NIST_HEADERSIZE);
#endif
    }
  }
  s->inByteOrder = SNACK_NATIVE;
  s->swap = 0;

  return(NIST_HEADERSIZE);
}

static int
GetSdHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	    char *buf)
{
  int datastart, len, i, j;
  double val = 16000.0;

  if (s->debug == 1) Snack_WriteLog("\tReading SD header\n");

  datastart = GetBELong(buf, 8);

  for (i = 0; i < 900; i++) { 
    if (strncasecmp("record_freq", &buf[i], strlen("record_freq")) == 0) {
      break;
    }
  }
  if (i < 900) {
    i = i + 18;
#ifdef LE
    for (j = 0; j < 4; j++) {
      char c = buf[i+j];

      buf[i+j] = buf[i+7-j];
      buf[i+7-j] = c;
    }
#endif
    memcpy(&val, &buf[i], 8);
  }

  s->sampformat = LIN16;
  s->sampsize = 2;
  s->nchannels = 1;
  s->sampfreq = (short) val;

  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    len = Tcl_Tell(ch);
    if (len == 0 || len < datastart) {
      Tcl_AppendResult(interp, "Failed reading SD header", NULL);
      return TCL_ERROR;
    }
    s->length = (len - datastart) / s->sampsize;
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      s->length = obj->length / s->sampsize;
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      s->length = length  / (s->sampsize * s->nchannels);
#endif
    }
  }
  s->headSize = datastart;

  return TCL_OK;
}

#define SND_FORMAT_MULAW_8   1
#define SND_FORMAT_LINEAR_8  2
#define SND_FORMAT_LINEAR_16 3
#define SND_FORMAT_ALAW_8    27

#define AU_HEADERSIZE 28

static int
GetAuHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	    char *buf)
{
  int tmp, hlen, nsamp, nsampfile;

  if (s->debug == 1) Snack_WriteLog("\tReading AU/SND header\n");

  if (s->firstNRead < AU_HEADERSIZE) {
    if (Tcl_Read(ch, (char *)&buf[s->firstNRead],
		 AU_HEADERSIZE-s->firstNRead) < 0) {
      return TCL_ERROR; 
    }
  }
  hlen = GetBELong(buf, 4);
  tmp  = GetBELong(buf, 12);
  if (tmp == SND_FORMAT_MULAW_8) {
    s->sampformat = MULAW;
    s->sampsize = 1;
  } else if (tmp == SND_FORMAT_LINEAR_8) {
    s->sampformat = LIN8OFFSET;
    s->sampsize = 1;
  } else if (tmp == SND_FORMAT_LINEAR_16) {
    s->sampformat = LIN16;
    s->sampsize = 2;
  } else if (tmp == SND_FORMAT_ALAW_8) {
    s->sampformat = ALAW;
    s->sampsize = 1;
  } else {
    Tcl_AppendResult(interp, "Unsupported AU format", NULL);
    return TCL_ERROR;
  }
  
  s->sampfreq = GetBELong(buf, 16);
  s->nchannels = GetBELong(buf, 20);
  s->headSize = hlen;
  nsamp = GetBELong(buf, 8) / (s->sampsize * s->nchannels);

  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    nsampfile = (Tcl_Tell(ch) - hlen) / (s->sampsize * s->nchannels);
    if (nsampfile < nsamp || nsamp == 0) {
      nsamp = nsampfile;
    }
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      nsamp = (obj->length - hlen) / (s->sampsize * s->nchannels);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      nsamp = (length - hlen) / (s->sampsize * s->nchannels);
#endif
    }
  }
  s->length = nsamp;
  SwapIfLE(s);

  return TCL_OK;
}

static int
PutAuHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  int tmp = 0;
  char buf[HEADBUF];

  if (s->debug == 1) Snack_WriteLog("\tSaving AU/SND\n");

  PutBELong(buf, 0, 0x2E736E64);
  PutBELong(buf, 4, AU_HEADERSIZE);
  PutBELong(buf, 8, len * s->sampsize * s->nchannels);

  if (s->sampformat == MULAW) {
    tmp = SND_FORMAT_MULAW_8;
  } else if (s->sampformat == LIN8OFFSET) {
    tmp = SND_FORMAT_LINEAR_8;
  } else if (s->sampformat == LIN16) {
    tmp = SND_FORMAT_LINEAR_16;
  } else if (s->sampformat == ALAW) {
    tmp = SND_FORMAT_ALAW_8;
  }
  PutBELong(buf, 12, tmp);

  PutBELong(buf, 16, s->sampfreq);
  PutBELong(buf, 20, s->nchannels);
  PutBELong(buf, 24, 0);

  if (ch != NULL) {
    if (Tcl_Write(ch, buf, AU_HEADERSIZE) == -1) return TCL_ERROR;
  } else {
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, AU_HEADERSIZE);
      memcpy(obj->bytes, buf, AU_HEADERSIZE);
    } else {
#ifdef TCL_81_API
      unsigned char *p = Tcl_SetByteArrayLength(obj, AU_HEADERSIZE);
      memcpy(p, buf, AU_HEADERSIZE);
#endif
    }
  }

  if (len == -1) {
    SwapIfLE(s);
  }
  s->inByteOrder = SNACK_BIGENDIAN;

  return(AU_HEADERSIZE);
}

#define WAVE_FORMAT_PCM	  1
#ifndef WIN
#  define WAVE_FORMAT_ALAW  6
#  define WAVE_FORMAT_MULAW 7
#endif

static int
GetHeaderBytes(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, char *buf, 
	       int len)
{
  int rlen = Tcl_Read(ch, &buf[s->firstNRead], len - s->firstNRead);

  if (rlen < len - s->firstNRead){
    Tcl_AppendResult(interp, "Failed reading header bytes", NULL);
    return TCL_ERROR;
  }
  s->firstNRead += rlen;

  return TCL_OK;
}

static int
GetWavHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	     char *buf)
{
  int tmp, nsamp = 0, nsampfile, i = 12, chunkLen;

  if (s->debug == 1) Snack_WriteLog("\tReading WAV header\n");

  /* buf[] = "RIFFxxxxWAVE" */

  while (1) {
    if (strncasecmp("fmt ", &buf[i], strlen("fmt ")) == 0) {
      chunkLen = GetLELong(buf, i + 4) + 8;
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      tmp = GetLEShort(buf, i+8);

      if (tmp == WAVE_FORMAT_PCM) {
	s->sampformat = LIN16;
      } else if (tmp == WAVE_FORMAT_ALAW) {
	s->sampformat = ALAW;
      } else if (tmp == WAVE_FORMAT_MULAW) {
	s->sampformat = MULAW;
      } else {
	Tcl_AppendResult(interp, "Unsupported WAV format", NULL);
	return TCL_ERROR;
      }
      
      s->nchannels = GetLEShort(buf, i+10);
      s->sampfreq   = GetLELong(buf, i+12);
      s->sampsize   = GetLEShort(buf, i+22) / 8;
      if (s->sampsize == 1 && s->sampformat == LIN16) {
	s->sampformat = LIN8OFFSET;
      }
      if (s->debug == 1) Snack_WriteLogInt("\tfmt chunk parsed", chunkLen);
    } else if (strncasecmp("data", &buf[i], strlen("data")) == 0) {
      nsamp = GetLELong(buf, i + 4) / (s->sampsize * s->nchannels);
      if (s->debug == 1) Snack_WriteLogInt("\tdata chunk parsed", nsamp);
      break;
    } else { /* unknown chunk */
      chunkLen = GetLELong(buf, i + 4) + 8;
      if (chunkLen < 0 || chunkLen > HEADBUF) {
	Tcl_AppendResult(interp, "Failed parsing WAV header", NULL);
	return TCL_ERROR;
      }
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      if (s->debug==1) Snack_WriteLogInt("\tSkipping unknown chunk", chunkLen);
    }

    i += chunkLen;
    if (s->firstNRead < i + 8) {
      if (GetHeaderBytes(s, interp, ch, buf, i + 8) != TCL_OK) {
	return TCL_ERROR;
      }
    }
    if (i >= HEADBUF) {
      Tcl_AppendResult(interp, "Failed parsing WAV header", NULL);
      return TCL_ERROR;
    }
  }
  
  s->headSize = i + 8;
  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    nsampfile = (Tcl_Tell(ch) - s->headSize) / (s->sampsize * s->nchannels);
    if (nsampfile < nsamp || nsamp == 0) {
      nsamp = nsampfile;
    }
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      nsampfile = (obj->length - s->headSize) / (s->sampsize * s->nchannels);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      nsampfile = (length - s->headSize) / (s->sampsize * s->nchannels);
#endif
    }
    if (nsampfile < nsamp || nsamp == 0) {
      nsamp = nsampfile;
    }
  }
  s->length = nsamp;
  SwapIfBE(s);

  return TCL_OK;
}

#define SNACK_WAV_HEADERSIZE 44

static int
PutWavHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  char buf[HEADBUF];

  sprintf(&buf[0], "RIFF");
  if (len != -1) {
    PutLELong(buf, 4, len * s->sampsize * s->nchannels + 36);
  } else {
    SwapIfBE(s);
    PutLELong(buf, 4, 0x7FFFFFFF);
  }
  sprintf(&buf[8], "WAVEfmt ");
  PutLELong(buf, 16, 16);

  if (s->sampformat == ALAW) PutLEShort(buf, 20, WAVE_FORMAT_ALAW);
  else if (s->sampformat == MULAW) PutLEShort(buf, 20, WAVE_FORMAT_MULAW);
  else PutLEShort(buf, 20, WAVE_FORMAT_PCM);

  PutLEShort(buf, 22, (short)s->nchannels);
  PutLELong(buf, 24, s->sampfreq);
  PutLELong(buf, 28, (s->sampfreq * s->nchannels * s->sampsize * 8 + 7) / 8);
  PutLEShort(buf, 32, (short)((s->nchannels * s->sampsize * 8 + 7) / 8));
  PutLEShort(buf, 34, (short) (s->sampsize * 8));
  sprintf(&buf[36], "data");
  if (len != -1) {
    PutLELong(buf, 40, len * s->sampsize * s->nchannels);
  } else {
    PutLELong(buf, 40, 0x7FFFFFDB);
  }
  if (ch != NULL) {
    if (Tcl_Write(ch, buf, SNACK_WAV_HEADERSIZE) == -1) return TCL_ERROR;
  } else {
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, SNACK_WAV_HEADERSIZE);
      memcpy(obj->bytes, buf, SNACK_WAV_HEADERSIZE);
    } else {
#ifdef TCL_81_API
      unsigned char *p = Tcl_SetByteArrayLength(obj, SNACK_WAV_HEADERSIZE);
      memcpy(p, buf, SNACK_WAV_HEADERSIZE);
#endif
    }
  }
  s->inByteOrder = SNACK_LITTLEENDIAN;

  return(SNACK_WAV_HEADERSIZE);
}

/* See http://www.borg.com/~jglatt/tech/aiff.htm */

static unsigned long
ConvertFloat(unsigned char *buffer)
{
  unsigned long mantissa;
  unsigned long last = 0;
  unsigned char exp;
  
  memcpy(&mantissa, buffer + 2, sizeof(long));
#ifdef LE
  mantissa = SwapLong(mantissa);
#endif
  exp = 30 - *(buffer+1);
  while (exp--) {
    last = mantissa;
    mantissa >>= 1;
  }
  if (last & 0x00000001) mantissa++;
  return(mantissa);
}

static void
StoreFloat(unsigned char * buffer, unsigned long value)
{
  unsigned long exp;
  unsigned char i;
  
  memset(buffer, 0, 10);
  
  exp = value;
  exp >>= 1;
  for (i=0; i<32; i++) {
    exp >>= 1;
    if (!exp) break;
  }
  *(buffer+1) = i;
  
  for (i=32; i; i--) {
    if (value & 0x80000000) break;
    value <<= 1;
  }

#ifdef LE
  value = SwapLong(value);
#endif  
  memcpy(buffer + 2, &value, sizeof(long));
}

static int
GetAiffHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	      char *buf)
{
  int tmp, i = 12, chunkLen = 4;

  if (s->debug == 1) Snack_WriteLog("\tReading AIFF header\n");
  
  /* buf[] = "FORMxxxxAIFF" */

  while (1) {
    if (strncasecmp("COMM", &buf[i], strlen("COMM")) == 0) {
      chunkLen = GetBELong(buf, i + 4) + 8;
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      s->nchannels = GetBEShort(buf, i + 8);
      tmp = GetBEShort(buf, i + 14);
      if (tmp == 16) {
	s->sampformat = LIN16;
	s->sampsize = 2;
      } else {
	s->sampformat = LIN8;
	s->sampsize = 1;
      }
      s->sampfreq = ConvertFloat((unsigned char *)&buf[i+16]);
      if (s->debug == 1) Snack_WriteLogInt("\tCOMM chunk parsed", chunkLen);
    } else if (strncasecmp("SSND", &buf[i], strlen("SSND")) == 0) {
      chunkLen = 16;
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + 8) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      s->length = (GetBELong(buf, i + 4) - 8) / (s->sampsize * s->nchannels);
      tmp = GetBELong(buf, i + 8);
      if (s->debug == 1) Snack_WriteLogInt("\tSSND chunk parsed", chunkLen);
      break;
    } else {
      chunkLen = 4;
      if (i > HEADBUF - 4) {
	Tcl_AppendResult(interp, "Missing chunk in AIFF header", NULL);
	return TCL_ERROR;
      }
    }
    i += chunkLen;
    if (s->firstNRead < i + 8) {
      if (GetHeaderBytes(s, interp, ch, buf, i + 8) != TCL_OK) {
	return TCL_ERROR;
      }
    }
  }
  s->headSize = i + 12 + tmp;
  SwapIfLE(s);

  return TCL_OK;
}

#define SNACK_AIFF_HEADERSIZE 54

int
PutAiffHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  char buf[HEADBUF];

  sprintf(&buf[0], "FORM");
  if (len != -1) {
    PutBELong(buf, 4, len * s->sampsize * s->nchannels + 46);
  } else {
    SwapIfLE(s);
    PutBELong(buf, 4, 0x7FFFFFFF);
  }
  sprintf(&buf[8], "AIFFCOMM");
  PutBELong(buf, 16, 18);
  PutBEShort(buf, 20, (short) s->nchannels);
  PutBELong(buf, 22, s->length);
  PutBEShort(buf, 26, (short) (s->sampsize * 8));
  StoreFloat((unsigned char *) &buf[28], (long) s->sampfreq);
  sprintf(&buf[38], "SSND");
  if (len != -1) {
    PutBELong(buf, 42, 8 + s->length * s->sampsize * s->nchannels);
  } else {
    PutBELong(buf, 42, 8 + 0x7FFFFFD1);
  }
  PutBELong(buf, 46, 0);
  PutBELong(buf, 50, 0);
  if (ch != NULL) {
    if (Tcl_Write(ch, buf, SNACK_AIFF_HEADERSIZE) == -1) return TCL_ERROR;
  } else {
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, SNACK_AIFF_HEADERSIZE);
      memcpy(obj->bytes, buf, SNACK_AIFF_HEADERSIZE);
    } else {
#ifdef TCL_81_API
      unsigned char *p = Tcl_SetByteArrayLength(obj, SNACK_AIFF_HEADERSIZE);
      memcpy(p, buf, SNACK_AIFF_HEADERSIZE);
#endif
    }
  }
  s->inByteOrder = SNACK_BIGENDIAN;

  return(SNACK_AIFF_HEADERSIZE);
}

static int
GetCSLHeader(Sound *s, Tcl_Interp *interp, Tcl_Channel ch, Tcl_Obj *obj,
	     char *buf)
{
  int tmp1, tmp2, nsamp = 0, nsampfile, i = 12, chunkLen;

  if (s->debug == 1) Snack_WriteLog("\tReading CSL header\n");

  /* buf[] = "FORMDS16xxxxHEDR" */

  while (1) {
    if (strncasecmp("HEDR", &buf[i], strlen("HEDR")) == 0) {
      chunkLen = GetLELong(buf, i + 4) + 8;
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      s->sampformat = LIN16;
      s->sampsize   = 2;
      s->nchannels  = 1;
      s->sampfreq   = GetLELong(buf, i+28);
      tmp1 = GetLEShort(buf, i+36);
      tmp2 = GetLEShort(buf, i+38);
      if (tmp1 != -1 && tmp2 != -1) {
	s->nchannels = 2;
      }
      if (s->debug == 1) Snack_WriteLogInt("\tHEDR block parsed", chunkLen);
    } else if (strncasecmp("HDR8", &buf[i], strlen("HDR8")) == 0) {
      chunkLen = GetLELong(buf, i + 4) + 8;
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      s->sampformat = LIN16;
      s->sampsize   = 2;
      s->nchannels  = 1;
      s->sampfreq   = GetLELong(buf, i+28);
      tmp1 = GetLEShort(buf, i+36);
      tmp2 = GetLEShort(buf, i+38);
      if (tmp1 != -1 && tmp2 != -1) {
	s->nchannels = 2;
      }
      if (s->debug == 1) Snack_WriteLogInt("\tHDR8 block parsed", chunkLen);
    } else if (strncasecmp("SDA_", &buf[i], strlen("SDA_")) == 0) {
      nsamp = GetLELong(buf, i + 4) / (s->sampsize * s->nchannels);
      s->nchannels  = 1;
      if (s->debug == 1) Snack_WriteLogInt("\tSDA_ block parsed", nsamp);
      break;
    } else if (strncasecmp("SD_B", &buf[i], strlen("SD_B")) == 0) {
      nsamp = GetLELong(buf, i + 4) / (s->sampsize * s->nchannels);
      s->nchannels  = 1;
      if (s->debug == 1) Snack_WriteLogInt("\tSD_B block parsed", nsamp);
      break;
    } else if (strncasecmp("SDAB", &buf[i], strlen("SDAB")) == 0) {
      nsamp = GetLELong(buf, i + 4) / (s->sampsize * s->nchannels);
      if (s->debug == 1) Snack_WriteLogInt("\tSDAB block parsed", nsamp);
      break;
    } else { /* unknown block */
      chunkLen = GetLELong(buf, i + 4) + 8;
      if (chunkLen & 1) chunkLen++;
      if (chunkLen < 0 || chunkLen > HEADBUF) {
	Tcl_AppendResult(interp, "Failed parsing CSL header", NULL);
	return TCL_ERROR;
      }
      if (s->firstNRead < i + chunkLen) {
	if (GetHeaderBytes(s, interp, ch, buf, i + chunkLen) != TCL_OK) {
	  return TCL_ERROR;
	}
      }
      if (s->debug==1) Snack_WriteLogInt("\tSkipping unknown block", chunkLen);
    }

    i += chunkLen;
    if (s->firstNRead < i + 8) {
      if (GetHeaderBytes(s, interp, ch, buf, i + 8) != TCL_OK) {
	return TCL_ERROR;
      }
    }
    if (i >= HEADBUF) {
      Tcl_AppendResult(interp, "Failed parsing CSL header", NULL);
      return TCL_ERROR;
    }
  }
  
  s->headSize = i + 8;
  if (ch != NULL) {
    Tcl_Seek(ch, 0, SEEK_END);
    nsampfile = (Tcl_Tell(ch) - s->headSize) / (s->sampsize * s->nchannels);
    if (nsampfile < nsamp || nsamp == 0) {
      nsamp = nsampfile;
    }
  }
  if (obj != NULL) {
    if (useOldObjAPI) {
      nsampfile = (obj->length - s->headSize) / (s->sampsize * s->nchannels);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      Tcl_GetByteArrayFromObj(obj, &length);
      nsampfile = (length - s->headSize) / (s->sampsize * s->nchannels);
#endif
    }
    if (nsampfile < nsamp || nsamp == 0) {
      nsamp = nsampfile;
    }
  }
  s->length = nsamp;
  SwapIfBE(s);

  return TCL_OK;
}

#define SNACK_CSL_HEADERSIZE 88
#define CSL_DATECOMMAND "clock format [clock seconds] -format {%b %d %T %Y}"

static int
PutCSLHeader(Sound *s, Tcl_Channel ch, Tcl_Obj *obj, int len)
{
  char buf[HEADBUF];

  sprintf(&buf[0], "FORMDS16");
  if (len != -1) {
    PutLELong(buf, 8, len * s->sampsize * s->nchannels + 78);
  } else {
    SwapIfBE(s);
    PutLELong(buf, 8, 0);
  }
  sprintf(&buf[12], "HEDR");
  PutLELong(buf, 16, 32);
  Tcl_GlobalEvalObj(s->interp, Tcl_NewStringObj(CSL_DATECOMMAND, -1));
  sprintf(&buf[20], Tcl_GetStringResult(s->interp));
  
  PutLELong(buf, 40, s->sampfreq);
  PutLELong(buf, 44, s->length);
  PutLEShort(buf, 48, (short) s->abmax);
  if (s->nchannels == 1) {
    PutLEShort(buf, 50, (short) -1);
  } else {
    PutLEShort(buf, 50, (short) s->abmax);
  }
  
  sprintf(&buf[52], "NOTE");
  PutLELong(buf, 56, 19);
  sprintf(&buf[60], "Created by Snack   ");

  if (s->nchannels == 1) {
    sprintf(&buf[80], "SDA_");
  } else {
    sprintf(&buf[80], "SDAB");
  }
  if (len != -1) {
    PutLELong(buf, 84, len * s->sampsize * s->nchannels);
  } else {
    PutLELong(buf, 84, 0);
  }
  if (ch != NULL) {
    if (Tcl_Write(ch, buf, SNACK_CSL_HEADERSIZE) == -1) return TCL_ERROR;
  } else {
    if (useOldObjAPI) {
      Tcl_SetObjLength(obj, SNACK_CSL_HEADERSIZE);
      memcpy(obj->bytes, buf, SNACK_CSL_HEADERSIZE);
    } else {
#ifdef TCL_81_API
      unsigned char *p = Tcl_SetByteArrayLength(obj, SNACK_CSL_HEADERSIZE);
      memcpy(p, buf, SNACK_CSL_HEADERSIZE);
#endif
    }
  }
  s->inByteOrder = SNACK_LITTLEENDIAN;

  return(SNACK_CSL_HEADERSIZE);
}

int
SnackOpenFile(openProc *openProc, Sound *s, Tcl_Interp *interp,
	      Tcl_Channel *ch, char *mode)
{
  if (openProc == NULL) {
    if ((*ch = Tcl_OpenFileChannel(interp, s->fcname, mode, 0)) == 0) {
      return TCL_ERROR;
    }
    Tcl_SetChannelOption(interp, *ch, "-translation", "binary");
#ifdef TCL_81_API
    Tcl_SetChannelOption(interp, *ch, "-encoding", "binary");
#endif
  } else {
    return((openProc)(s, interp, ch, mode));
  }

  return TCL_OK;
}

int
SnackCloseFile(closeProc *closeProc, Sound *s, Tcl_Interp *interp,
	       Tcl_Channel *ch)
{
  if (closeProc == NULL) {
    Tcl_Close(interp, *ch);
    *ch = NULL;
  } else {
    return((closeProc)(s, interp, ch));
  }

  return TCL_OK;
}

int
SnackSeekFile(seekProc *seekProc, Sound *s, Tcl_Interp *interp,
	      Tcl_Channel ch, int pos)
{
  if (seekProc == NULL) {
    Tcl_Seek(ch, pos, SEEK_SET);
  } else {
    return((seekProc)(s, interp, ch, pos));
  }

  return TCL_OK;
}

char *
LoadSound(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj, int startpos,
	  int endpos)
{
  Tcl_Channel ch = NULL;
  int status = TCL_OK;
  jkFileFormat *ff;
  int oldsampfmt = s->sampformat;
  
  if (s->debug == 1) Snack_WriteLog("\tEnter LoadSound\n");

  if (GetHeader(s, interp, obj) != TCL_OK) {
    return NULL;
  }
  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(s->fileType, ff->formatName) == 0) {
      if (obj == NULL) {
	status = SnackOpenFile(ff->openProc, s, interp, &ch, "r");
      }
      if (status == TCL_OK) {
	s->startPos = startpos * s->sampsize * s->nchannels;
	if (obj == NULL) {
	  status = SnackSeekFile(ff->seekProc, s, interp, ch,
				 s->headSize + s->startPos);
	}
      }
      if (status == TCL_OK) {
	if (s->active == WRITE && s->sampformat != oldsampfmt) {
	  Snack_StopSound(s, NULL);
	}
	status = ReadSound(ff->readProc, s, interp, ch, obj, endpos);
	if (s->swap && (s->sampsize == 2)) {
	  ByteSwapSound(s);
	}
      }
      if (status == TCL_OK && obj == NULL) {
	status = SnackCloseFile(ff->closeProc, s, interp, &ch);
      }
      if (status == TCL_ERROR) {
	return NULL;
      }
      break;
    }
  }
  Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  if (s->debug == 1) Snack_WriteLog("\tExit LoadSound\n");

  return(s->fileType);
}

int
SaveSound(Sound *s, Tcl_Interp *interp, char *filename, Tcl_Obj *obj,
	  int startpos, int len, char *type)
{
  Tcl_Channel ch = NULL;
  jkFileFormat *ff;
  int hdsize;

  if (s->debug == 1) Snack_WriteLog("Enter SaveSound\n");

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(type, ff->formatName) == 0) {
      if (ff->putHeaderProc != NULL) {
	if (filename != NULL) {
	  if ((ch = Tcl_OpenFileChannel(interp, filename, "w", 420)) == 0) {
	    return TCL_ERROR;
	  }
	  Tcl_SetChannelOption(interp, ch, "-translation", "binary");
#ifdef TCL_81_API
	  Tcl_SetChannelOption(interp, ch, "-encoding", "binary");
#endif
	}
	if ((hdsize = (ff->putHeaderProc)(s, ch, obj, len)) < 0) {
	  Tcl_AppendResult(interp, "Error while writing header", NULL);
	  return TCL_ERROR;
	}
	if (WriteSound(ff->writeProc, s, interp, ch, obj, startpos,
		       len, hdsize) != TCL_OK) {
	  Tcl_AppendResult(interp, "Error while writing", NULL);
	  return TCL_ERROR;
	}
      } else {
	Tcl_AppendResult(interp, "Unsupported save format", NULL);
	return TCL_ERROR;
      }
      break;
    }
  }

  if (ch != NULL) {
    Tcl_Close(interp, ch);
  }

  if (s->debug == 1) Snack_WriteLog("Exit SaveSound\n");

  return TCL_OK;
}

int
readCmd(Sound *s, Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[])
{
  char *filetype;
  int arg, startpos = 0, endpos = -1;
  static char *subOptionStrings[] = {
    "-frequency", "-skiphead", "-byteorder", "-channels",
    "-format", "-start", "-end", "-fileformat", "-guessproperties", 
    "-progress", NULL
  };
  enum subOptions {
    FREQUENCY, SKIPHEAD, BYTEORDER, CHANNELS, FORMAT, START, END,
    FILEFORMAT, GUESSPROPS, PROGRESS
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "read only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
  if (Tcl_IsSafe(interp)) {
    Tcl_AppendResult(interp, "can not read sound from a file in a safe",
		     " interpreter", (char *) NULL);
    return TCL_ERROR;
  }

  s->guessFormat = -1;
  s->guessFrequency = -1;
  s->swap = 0;

  for (arg = 3; arg < objc; arg+=2) {
    int index;
	
    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings, "option",
			    0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    switch ((enum subOptions) index) {
    case FREQUENCY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->sampfreq) != TCL_OK)
	  return TCL_ERROR;
	s->guessFrequency = 0;
	break;
      }
    case SKIPHEAD: 
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->skipBytes) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case BYTEORDER:
      {
	int length;
	char *str = Tcl_GetStringFromObj(objv[arg+1], &length);

	if (strncasecmp(str, "littleEndian", length) == 0) {
	  SwapIfBE(s);
	} else if (strncasecmp(str, "bigEndian", length) == 0) {
	  SwapIfLE(s);
	} else {
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian",
			   " or littleEndian", NULL);
	  return TCL_ERROR;
	}
	s->guessFormat = 0;
	break;
      }
    case CHANNELS:
      {
	if (GetChannels(interp, objv[arg+1], &s->nchannels) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case FORMAT:
      {
	if (GetFormat(interp, objv[arg+1], &s->sampformat, &s->sampsize) != TCL_OK)
	  return TCL_ERROR;
	s->guessFormat = 0;
	break;
      }
    case START:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &startpos) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case END:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &endpos) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case FILEFORMAT:
      {
	if (GetFileFormat(interp, objv[arg+1], &s->fileType) != TCL_OK)
	  return TCL_ERROR;
	s->forceFormat = 1;
	break;
      }
    case GUESSPROPS:
      {
	int guessProps;
	if (Tcl_GetBooleanFromObj(interp, objv[arg+1], &guessProps) != TCL_OK)
	  return TCL_ERROR;
	if (guessProps) {
	  if (s->guessFormat == -1) s->guessFormat = 1;
	  if (s->guessFrequency == -1) s->guessFrequency = 1;
	}
	break;
      }
    case PROGRESS:
      {
	Tcl_IncrRefCount(objv[arg+1]);
	s->cmdPtr = objv[arg+1];
	break;
      }
    }
  }
  if (s->guessFormat == -1) s->guessFormat = 0;
  if (s->guessFrequency == -1) s->guessFrequency = 0;
  if (startpos < 0) startpos = 0;
  if (startpos > endpos && endpos != -1) return TCL_OK;
  if (SetFcname(s, interp, objv[2]) != TCL_OK) {
    return TCL_ERROR;
  }
  if (strlen(s->fcname) == 0) {
    return TCL_OK;
  }
  filetype = LoadSound(s, interp, NULL, startpos, endpos);
      
  if (filetype == NULL) {
    return TCL_ERROR;
  } else {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(filetype, -1));
  }

  return TCL_OK;
}

int
writeCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int startpos = 0, endpos = s->length, arg, len;
  char *string, *filetype = NULL;
  static char *subOptionStrings[] = {
    "-start", "-end", "-fileformat", "-progress", "-byteorder", NULL
  };
  enum subOptions {
    START, END, FILEFORMAT, PROGRESS, BYTEORDER
  };

  if (s->debug == 1) { Snack_WriteLog("Enter writeCmd\n"); }

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "write only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
  if (Tcl_IsSafe(interp)) {
    Tcl_AppendResult(interp, "can not write sound to a file in a safe",
		     " interpreter", (char *) NULL);
    return TCL_ERROR;
  }

  s->inByteOrder = SNACK_NATIVE;

  for (arg = 3; arg < objc; arg+=2) {
    int index;
	
    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			    "option", 0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    switch ((enum subOptions) index) {
    case START:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &startpos) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case END:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &endpos) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case FILEFORMAT:
      {
	if (GetFileFormat(interp, objv[arg+1], &filetype) != TCL_OK)
	  return TCL_ERROR;
	break;	
      }
    case PROGRESS:
      {
	Tcl_IncrRefCount(objv[arg+1]);
	s->cmdPtr = objv[arg+1];
	break;
      }
    case BYTEORDER:
      {
	int length;
	char *str = Tcl_GetStringFromObj(objv[arg+1], &length);

	if (strncasecmp(str, "littleEndian", length) == 0) {
  	  s->inByteOrder = SNACK_LITTLEENDIAN;
	} else if (strncasecmp(str, "bigEndian", length) == 0) {
	  s->inByteOrder = SNACK_BIGENDIAN;
	} else {
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian",
			   " or littleEndian", NULL);
	  return TCL_ERROR;
	}
	break;
      }
    }
  }
  len = s->length;
  if (endpos >= len) endpos = len;
  if (endpos < 0)    endpos = len;
  if (endpos > startpos) len -= (len - endpos);
  if (startpos > endpos) return TCL_OK;
  if (startpos > 0) len -= startpos; else startpos = 0;
      
  if (objc < 3) {
    Tcl_AppendResult(interp, "No file name given", NULL);
    return TCL_ERROR;
  }
  string = Tcl_GetStringFromObj(objv[2], NULL);
  if (filetype == NULL) {
    filetype = NameGuessFileType(string);
  }
  if (strlen(string) == 0) {
    return TCL_OK;
  }
  if (SaveSound(s, interp, string, NULL, startpos, len, filetype) == TCL_ERROR) {
    return TCL_ERROR;
  }

  if (s->debug == 1) { Snack_WriteLog("Exit writeCmd\n"); }

  return TCL_OK;
} /* writeCmd */

int
dataCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int varflag = 0;

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "data only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
      
  if (objc > 2) {
    char *string = Tcl_GetStringFromObj(objv[2], NULL);
    
    if (string[0] == '-') {
      varflag = 1;
    }
  } else {
    varflag = 1;
  }
  
  if (varflag) { /* sound -> variable */
    Tcl_Obj *new = Tcl_NewObj();
    char *filetype = s->fileType;
    int arg, len, startpos = 0, endpos = s->length;
    static char *subOptionStrings[] = {
      "-fileformat", "-start", "-end", "-byteorder",
      NULL
    };
    enum subOptions {
      FILEFORMAT, START, END, BYTEORDER
    };

    s->swap = 0;

    for (arg = 2; arg < objc; arg += 2) {
      int index;
      char *str;
      
      if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			      "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
      }
      
      switch ((enum subOptions) index) {
      case FILEFORMAT:
	{
	  if (GetFileFormat(interp, objv[arg+1], &filetype) != TCL_OK)
	    return TCL_ERROR;
	  break;

	}
      case START:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &startpos) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case END:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &endpos) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case BYTEORDER:
	{
	  str = Tcl_GetStringFromObj(objv[arg+1], &len);
	  if (strncasecmp(str, "littleEndian", len) == 0) {
	    SwapIfBE(s);
	  } else if (strncasecmp(str, "bigEndian", len) == 0) {
	    SwapIfLE(s);
	  } else {
	    Tcl_AppendResult(interp,
	       "-byteorder option should be bigEndian or littleEndian", NULL);
	    return TCL_ERROR;
	  }
	  break;
	}
      }
    }
    
    len = s->length;
    if (endpos >= len) endpos = len;
    if (endpos < 0)    endpos = len;
    if (endpos > startpos) len -= (len - endpos);
    if (startpos > endpos) return TCL_OK;
    if (startpos > 0) len -= startpos; else startpos = 0;

    if (SaveSound(s, interp, NULL, new, startpos, len, filetype)
	== TCL_ERROR) {
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, new);
  } else { /* variable -> sound */
    int arg, startpos = 0, endpos = -1;
    char *filetype;
    static char *subOptionStrings[] = {
      "-frequency", "-skiphead", "-byteorder",
      "-channels", "-format", "-start", "-end", "-fileformat",
      "-guessproperties", NULL
    };
    enum subOptions {
      FREQUENCY, SKIPHEAD, BYTEORDER, CHANNELS, FORMAT, START, END,
      FILEFORMAT, GUESSPROPS
    };

    s->guessFormat = -1;
    s->guessFrequency = -1;
    s->swap = 0;

    for (arg = 3; arg < objc; arg += 2) {
      int index;
      
      if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			      "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
      }
	
      switch ((enum subOptions) index) {
      case FREQUENCY:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->sampfreq) != TCL_OK)
	    return TCL_ERROR;
	  s->guessFrequency = 0;
	  break;
	}
      case SKIPHEAD: 
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->skipBytes) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  break;
	}
      case BYTEORDER:
	{
	  int length;
	  char *str = Tcl_GetStringFromObj(objv[arg+1], &length);
	      
	  if (strncasecmp(str, "littleEndian", length) == 0) {
	    SwapIfBE(s);
	  } else if (strncasecmp(str, "bigEndian", length) == 0) {
	    SwapIfLE(s);
	  } else {
	    Tcl_AppendResult(interp, "-byteorder option should be bigEndian",
			     " or littleEndian", NULL);
	    return TCL_ERROR;
	  }
	  s->guessFormat = 0;
	  break;
	}
      case CHANNELS:
	{
	  if (GetChannels(interp, objv[arg+1], &s->nchannels) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case FORMAT:
	{
	  if (GetFormat(interp, objv[arg+1], &s->sampformat, &s->sampsize)
	      != TCL_OK)
	    return TCL_ERROR;
	  s->guessFormat = 0;
	  break;
	}
      case START:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &startpos) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case END:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &endpos) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case FILEFORMAT:
	{
	  if (GetFileFormat(interp, objv[arg+1], &s->fileType) != TCL_OK)
	    return TCL_ERROR;
	  s->forceFormat = 1;
	  break;
	}
      case GUESSPROPS:
	{
	  int guessProps;
	  if (Tcl_GetBooleanFromObj(interp, objv[arg+1], &guessProps) !=TCL_OK)
	    return TCL_ERROR;
	  if (guessProps) {
	    if (s->guessFormat == -1) s->guessFormat = 1;
	    if (s->guessFrequency == -1) s->guessFrequency = 1;
	  }
	  break;
	}
      }
    }
    if (s->guessFormat == -1) s->guessFormat = 0;
    if (s->guessFrequency == -1) s->guessFrequency = 0;
    if (startpos < 0) startpos = 0;
    if (startpos > endpos && endpos != -1) return TCL_OK;
    filetype = LoadSound(s, interp, objv[2], startpos, endpos);
    if (filetype == NULL) {
      return TCL_ERROR;
    } else {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(filetype, -1));
      return TCL_OK;
    }
  }

  return TCL_OK;
} /* dataCmd */

void
AddSnackNativeFormats()
{
  Snack_AddFileFormat(RAW_STRING, GuessRawFile, GetRawHeader,
		      NULL, PutRawHeader, NULL, NULL, NULL, NULL, NULL);

  Snack_AddFileFormat(SMP_STRING, GuessSmpFile, GetSmpHeader,
		      ExtSmpFile, PutSmpHeader, NULL, NULL, NULL, NULL, NULL);

  Snack_AddFileFormat(CSL_STRING, GuessCSLFile, GetCSLHeader,
		      ExtCSLFile, PutCSLHeader, NULL, NULL, NULL, NULL, NULL);

  Snack_AddFileFormat(SD_STRING, GuessSdFile, GetSdHeader,
		      ExtSdFile, NULL, NULL, NULL, NULL, NULL, NULL);

  Snack_AddFileFormat(AIFF_STRING, GuessAiffFile, GetAiffHeader,
		      ExtAiffFile, PutAiffHeader, NULL, NULL, NULL, NULL,NULL);

  Snack_AddFileFormat(AU_STRING, GuessAuFile, GetAuHeader,
		      ExtAuFile, PutAuHeader, NULL, NULL, NULL, NULL, NULL);

  Snack_AddFileFormat(MP3_STRING, GuessMP3File, GetMP3Header, ExtMP3File, NULL,
			     NULL, NULL, ReadMP3Samples, NULL, SeekMP3File);

  Snack_AddFileFormat(WAV_STRING, GuessWavFile, GetWavHeader,
		      ExtWavFile, PutWavHeader, NULL, NULL, NULL, NULL, NULL);
}

int
GetHeader(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj)
{
  jkFileFormat *ff;
  Tcl_Channel ch = NULL;
  int status = TCL_OK;
  int buflen = max(HEADBUF, NFIRSTSAMPLES * 2), len = 0;

  if (s->guessFormat) {
    s->swap = 0;
  }
  if ((s->tmpbuf = (short *) ckalloc(buflen)) == NULL) {
    Tcl_AppendResult(interp, "Could not allocate buffer!", NULL);
    return TCL_ERROR;
  }
  if (obj == NULL) {
    ch = Tcl_OpenFileChannel(interp, s->fcname, "r", 0);
    if (ch != NULL) {
      Tcl_SetChannelOption(interp, ch, "-translation", "binary");
#ifdef TCL_81_API
      Tcl_SetChannelOption(interp, ch, "-encoding", "binary");
#endif
      if ((len = Tcl_Read(ch, (char *)s->tmpbuf, buflen)) > 0) {
	Tcl_Close(interp, ch);
	ch = NULL;
      }
    } else {
      return TCL_ERROR;
    }
  } else {
    unsigned char *ptr = NULL;

    if (useOldObjAPI) {
      len = min(obj->length, buflen);
      memcpy((char *)s->tmpbuf, obj->bytes, len);
    } else {
#ifdef TCL_81_API
      int length = 0;
      
      ptr = Tcl_GetByteArrayFromObj(obj, &length);
      len = min(length, buflen);
      memcpy((char *)s->tmpbuf, ptr, len);
#endif
    }
  }
  if (s->forceFormat == 0) {
    s->fileType = GuessFileType((char *)s->tmpbuf, len, 1);
  }
  s->firstNRead = len;

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(s->fileType, ff->formatName) == 0) {
      if (obj == NULL) {
	status = SnackOpenFile(ff->openProc, s, interp, &ch, "r");
      }
      if (status == TCL_OK) {
	status = (ff->getHeaderProc)(s, interp, ch, obj, (char *)s->tmpbuf);
      }
      if (strcmp(s->fileType, RAW_STRING) == 0 && s->guessFormat) {
	GuessFormat(s, (unsigned char *)s->tmpbuf, len);
      }
      if (obj == NULL && status == TCL_OK) {
	status = SnackCloseFile(ff->closeProc, s, interp, &ch);
	Tcl_Close(interp, ch);
      }
      ckfree((char *)s->tmpbuf);
      s->tmpbuf = NULL;

      return(status);
    }
  }
  ckfree((char *)s->tmpbuf);
  s->tmpbuf = NULL;

  return TCL_OK;
}

void
PutHeader(Sound *s)
{
  jkFileFormat *ff;

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(s->fileType, ff->formatName) == 0) {
      if (ff->putHeaderProc != NULL) {
	(ff->putHeaderProc)(s, s->recchan, NULL, -1);
      }
      break;
    }
  }
}

int
GetFileFormat(Tcl_Interp *interp, Tcl_Obj *obj, char **filetype)
{
  int length;
  char *str = Tcl_GetStringFromObj(obj, &length);
  jkFileFormat *ff;

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcasecmp(str, ff->formatName) == 0) {
      *filetype = ff->formatName;
      return TCL_OK;
    }
  }

  if (strcasecmp(str, RAW_STRING) == 0) {
    *filetype = RAW_STRING;
    return TCL_OK;
  }

  Tcl_AppendResult(interp, "Unknown file format", NULL);

  return TCL_ERROR;
}

#define BACKLOGSAMPS 1

int
OpenLinkedFile(Sound *s, SnackLinkedFileInfo *infoPtr)
{
  jkFileFormat *ff;

  infoPtr->sound = s;

  if (strlen(s->fcname) == 0) {
    return TCL_OK;
  }
  if (s->itemRefCnt && s->active == READ) {
    return TCL_OK;
  }

  infoPtr->buffer = (unsigned char *) ckalloc(ITEMBUFFERSIZE);
  infoPtr->filePos = -1;

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(s->fileType, ff->formatName) == 0) {
      if (SnackOpenFile(ff->openProc, s, s->interp, &infoPtr->linkCh, "r")
	  != TCL_OK) {
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return TCL_ERROR;
}

void
CloseLinkedFile(SnackLinkedFileInfo *infoPtr)
{
  Sound *s = infoPtr->sound;
  jkFileFormat *ff;

  if (strlen(s->fcname) == 0) {
    return;
  }
  if (s->itemRefCnt && s->active == READ) {
    return;
  }

  ckfree((char *) infoPtr->buffer);

  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
    if (strcmp(s->fileType, ff->formatName) == 0) {
      SnackCloseFile(ff->closeProc, s, s->interp, &infoPtr->linkCh);
      return;
    }
  }
}

short
GetSample(SnackLinkedFileInfo *infoPtr, int index)
{
  Sound *s = infoPtr->sound;
  jkFileFormat *ff;
  int nRead, size = ITEMBUFFERSIZE, i;

  if (s->itemRefCnt && s->active == READ) {
    return SSAMPLE(s, index);
  }

  if (index < infoPtr->filePos + ITEMBUFFERSIZE / s->sampsize
      && index >= infoPtr->filePos && infoPtr->filePos != -1) {
    if (s->sampsize == 2) {
      return(((short *)infoPtr->buffer)[index-infoPtr->filePos]);
    } else if (s->sampformat == LIN8OFFSET) {
      return(infoPtr->buffer[index-infoPtr->filePos]);
    } else /*if (s->sampformat == LIN8)*/ {
      return(((char *)infoPtr->buffer)[index-infoPtr->filePos]);
    }
  } else {
    int pos = 0;

    /* Keep BACKLOGSAMPS old samples in the buffer */

    if (index > BACKLOGSAMPS) {
      index -= BACKLOGSAMPS;
      pos = BACKLOGSAMPS * s->nchannels;
    }
    for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
      if (strcmp(s->fileType, ff->formatName) == 0) {
	SnackSeekFile(ff->seekProc, s, s->interp, infoPtr->linkCh, 
		      s->headSize + index * s->sampsize);
	if (ff->readProc == NULL) {
	  nRead = Tcl_Read(infoPtr->linkCh, (char *)infoPtr->buffer, size);
	} else {
	  nRead = (ff->readProc)(s, s->interp, infoPtr->linkCh, NULL,
				 (char *)infoPtr->buffer, size);
	}
	if (s->sampsize == 2 && s->swap)
	  for (i = 0; i < nRead/2; i++)
	    ((short *)infoPtr->buffer)[i] =
		      Snack_SwapShort(((short *)infoPtr->buffer)[i]);
	break;
      }
    }
    infoPtr->filePos = index;
    if (s->sampsize == 2) {
      return(((short *)infoPtr->buffer)[pos]);
    } else {
      return(infoPtr->buffer[pos]);
    }
  }
}

/*
  A special hack for the Transcriber package on Windows.
  http://www.etca.fr/CTA/gip/Projets/Transcriber/
 */

jkFileFormat *
Snack_GetFileFormats()
{
  return snackFileFormats;
}

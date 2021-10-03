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
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "jkAudIO.h"
#include "jkSound.h"

#if defined Linux || defined WIN || defined _LITTLE_ENDIAN
#  define LE
#endif

#ifdef MAC_SNACK
#define FIXED_READ_CHUNK 1
#endif /* MAC_SNACK */

static int rop = IDLE;
static int wop = IDLE;
static ADesc adi;
static ADesc ado;

extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);

int
Snack_AddCallback(Sound *s, updateProc *proc, ClientData cd)
{
  jkCallback *cb = (jkCallback *) ckalloc(sizeof(jkCallback));

  if (cb == NULL) return(-1);
  cb->proc = proc;
  cb->clientData = cd;
  if (s->firstCB != NULL) {
    cb->id = s->firstCB->id + 1;
  } else {
    cb->id = 1;
  }
  cb->next = s->firstCB;
  s->firstCB = cb;

  if (s->debug == 1) { Snack_WriteLogInt("\tSnack_AddCallback", cb->id); }

  return(cb->id);
}

void
Snack_RemoveCallback(Sound *s, int id)
{
  jkCallback *cb = s->firstCB, *prev = cb;

  if (s->debug == 1) Snack_WriteLogInt("\tSnack_RemoveCallback", id);

  if (id == -1) return;
  if (cb->id == id) {
    s->firstCB = cb->next;
    ckfree((char *)cb);
    return;
  }

  for (cb = cb->next; cb != NULL; cb = cb->next) {
    if (cb->id == id) {
      prev->next = cb->next;
      ckfree((char *)cb);
    }
    prev = cb;
  }
}

void
Snack_ExecCallbacks(Sound *s, int flag)
{
  jkCallback *cb;

  if (s->debug == 1) Snack_WriteLog("\tEnter Snack_ExecCallbacks\n");

  for (cb = s->firstCB; cb != NULL; cb = cb->next) {
    if (s->debug == 1) Snack_WriteLogInt("\tExecuting callback", cb->id);
    (cb->proc)(cb->clientData, flag);
    if (s->debug == 1) Snack_WriteLog("\tdone\n");
  }
/*  if (s->monitorCmdPtr != NULL) {
    Tcl_Obj *tmp = Tcl_DuplicateObj(s->monitorCmdPtr);
    Tcl_Preserve((ClientData) s->interp);
    if (flag == SNACK_NEW_SOUND) {
      Tcl_AppendStringsToObj(tmp, " New", (char *) NULL);
    } else {
      Tcl_AppendStringsToObj(tmp, " More", (char *) NULL);
    }
    if (Tcl_GlobalEvalObj(s->interp, tmp) != TCL_OK) {
      Tcl_AddErrorInfo(s->interp, "\n    (\"command\" script)");
      Tcl_BackgroundError(s->interp);
    }
    Tcl_Release((ClientData) s->interp);
  }*/
}

void
Snack_GetExtremes(Sound *s, SnackLinkedFileInfo *info, int start, int end,
		  int chan, int *pmax, int *pmin)
{
  int i, maxs, mins, inc;

  if (chan == -1) {
    inc = 1;
    chan = 0;
  } else {
    inc = s->nchannels;
  }

  start = start * s->nchannels + chan;
  end   = end * s->nchannels + chan;

  if (s->sampformat == LIN8OFFSET) {
    maxs = 128;
    mins = 128;
  } else {
    maxs = 0;
    mins = 0;
  }

  if (s->length == 0) {
    *pmax = maxs;
    *pmin = mins;
    return;
  }

  switch (s->sampformat) {
  case LIN16:
    if (s->storeType == SOUND_IN_MEMORY) {
      for (i = start; i <= end; i += inc) {
	short tmp = SSAMPLE(s, i);
	if (tmp > maxs) {
	  maxs = tmp;
	}
	if (tmp < mins) {
	  mins = tmp;
	}
      }
    } else {
      for (i = start; i <= end; i += inc) {
	short tmp = GetSample(info, i);
	if (tmp > maxs) {
	  maxs = tmp;
	}
	if (tmp < mins) {
	  mins = tmp;
	}
      }
    }
    break;
  case ALAW:
    if (s->storeType == SOUND_IN_MEMORY) {
      for (i = start; i <= end; i += inc) {
	int tmp = Snack_Alaw2Lin(UCSAMPLE(s, i));
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    } else {
      for (i = start; i <= end; i += inc) {
	int tmp = Snack_Alaw2Lin((unsigned char)GetSample(info, i));
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    }
    break;
  case MULAW:
    if (s->storeType == SOUND_IN_MEMORY) {
      for (i = start; i <= end; i += inc) {
	int tmp = Snack_Mulaw2Lin(UCSAMPLE(s, i));
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    } else {
      for (i = start; i <= end; i += inc) {
	int tmp = Snack_Mulaw2Lin((unsigned char)GetSample(info, i));
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    }
    break;
  case LIN8OFFSET:
    if (s->storeType == SOUND_IN_MEMORY) {
      for (i = start; i <= end; i += inc) {
	unsigned char tmp = UCSAMPLE(s, i);
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    } else {
      for (i = start; i <= end; i += inc) {
	unsigned char tmp = (unsigned char) GetSample(info, i);
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    }
    break;
  default:
    if (s->storeType == SOUND_IN_MEMORY) {
      for (i = start; i <= end; i += inc) {
	char tmp = CSAMPLE(s, i);
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    } else {
      for (i = start; i <= end; i += inc) {
	char tmp = (char )GetSample(info, i);
	if (tmp > maxs) maxs = tmp;
	if (tmp < mins) mins = tmp;
      }
    }
  }

  *pmax = maxs;
  *pmin = mins;
}

void
Snack_UpdateExtremes(Sound *s, int start, int end, int flag)
{
  int maxs, mins, newmax, newmin;

  if (flag == SNACK_NEW_SOUND) {
    if (s->sampformat == LIN8OFFSET) {
      s->maxsamp = 128;
      s->minsamp = 128;
    } else {
      s->maxsamp = 0;
      s->minsamp = 0;
    }
  }

  maxs = s->maxsamp;
  mins = s->minsamp;

  Snack_GetExtremes(s, NULL, start, end - 1, -1, &newmax, &newmin);

  if (newmax > maxs) {
    s->maxsamp = newmax;
  } else {
    s->maxsamp = maxs;
  }
  if (newmin < mins) {
    s->minsamp = newmin;
  } else {
    s->minsamp = mins;
  }
  if (s->maxsamp > -s->minsamp)
    s->abmax = s->maxsamp;
  else
    s->abmax = -s->minsamp;
}

short
Snack_SwapShort(short s)
{
  char tc, *p;

  p = (char *) &s;
  tc = *p;
  *p = *(p+1);
  *(p+1) = tc;
  
  return(s);
}

long
SwapLong(long l)
{
  char tc, *p;

  p = (char *) &l;
  tc = *p;
  *p = *(p+3);
  *(p+3) = tc;

  tc = *(p+1);
  *(p+1) = *(p+2);
  *(p+2) = tc;
  
  return(l);
}

void
ByteSwapSound(Sound *s)
{
  int i, j;

  for (j = 0; j < s->nblks; j++)
    for (i = 0; i < min(SBLKSIZE, s->length * s->nchannels); i++)
      s->blocks[j][i] = Snack_SwapShort(s->blocks[j][i]);
}

void
Snack_DeleteSound(Sound *s)
{
  Snack_ResizeSoundStorage(s, 0);
  ckfree((char *) s->blocks);
  if (s->linkInfo.linkCh != NULL) {
    CloseLinkedFile(&s->linkInfo);
  }
  if (s->fcname != NULL) ckfree((char *)s->fcname);
  ckfree((char *) s);
}

int
Snack_ResizeSoundStorage(Sound *s, int len)
{
  int neededblks, i;

  if (s->debug == 1) Snack_WriteLogInt("Enter ResizeSoundStorage", len);

  if (s->sampformat == LIN16) {
    neededblks = 1 + len * s->nchannels / SBLKSIZE;
  } else {
    neededblks = 1 + len * s->nchannels / CBLKSIZE;
  }
  if (len == 0) neededblks = 0;

  if (neededblks > s->maxblks) {
    void *tmp = realloc(s->blocks, neededblks * sizeof(short*));
    if (tmp == NULL) {
      if (s->debug == 2) Snack_WriteLogInt("  realloc failed", neededblks);
      return TCL_ERROR;
    }
    s->maxblks = neededblks;
    s->blocks = (short **)tmp;
  }

  if (neededblks > s->nblks) {
    for (i = s->nblks; i < neededblks; i++) {
      if ((s->blocks[i] = (short *) ckalloc(CBLKSIZE)) == NULL) {
	break;
      }
    }
    if (i < neededblks) {
      if (s->debug == 2) Snack_WriteLogInt("  block alloc failed", i);
      for (--i; i >= s->nblks; i--) {
	ckfree((char *) s->blocks[i]);
      }
      return TCL_ERROR;
    }
  }

  if (neededblks < s->nblks) {
    for (i = neededblks; i < s->nblks; i++) {
      ckfree((char *) s->blocks[i]);
    }
  }

  if (s->sampformat == LIN16) {
    s->maxlength = neededblks * SBLKSIZE / s->nchannels;
  } else {
    s->maxlength = neededblks * CBLKSIZE / s->nchannels;
  }
  s->nblks = neededblks;

  if (s->debug == 1) Snack_WriteLogInt("Exit ResizeSoundStorage", neededblks);

  return TCL_OK;
}

char *ssfmt[] = { "", "Lin16", "Alaw", "Mulaw", "Lin8offset", "Lin8" };

#define FPS 32
#define RECGRAIN 10
#define BUFSCROLLSIZE 100000

static void
RecCallback(ClientData clientData)
{
  register Sound *s = (Sound *) clientData;
  int nRead = 0, sampsleft = SnackAudioReadable(&adi);
  int size = s->sampfreq / FPS;

  if (s->debug == 1) Snack_WriteLogInt("Enter RecCallback", sampsleft);

  if (s->recchan) { /* sound from file or channel */
    int i;

    if (sampsleft > size * 2) size *= 2;
    if (sampsleft > size * 2) size = sampsleft;
    if (size > s->buffersize / (s->sampsize * s->nchannels)) 
      size = s->buffersize / (s->sampsize * s->nchannels);
    nRead = SnackAudioRead(&adi, s->tmpbuf, size);

    if ((s->length + nRead - s->validStart) * s->sampsize * s->nchannels > CBLKSIZE) {
      s->validStart += (BUFSCROLLSIZE / (s->sampsize * s->nchannels));
      memmove(&((unsigned char **)s->blocks)[0][0],
	      &((unsigned char **)s->blocks)[0][BUFSCROLLSIZE], CBLKSIZE-BUFSCROLLSIZE);
    }

    memmove(&((unsigned char **)s->blocks)[0][(s->length - s->validStart) * s->sampsize \
					     * s->nchannels], s->tmpbuf, nRead * \
	    s->sampsize * s->nchannels);
    if (s->sampsize == 2 && s->swap)
      for (i = 0; i < (nRead * s->sampsize * s->nchannels) / 2; i++)
	s->tmpbuf[i] = Snack_SwapShort(s->tmpbuf[i]);
    Tcl_Write(s->recchan, (char *)s->tmpbuf, nRead *s->sampsize *s->nchannels);
    Tcl_Flush(s->recchan);

  } else { /* sound in memory */

    if (s->length > s->maxlength - max(sampsleft, 2 * size)) {
      if (Snack_ResizeSoundStorage(s, s->length + max(sampsleft, 2 * size)) != TCL_OK) {
	return;
      }
    }

    if (sampsleft > size * 2) size *= 2;
    if (sampsleft > size * 2) size = sampsleft;
    
    if (s->sampformat == LIN16) {
      int n = (s->length * s->nchannels) >> SEXP;
      int i = (s->length * s->nchannels) - (n << SEXP);
      
      if (size * s->nchannels + i > SBLKSIZE) {
	size = (SBLKSIZE - i) / s->nchannels;
      }
      nRead = SnackAudioRead(&adi, &s->blocks[n][i], size);
    } else {
      int n = (s->length * s->nchannels) >> CEXP;
      unsigned char *p = (unsigned char *) s->blocks[n];
      int i = (s->length * s->nchannels) - (n << CEXP);
      
      if (size * s->nchannels + i > CBLKSIZE) {
	size = (CBLKSIZE - i) / s->nchannels;
      }
#ifdef FIXED_READ_CHUNK
      size = s->smpfreq/16;
#endif
      nRead = SnackAudioRead(&adi, (short *)&p[i], size);
    }
  }
  if (nRead > 0) {
    if (s->storeType == SOUND_IN_MEMORY) {
      Snack_UpdateExtremes(s, s->length, s->length + nRead, SNACK_MORE_SOUND);
    }
    s->length += nRead;
    Snack_ExecCallbacks(s, SNACK_MORE_SOUND);
  }
  
  s->rtoken = Tcl_CreateTimerHandler(RECGRAIN, (Tcl_TimerProc *) RecCallback,
				     (int *)s);
  if (s->debug == 1) Snack_WriteLogInt("Exit RecCallback", nRead);
}

#define EXEC_AND_CLEAN 1
#define CLEAN_ONLY     0

static void
ExecSoundCmd(Sound *s, int flag)
{
  Tcl_Interp *interp = s->interp;

  if (s->cmdPtr != NULL) {
    if (flag == EXEC_AND_CLEAN) {
      Tcl_Preserve((ClientData) interp);
      if (Tcl_GlobalEvalObj(interp, s->cmdPtr) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (\"command\" script)");
	Tcl_BackgroundError(interp);
      }
      Tcl_Release((ClientData) interp);
    }
    s->cmdPtr = NULL;
  }
}

typedef struct jkQueuedSound {
  Sound *sound;
  int startPos;
  int totLen;
  Tcl_Obj *cmdPtr;
  int done;
  int execd;
  int id;
  struct jkQueuedSound *next;
} jkQueuedSound;

static struct jkQueuedSound *soundQueue = NULL;
static int corr = 0;
static Sound *sCurr = NULL;

static void
CleanSoundQueue()
{
  jkQueuedSound *p, *q;

  if (soundQueue == NULL) return;

  p = soundQueue;
  do {
    q = p->next;
    p->sound->active = IDLE;
    if (p->cmdPtr != NULL) Tcl_DecrRefCount(p->cmdPtr);
    if (p->sound->destroy) {
      Snack_DeleteSound(p->sound);
    }
    ckfree((char *)p);
    p = q;
  } while (p != NULL);

  soundQueue = NULL;
}

#define IPLAYGRAIN 0
#define PLAYGRAIN 100
#define BUFSECS 4

extern struct jkFileFormat *snackFileFormats;

static int
AssembleSoundChunk(int inSize)
{
  int nWritten = 1, writeSize = 0, outSize = 0, size = inSize;
  jkQueuedSound *p;
  Sound *s = sCurr;

  if (s->debug == 1) Snack_WriteLogInt("Enter AssembleSoundChunk", size);

  while (size > 0 && nWritten > 0) {
    nWritten = 0;
    if (s->storeType == SOUND_IN_MEMORY) { /* sound in memory */
      if ((s->nPlayed < s->totLen) &&
	  (s->startPos + s->nPlayed < s->length)) {
	if (s->sampformat == LIN16) {
	  int n = ((s->startPos + s->nPlayed) * s->nchannels) >> SEXP;
	  int i = ((s->startPos + s->nPlayed) * s->nchannels) - (n << SEXP);
	
	  if (size * s->nchannels + i > SBLKSIZE) {
	    writeSize = (SBLKSIZE - i) / s->nchannels;
	  } else {
	    writeSize = size;
	  }
	  if (writeSize > s->length - s->startPos - s->nPlayed) {
	    writeSize = s->length - s->startPos - s->nPlayed;
	  }
	  if (writeSize > s->totLen - s->nPlayed) {
	    writeSize = s->totLen - s->nPlayed;
	  }
	  nWritten = SnackAudioWrite(&ado, &s->blocks[n][i], writeSize);
	} else {
	  int n = ((s->startPos + s->nPlayed) * s->nchannels) >> CEXP;
	  int i = ((s->startPos + s->nPlayed) * s->nchannels) - (n << CEXP);
	  unsigned char *p = (unsigned char *) s->blocks[n];
	  
	  if (size * s->nchannels + i > CBLKSIZE) {
	    writeSize = (CBLKSIZE - i) / s->nchannels;
	  } else {
	    writeSize = size;
	  }
	  if (writeSize > s->length - s->startPos - s->nPlayed) {
	    writeSize = s->length - s->startPos - s->nPlayed;
	  }
	  if (writeSize > s->totLen - s->nPlayed) {
	    writeSize = s->totLen - s->nPlayed;
	  }
	  nWritten = SnackAudioWrite(&ado, &p[i], writeSize);
	}
      }
    } else { /* sound in file or channel */
      int nRead = 0, i;
      jkFileFormat *ff;

      if (s->nPlayed < s->totLen ||
	  (s->totLen == 0 &&
	   (s->rwchan == NULL || (s->rwchan != NULL&&!Tcl_Eof(s->rwchan))))) {
	if (s->totLen != 0 && size > s->totLen - s->nPlayed) {
	  size = s->totLen - s->nPlayed;
	}
	for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
	  if (strcmp(s->fileType, ff->formatName) == 0) {
	    int status;
	    
	    if (s->rwchan == NULL && s->storeType == SOUND_IN_FILE) {
	      status = SnackOpenFile(ff->openProc, s, s->interp,
				     &s->rwchan, "r");
	      if (status == TCL_OK) {
		status = SnackSeekFile(ff->seekProc, s, s->interp,s->rwchan, 
				       s->headSize+s->startPos*s->sampsize*s->nchannels);
	      } else {
		nRead = 0;
		break;
	      }
	    }
	    if (ff->readProc == NULL) {
	      nRead = Tcl_Read(s->rwchan, (char *)s->tmpbuf, size * 
			       s->sampsize * s->nchannels);
	    } else {
	      nRead = (ff->readProc)(s, s->interp, s->rwchan, NULL,
				     (char *)s->tmpbuf, size * 
				     s->sampsize * s->nchannels);
	    }
	    break;
	  }
	}
	if (s->sampsize == 2 && s->swap)
	  for (i = 0; i < nRead/2; i++)
	    s->tmpbuf[i] = Snack_SwapShort(s->tmpbuf[i]);
	
	if (s->debug == 1) Snack_WriteLogInt("Tcl_Read", nRead);
	
	if (nRead > 0) {
	  nWritten = SnackAudioWrite(&ado, s->tmpbuf, nRead / (s->sampsize*s->nchannels));
	  writeSize = nWritten;
	} else {
	  s->totLen = s->nPlayed;
	}
      } else { /* s->nPlayed == s->totLen or EOF */
	if (s->rwchan != NULL) {
	  if (s->storeType == SOUND_IN_FILE) {
	    for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
	      if (strcmp(s->fileType, ff->formatName) == 0) {
		SnackCloseFile(ff->closeProc, s, s->interp, &s->rwchan);
		s->rwchan = NULL;
		break;
	      }
	    }
	  }
	}
      }
    }
    size -= writeSize;
    s->nPlayed += nWritten;
    outSize += nWritten;

    if (outSize < inSize && s->storeType != SOUND_IN_CHANNEL) {
      for (p = soundQueue; p->done == 1 && p->next != NULL; p = p->next);

      if (p != NULL && p->done == 0) {
	int len = s->totLen;

	corr += s->nPlayed;
	p->done = 1;
	sCurr = p->sound;
	s = p->sound;
	s->totLen += len;
	s->startPos = p->startPos;
	s->cmdPtr = p->cmdPtr;
	s->nPlayed = 0;
	s->active = WRITE;
	ado.debug = s->debug;
      }
    }
  }
  if (s->debug == 1) Snack_WriteLogInt("Exit AssembleSoundChunk", outSize);

  return outSize;
}

static void
PlayCallback(ClientData clientData)
{
  register Sound *s = (Sound *) clientData;
  int currPlayed, writeable, totPlayed = 0, count = 0, closeDown = 0, size;
  jkQueuedSound *p;

  do {
    totPlayed = SnackAudioPlayed(&ado);
    currPlayed = totPlayed - corr;
    writeable = SnackAudioWriteable(&ado);

    if (s->debug == 1) Snack_WriteLogInt("PlayCallback", totPlayed);

    if (s->nPlayed - currPlayed < BUFSECS * s->sampfreq) {
      if (s->storeType == SOUND_IN_MEMORY) {
	size = s->sampfreq / 2;
      } else {
	size = s->buffersize;
      }
      if (writeable >= 0 && writeable < size) {
	size = writeable;
      }

      if (AssembleSoundChunk(size) < size) {
	static int oplayed = -1;
      
	SnackAudioPost(&ado);
	if (s->nPlayed - currPlayed <= 0 || currPlayed == oplayed) {
	  if (SnackAudioClose(&ado) != -1) {
	    s->active = IDLE;
	    closeDown = 1;
	    oplayed = -1;
	    break;
	  }
	} else {
	  oplayed = currPlayed;
	}
      }
    }
  } while (s->blockingPlay);

  for (p = soundQueue; p->done == 1 && p->next != NULL; p = p->next) {
    count += p->totLen;

    if (p->execd == 0 && totPlayed >= count) {
      Tcl_Interp *interp = p->sound->interp;

      if (p->cmdPtr != NULL) {
	Tcl_Preserve((ClientData) interp);
	if (Tcl_GlobalEvalObj(interp, p->cmdPtr) != TCL_OK) {
	  Tcl_AddErrorInfo(interp, "\n    (\"command\" script)");
	  Tcl_BackgroundError(interp);
	}
	Tcl_Release((ClientData) interp);
	p->cmdPtr = NULL;
      }
      p->execd = 1;
    }
  }

  if (closeDown) {
    /*if (s->debug == 1) Snack_WriteLogInt("Exit PlayCallback finish", s->nPlayed); pga destroy s*/
    ExecSoundCmd(sCurr, EXEC_AND_CLEAN);
    CleanSoundQueue();
    wop = IDLE;
    return;
  }

  if (!s->blockingPlay) {
    s->ptoken = Tcl_CreateTimerHandler(PLAYGRAIN, (Tcl_TimerProc *) PlayCallback, (int *)s);
  }
  if (s->debug == 1) Snack_WriteLogInt("Exit PlayCallback", s->nPlayed);
}

int
GetChannels(Tcl_Interp *interp, Tcl_Obj *obj, int *nchannels)
{
  int length, val;
  char *str = Tcl_GetStringFromObj(obj, &length);

  if (strncasecmp(str, "MONO", length) == 0) {
    *nchannels = SNACK_MONO;
    return TCL_OK;
  }
  if (strncasecmp(str, "STEREO", length) == 0) {
    *nchannels = SNACK_STEREO;
    return TCL_OK;
  }
  if (strncasecmp(str, "QUAD", length) == 0) {
    *nchannels = SNACK_QUAD;
    return TCL_OK;
  }
  if (Tcl_GetIntFromObj(interp, obj, &val) != TCL_OK) return TCL_ERROR;
  if (val < 1) {
    Tcl_AppendResult(interp, "Number of channels must be >= 1", NULL);
    return TCL_ERROR;
  }
  *nchannels = val;
  return TCL_OK;
}

int
GetFormat(Tcl_Interp *interp, Tcl_Obj *obj, int *sampformat, int *sampsize)
{
  int length;
  char *str = Tcl_GetStringFromObj(obj, &length);

  if (strncasecmp(str, "LIN16", length) == 0) {
    *sampformat = LIN16;
    *sampsize = 2;
  } else if (strncasecmp(str, "ALAW", length) == 0) {
    *sampformat = ALAW;
    *sampsize = 1;
  } else if (strncasecmp(str, "MULAW", length) == 0) {
    *sampformat = MULAW;
    *sampsize = 1;
  } else if (strncasecmp(str, "LIN8", length) == 0) {
    *sampformat = LIN8;
    *sampsize = 1;
  } else if (strncasecmp(str, "LIN8OFFSET", length) == 0) {
    *sampformat = LIN8OFFSET;
    *sampsize = 1;
  } else {
    Tcl_AppendResult(interp, "Unknown format", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

void
Snack_StopSound(Sound *s, Tcl_Interp *interp)
{
  if (s->debug == 1) Snack_WriteLog("Enter Snack_StopSound\n");

  if (s->storeType == SOUND_IN_MEMORY) {
    if ((rop == READ || rop == PAUSED) && (s->active == READ)) {
      if (rop == READ) {
	SnackAudioPause(&adi);
	while (SnackAudioReadable(&adi) > 0) {
	  if (s->length < s->maxlength - s->sampfreq / 16) {
	    int nRead = 0;
	    int size = s->sampfreq / 16;
	    if (s->sampformat == LIN16) {
	      int n = (s->length * s->nchannels) >> SEXP;
	      int i = (s->length * s->nchannels) - (n << SEXP);
	      if (size * s->nchannels + i > SBLKSIZE) {
		size = (SBLKSIZE - i) / s->nchannels;
	      }
	      nRead = SnackAudioRead(&adi, &s->blocks[n][i], size);
	    } else {
	      int n = (s->length * s->nchannels) >> CEXP;
	      unsigned char *p = (unsigned char *) s->blocks[n];
	      int i = (s->length * s->nchannels) - (n << CEXP);
	      if (size * s->nchannels + i > CBLKSIZE) {
		size = (CBLKSIZE - i) / s->nchannels;
	      }
	      nRead = SnackAudioRead(&adi, (short *)&p[i], size);
	    }
	    if (nRead > 0) {
	      if (s->debug == 1) Snack_WriteLogInt("Recording", nRead);
	      Snack_UpdateExtremes(s, s->length, s->length + nRead, SNACK_MORE_SOUND);
	      s->length += nRead;
	    }
	  } else {
	    break;
	  }
	}
	SnackAudioFlush(&adi);
	if (s->grab == 0) SnackAudioClose(&adi);
	Tcl_DeleteTimerHandler(s->rtoken);
      }
      rop = IDLE;
      s->active = IDLE;
      Snack_ExecCallbacks(s, SNACK_MORE_SOUND);
    }
    if ((wop == WRITE || wop == PAUSED) && (s->active == WRITE)) {
      if (s->debug == 1) Snack_WriteLogInt("Stopping", SnackAudioPlayed(&ado));
      if (wop == PAUSED) {
	SnackAudioResume(&ado);
      }
      SnackAudioFlush(&ado);
      SnackAudioClose(&ado);
      wop = IDLE;
      Tcl_DeleteTimerHandler(sCurr->ptoken);
      ExecSoundCmd(sCurr, CLEAN_ONLY);
      CleanSoundQueue();
    }
  } else { /* sound in file or channel */
    if ((rop == READ || rop == PAUSED) && (s->active == READ)) {
      SnackAudioPause(&adi);
      while (SnackAudioReadable(&adi) > 0) {
	int nRead = 0, i;
	int size = s->sampfreq / 16;
	nRead = SnackAudioRead(&adi, s->tmpbuf, size);
	if (s->sampsize == 2 && s->swap)
	  for (i = 0; i < (nRead * s->sampsize * s->nchannels) / 2; i++)
	    s->tmpbuf[i] = Snack_SwapShort(s->tmpbuf[i]);
	Tcl_Write(s->recchan, (char *)s->tmpbuf, nRead * s->sampsize);
	s->length += nRead;
      }
      if (Tcl_Seek(s->recchan, 0, SEEK_SET) != -1) {
	PutHeader(s);
	Tcl_Seek(s->recchan, 0, SEEK_END);
      }
      if (s->storeType == SOUND_IN_FILE) {
	Tcl_Close(interp, s->recchan);
      }
      ckfree((char *)s->tmpbuf);
      s->tmpbuf = NULL;
      s->recchan = NULL;
      SnackAudioFlush(&adi);
      if (s->grab == 0) SnackAudioClose(&adi);
      Tcl_DeleteTimerHandler(s->rtoken);
      rop = IDLE;
      s->active = IDLE;
      Snack_ExecCallbacks(s, SNACK_MORE_SOUND);
      s->validStart = 0;
    }
    if ((wop == WRITE || wop == PAUSED) && (s->active == WRITE)) {
      if (s->debug == 1) Snack_WriteLogInt("Stopping", SnackAudioPlayed(&ado));
      if (wop == PAUSED) {
	SnackAudioResume(&ado);
      }
      SnackAudioFlush(&ado);
      SnackAudioClose(&ado);
      wop = IDLE;
      Tcl_DeleteTimerHandler(sCurr->ptoken);
      ExecSoundCmd(sCurr, CLEAN_ONLY);
      CleanSoundQueue();
      ckfree((char *)s->tmpbuf);
      if (s->rwchan != NULL) {
        if (s->storeType == SOUND_IN_FILE) {
	  jkFileFormat *ff;
	  for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
	    if (strcmp(s->fileType, ff->formatName) == 0) {
	      SnackCloseFile(ff->closeProc, s, s->interp, &s->rwchan);
	      s->rwchan = NULL;
	      break;
	    }
	  }
        }
      }
    }
  }

  if (s->debug == 1) Snack_WriteLog("Exit Snack_StopSound\n");
}

void
SwapIfBE(Sound *s)
{
#ifdef LE
  s->swap = 0;
#else
  s->swap = 1;
#endif
}

void
SwapIfLE(Sound *s)
{
#ifdef LE
  s->swap = 1;
#else
  s->swap = 0;
#endif
}

#define QUERYBUFSIZE 100

static double startTime;

static int
playCmd(Sound *s, Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[])
{
  int startpos = 0, endpos = -1, block = 0, totlen = 0, arg;
  static int id = 1;
  static char *subOptionStrings[] = {
    "-output", "-start", "-end", "-command", "-blocking", NULL
  };
  enum subOptions {
    OUTPUT, START, END, COMMAND, BLOCKING
  };
  jkQueuedSound *qs, *p;
  jkFileFormat *ff;

  if (s->active == WRITE && wop == PAUSED) {
    startTime = SnackCurrentTime() - startTime;
    wop = WRITE;
    SnackAudioResume(&ado);
    sCurr->ptoken = Tcl_CreateTimerHandler(IPLAYGRAIN, (Tcl_TimerProc *) PlayCallback, (int *)sCurr);
    return TCL_OK;
  }

  if (!((wop == IDLE) && (s->active == IDLE))) {
    if (s->sampformat != sCurr->sampformat || s->nchannels != sCurr->nchannels) {
      Tcl_AppendResult(interp, "Sound format differs", NULL);
      return TCL_ERROR;
    }
  }

  s->cmdPtr = NULL;
  s->firstNRead = 0;
  
  for (arg = 2; arg < objc; arg+=2) {
    int index, length;
    char *str;
    
    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			    "option", 0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
    
    switch ((enum subOptions) index) {
    case OUTPUT:
      {
	str = Tcl_GetStringFromObj(objv[arg+1], &length);
	SnackMixerSetOutputJack(str, "1");
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
    case COMMAND:
      {
	Tcl_IncrRefCount(objv[arg+1]);
	s->cmdPtr = objv[arg+1];
	break;
      }
    case BLOCKING:
      {
	if (Tcl_GetBooleanFromObj(interp, objv[arg+1], &block) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }
  /*  if (s->storeType == SOUND_IN_FILE) {
    for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
      if (strcmp(s->fileType, ff->formatName) == 0) {
	if (GetHeader(s, interp, NULL) != TCL_OK) {
	  return TCL_ERROR;
	}
	break;
      }
    }
  }*/
  if (s->storeType == SOUND_IN_CHANNEL) {
    int tlen = 0, rlen = 0;

    s->buffersize = 1024;
    if ((s->tmpbuf = (short *) ckalloc(1024)) == NULL) {
      Tcl_AppendResult(interp, "Couldn't allocate buffer!", NULL);
      return TCL_ERROR;
    }
    while (tlen < s->buffersize) {
      rlen = Tcl_Read(s->rwchan, &((char *)s->tmpbuf)[tlen], 1);
      if (rlen < 0) break;
      s->firstNRead += rlen;
      tlen += rlen;
      if (s->forceFormat == 0) {
	s->fileType = GuessFileType((char *)s->tmpbuf, tlen, 0);
	if (strcmp(s->fileType, QUE_STRING) != 0) break;
      }
    }
    for (ff = snackFileFormats; ff != NULL; ff = ff->next) {
      if (strcmp(s->fileType, ff->formatName) == 0) {
	if ((ff->getHeaderProc)(s, interp, s->rwchan, NULL,
				(char *)s->tmpbuf)
	    != TCL_OK) return TCL_ERROR;
	break;
      }
    }
    if (strcmp(s->fileType, RAW_STRING) == 0 && s->guessFormat) {
      GuessFormat(s, (unsigned char *)s->tmpbuf, s->firstNRead / 2);
    }
    ckfree((char *)s->tmpbuf);
    s->firstNRead -= s->headSize;
  }
  if (s->storeType != SOUND_IN_MEMORY) {
    if (s->buffersize < s->sampfreq / 2) {
      s->buffersize = s->sampfreq / 2;
    }
    if ((s->tmpbuf = (short *) ckalloc(s->buffersize * s->sampsize *
				       s->nchannels)) == NULL) {
      Tcl_AppendResult(interp, "Couldn't allocate buffer!", NULL);
      return TCL_ERROR;
    }
  }
  if (s->storeType == SOUND_IN_MEMORY) {
    totlen = s->length;
    if (endpos >= totlen) endpos = totlen;
    if (endpos < 0)       endpos = totlen;
    if (endpos > startpos) totlen -= (totlen - endpos);
    if (startpos >= endpos) {
      ExecSoundCmd(s, EXEC_AND_CLEAN);
      return TCL_OK;
    }
    if (startpos > 0) totlen -= startpos; else startpos = 0;
  } else if (s->length != -1 && s->storeType == SOUND_IN_FILE) {
    totlen = s->length;
    if (endpos >= totlen) endpos = totlen;
    if (endpos < 0)       endpos = totlen;
    if (endpos > startpos) totlen -= (totlen - endpos);
    if (startpos >= endpos) {
      ExecSoundCmd(s, EXEC_AND_CLEAN);
      return TCL_OK;
    }
    if (startpos > 0) totlen -= startpos; else startpos = 0;
    s->totLen = totlen;
  } else {
    s->length = 0;
    s->totLen = 0;
    if (startpos < 0) startpos = 0;
    if (startpos >= endpos && endpos > 0) {
      ExecSoundCmd(s, EXEC_AND_CLEAN);
      return TCL_OK;
    }
    if (endpos > 0) s->totLen = endpos - startpos;
  }
  
  qs = (jkQueuedSound *) ckalloc(sizeof(jkQueuedSound));
  
  if (qs == NULL) {
    Tcl_AppendResult(interp, "Unable to alloc queue struct", NULL);
    return TCL_ERROR;
  }
  qs->sound = s;
  qs->startPos = startpos;
  qs->totLen = totlen;
  qs->cmdPtr = s->cmdPtr;
  if (s->cmdPtr != NULL) {
    Tcl_IncrRefCount(s->cmdPtr);
  }
  qs->done = 0;
  qs->execd = 0;
  qs->id = id++;
  qs->next = NULL;
  if (soundQueue == NULL) {
    soundQueue = qs;
  } else {
    for (p = soundQueue; p->next != NULL; p = p->next);
    p->next = qs;
  } 
  s->totLen = totlen;     
  if (!((wop == IDLE) && (s->active == IDLE))) {
    s->active = WRITE;
    return TCL_OK;
  } else {
    qs->done = 1;
    sCurr = s;
  }
  s->startPos = startpos;
  if (wop != PAUSED) {
    s->nPlayed = 0;
  }
  ado.debug = s->debug;
  if (s->storeType == SOUND_IN_FILE) {
    s->rwchan = NULL;
  }    
  wop = WRITE;
  s->active = WRITE;
  if (SnackAudioOpen(&ado, interp, PLAY, s->sampfreq, s->nchannels,
		     s->sampformat) != TCL_OK) {
    wop = IDLE;
    s->active = IDLE;
    return TCL_ERROR;
  }
  s->blockingPlay = block;
  corr = 0;
  if (s->blockingPlay) {
    PlayCallback((ClientData) s);
  } else {
    s->ptoken = Tcl_CreateTimerHandler(IPLAYGRAIN, (Tcl_TimerProc *) PlayCallback, (int *)s);
  }
  startTime = SnackCurrentTime();

  return TCL_OK;
}

static int
readCmd(Sound *s, Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[])
{
  char *filetype;
  int arg, startpos = 0, endpos = -1;
  static char *subOptionStrings[] = {
    "-frequency", "-skiphead", "-byteorder", "-channels",
    "-format", "-start", "-end", "-fileformat", "-guessproperties", NULL
  };
  enum subOptions {
    FREQUENCY, SKIPHEAD, BYTEORDER, CHANNELS, FORMAT, START, END,
    FILEFORMAT, GUESSPROPS
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "read only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
  if (Tcl_IsSafe(interp)) {
    Tcl_AppendResult(interp, "can't read sound from a file in a safe",
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
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
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

static int
recordCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (s->active == READ && rop == PAUSED) {
    rop = READ;
    if (s->grab == 0) {
      if (SnackAudioOpen(&adi, interp, RECORD, s->sampfreq, s->nchannels,
			 s->sampformat) != TCL_OK) {
	rop = IDLE;
	s->active = IDLE;
	return TCL_ERROR;
      }
    }
    SnackAudioFlush(&adi);
    SnackAudioResume(&adi);
    Snack_ExecCallbacks(s, SNACK_MORE_SOUND); 
    s->rtoken = Tcl_CreateTimerHandler(RECGRAIN, (Tcl_TimerProc *) RecCallback,
				       (int *)s);

    return TCL_OK;
  }

  if (((rop == IDLE) || (rop == GRABBED)) && (s->active == IDLE)) {
    rop = READ;
    s->active = READ;
  } else {
    return TCL_OK;
  }
      
  if (s->storeType == SOUND_IN_MEMORY) {
    int arg, append = 0;
    static char *subOptionStrings[] = {
      "-input", "-append", NULL
    };
    enum subOptions {
      INPUT, APPEND
    };
	
    for (arg = 2; arg < objc; arg+=2) {
      int index, length;
      char *str;
	  
      if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings, "option",
			      0, &index) != TCL_OK) {
	return TCL_ERROR;
      }
	  
      switch ((enum subOptions) index) {
      case INPUT:
	{
	  str = Tcl_GetStringFromObj(objv[arg+1], &length);
	  SnackMixerSetInputJack(interp, str, "1");
	  break;
	}
      case APPEND:
	{
	  if (Tcl_GetBooleanFromObj(interp, objv[arg+1], &append) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  break;
	}
      }
    }

    if (!append) {
      s->length = 0;
      s->maxsamp = 0;
      s->minsamp = 0;
    }
    adi.debug = s->debug;
    if (s->grab == 0) {
      if (SnackAudioOpen(&adi, interp, RECORD, s->sampfreq, s->nchannels,
			 s->sampformat) != TCL_OK) {
	rop = IDLE;
	s->active = IDLE;
	return TCL_ERROR;
      }
    }
    SnackAudioFlush(&adi);
    SnackAudioResume(&adi);
    Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
      
    s->rtoken = Tcl_CreateTimerHandler(RECGRAIN, (Tcl_TimerProc *) RecCallback, (int *)s);
  } else { /* SOUND_IN_FILE or SOUND_IN_CHANNEL */
    int arg, mode;
    static char *subOptionStrings[] = {
      "-input", "-fileformat", NULL
    };
    enum subOptions {
      INPUT, FILEFORMAT
    };

    s->tmpbuf = NULL;

    for (arg = 2; arg < objc; arg+=2) {
      int index, length;
      char *str;
	
      if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings, "option", 0,
			      &index) != TCL_OK) {
	return TCL_ERROR;
      }
    
      switch ((enum subOptions) index) {
      case INPUT:
	{
	  str = Tcl_GetStringFromObj(objv[arg+1], &length);
	  SnackMixerSetInputJack(interp, str, "1");
	  break;
	}
      case FILEFORMAT:
	{
	  if (GetFileFormat(interp, objv[arg+1], &s->fileType) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      }
    }

    if (s->buffersize < s->sampfreq / 2) {
      s->buffersize = s->sampfreq / 2;
    }

    if ((s->tmpbuf = (short *) ckalloc(s->buffersize * s->sampsize * 
				       s->nchannels)) == NULL) {
      Tcl_AppendResult(interp, "Couldn't allocate buffer!", NULL);
      return TCL_ERROR;
    }

    if (s->storeType == SOUND_IN_FILE) {
      s->recchan = Tcl_OpenFileChannel(interp, s->fcname, "w", 420);
      if (s->recchan != NULL) {
	mode = TCL_WRITABLE;
      }
    } else {
      s->recchan = Tcl_GetChannel(interp, s->fcname, &mode);
    }

    if (s->recchan == NULL) {
      return TCL_ERROR;
    }
    Tcl_SetChannelOption(interp, s->recchan, "-translation", "binary");
#ifdef TCL_81_API
    Tcl_SetChannelOption(interp, s->recchan, "-encoding", "binary");
#endif
    if (!(mode & TCL_WRITABLE)) {
      Tcl_AppendResult(interp, "channel \"", s->fcname, 
		       "\" wasn't opened for writing", NULL);
      s->recchan = NULL;
      return TCL_ERROR;
    }

    PutHeader(s);
    Snack_ResizeSoundStorage(s, 1);
    s->length = 0;
    s->validStart = 0;
    if (SnackAudioOpen(&adi, interp, RECORD, s->sampfreq, s->nchannels,
		       s->sampformat) != TCL_OK) {
      rop = IDLE;
      s->active = IDLE;
      return TCL_ERROR;
    }
    SnackAudioFlush(&adi);
    SnackAudioResume(&adi);
    Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

    s->rtoken = Tcl_CreateTimerHandler(RECGRAIN, (Tcl_TimerProc *) RecCallback, (int *)s);
  }

  return TCL_OK;
}

static int
grabCmd(Sound *s, Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[])
{
  int arg;
  static char *subOptionStrings[] = {
    "-input", "-frequency", "-channels", "-format", NULL
  };
  enum subOptions {
    INPUT, FREQUENCY, CHANNELS, FORMAT
  };

  for (arg = 2; arg < objc; arg+=2) {
    int index, length;
    char *str;
	
    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings, "option", 0,
			    &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    switch ((enum subOptions) index) {
    case INPUT:
      {
	str = Tcl_GetStringFromObj(objv[arg+1], &length);
	SnackMixerSetInputJack(interp, str, "1");
	break;
      }
    case FREQUENCY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->sampfreq) != TCL_OK)
	  return TCL_ERROR;
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
	break;
      }
    }
  }
  if (s->grab == 1) return TCL_OK;
  if (rop == IDLE) {
    rop = GRABBED;
  } else {
    return TCL_OK;
  }
  adi.debug = s->debug;
  if (SnackAudioOpen(&adi, interp, RECORD, s->sampfreq, s->nchannels,
		     s->sampformat) != TCL_OK) {
    rop = IDLE;
    return TCL_ERROR;
  }
  s->grab = 1;

  return TCL_OK;
}

static int
releaseCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (s->grab == 1) SnackAudioClose(&adi);
  rop = IDLE;
  s->grab = 0;

  return TCL_OK;
}

static int
stopCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Snack_StopSound(s, interp);

  return TCL_OK;
}

static int
writeCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int startpos = 0, endpos = s->length, arg, len;
  char *string, *filetype = NULL;
  static char *subOptionStrings[] = {
    "-start", "-end", "-fileformat", NULL
  };
  enum subOptions {
    START, END, FILEFORMAT
  };

  if (s->debug == 1) { Snack_WriteLog("Enter writeCmd\n"); }

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "write only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
  if (Tcl_IsSafe(interp)) {
    Tcl_AppendResult(interp, "can't write sound to a file in a safe",
		     " interpreter", (char *) NULL);
    return TCL_ERROR;
  }

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

static int
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
	    Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
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
	    Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
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

static int
infoCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Tcl_Obj *objs[8];

  objs[0] = Tcl_NewIntObj(s->length);
  objs[1] = Tcl_NewIntObj(s->sampfreq);
  objs[2] = Tcl_NewIntObj(s->maxsamp);
  objs[3] = Tcl_NewIntObj(s->minsamp);
  objs[4] = Tcl_NewStringObj(ssfmt[s->sampformat], -1);
  objs[5] = Tcl_NewIntObj(s->nchannels);
  objs[6] = Tcl_NewStringObj(s->fileType, -1);
  objs[7] = Tcl_NewIntObj(s->headSize);

  Tcl_SetObjResult(interp, Tcl_NewListObj(8, objs));
  return TCL_OK;
}

static int
current_positionCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int n = s->startPos + SnackAudioPlayed(&ado);
  int arg, len, type = 0;
      
  if (wop == IDLE) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(-1));
    return TCL_OK;
  }
  for (arg = 2; arg < objc; arg++) {
    char *string = Tcl_GetStringFromObj(objv[arg], &len);
	
    if (strncmp(string, "-units", len) == 0) {
      string = Tcl_GetStringFromObj(objv[++arg], &len);
      if (strncasecmp(string, "seconds", len) == 0) type = 1;
      if (strncasecmp(string, "samples", len) == 0) type = 0;
      arg++;
    }
  }
      
  if (type == 0) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(max(n, 0)));
  } else {
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj((float) max(n,0) / s->sampfreq));
  }

  return TCL_OK;
}

static int
maxCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int startpos = 0, endpos = s->length, arg, maxsamp, minsamp, channel = -1;
  SnackLinkedFileInfo info;
  static char *subOptionStrings[] = {
    "-start", "-end", "-channel", NULL
  };
  enum subOptions {
    START, END, CHANNEL
  };

  for (arg = 2; arg < objc; arg+=2) {
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
    case CHANNEL:
      {
	char *str = Tcl_GetStringFromObj(objv[arg+1], NULL);
	if (GetChannel(interp, str, s->nchannels, &channel) != TCL_OK) {
	  return TCL_ERROR;
	  break;
	}
      }
    }
  }
  if (endpos < 0) endpos = s->length;

  if (startpos < 0 || startpos > s->length) {
    Tcl_AppendResult(interp, "Start value out of bounds", NULL);
    return TCL_ERROR;
  }
  if (endpos > s->length) {
    Tcl_AppendResult(interp, "End value out of bounds", NULL);
    return TCL_ERROR;
  }

  if (objc == 2) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(s->maxsamp));
  } else {
    if (s->storeType != SOUND_IN_MEMORY) {
      OpenLinkedFile(s, &info);
    }
    Snack_GetExtremes(s, &info, startpos, endpos, channel, &maxsamp, &minsamp);
    if (s->storeType != SOUND_IN_MEMORY) {
      CloseLinkedFile(&info);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(maxsamp));
  }

  return TCL_OK;
}

static int
minCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int startpos = 0, endpos = s->length, arg, maxsamp, minsamp, channel = -1;
  SnackLinkedFileInfo info;
  static char *subOptionStrings[] = {
    "-start", "-end", "-channel", NULL
  };
  enum subOptions {
    START, END, CHANNEL
  };

  for (arg = 2; arg < objc; arg+=2) {
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
    case CHANNEL:
      {
	char *str = Tcl_GetStringFromObj(objv[arg+1], NULL);
	if (GetChannel(interp, str, s->nchannels, &channel) != TCL_OK) {
	  return TCL_ERROR;
	}
	break;
      }
    }
  }
  if (endpos < 0) endpos = s->length;

  if (startpos < 0 || startpos > s->length) {
    Tcl_AppendResult(interp, "Start value out of bounds", NULL);
    return TCL_ERROR;
  }
  if (endpos > s->length) {
    Tcl_AppendResult(interp, "End value out of bounds", NULL);
    return TCL_ERROR;
  }

  if (objc == 2) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(s->minsamp));
  } else {
    if (s->storeType != SOUND_IN_MEMORY) {
      OpenLinkedFile(s, &info);
    }
    Snack_GetExtremes(s, &info, startpos, endpos, channel, &maxsamp, &minsamp);
    if (s->storeType != SOUND_IN_MEMORY) {
      CloseLinkedFile(&info);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(minsamp));
  }

  return TCL_OK;
}

static int
sampleCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int i, n, val;
  char buf[10];

  if (Tcl_GetIntFromObj(interp, objv[2], &i) != TCL_OK) return TCL_ERROR;
  if (i < 0 || i >= s->length) {
    Tcl_AppendResult(interp, "Index out of bounds", NULL);
    return TCL_ERROR;
  }

  i *= s->nchannels;

  if (objc < 4) {
    if (s->storeType != SOUND_IN_MEMORY && s->linkInfo.linkCh == NULL) {
      OpenLinkedFile(s, &s->linkInfo);
    }
    
    for (n = 0; n < s->nchannels; n++, i++) {
      switch (s->sampformat) {
      case LIN16:
	if (s->storeType == SOUND_IN_MEMORY) {
	  sprintf(buf, "%d", SSAMPLE(s, i));
	} else {
	  sprintf(buf, "%d", GetSample(&s->linkInfo, i));
	}
	break;
      case ALAW:
	if (s->storeType == SOUND_IN_MEMORY) {
	  sprintf(buf, "%d", Snack_Alaw2Lin(UCSAMPLE(s, i)));
	} else {
	  sprintf(buf,"%d", Snack_Alaw2Lin((unsigned char)GetSample(&s->linkInfo, i)));
	}
	break;
      case MULAW:
	if (s->storeType == SOUND_IN_MEMORY) {
	  sprintf(buf, "%d", Snack_Mulaw2Lin(UCSAMPLE(s, i)));
	} else {
	  sprintf(buf,"%d", Snack_Mulaw2Lin((unsigned char)GetSample(&s->linkInfo, i)));
	}
	break;
      case LIN8OFFSET:
	if (s->storeType == SOUND_IN_MEMORY) {
	  sprintf(buf, "%d", CSAMPLE(s, i));
	} else {
	  sprintf(buf, "%d", GetSample(&s->linkInfo, i));
	}
	break;
      case LIN8:
	if (s->storeType == SOUND_IN_MEMORY) {
	  sprintf(buf, "%d", UCSAMPLE(s, i));
	} else {
	  sprintf(buf, "%d", GetSample(&s->linkInfo, i));
	}
	break;
      }
      if (n < s->nchannels - 1) {
	Tcl_AppendResult(interp, buf, " ", NULL);
      } else {
	Tcl_AppendResult(interp, buf, NULL);
      }
    }
  } else {
    if (s->storeType != SOUND_IN_MEMORY) {
      Tcl_AppendResult(interp, "setting sample values only works with in-memory sounds", (char *) NULL);
      return TCL_ERROR;
    }
    for (n = 3; n < 3 + s->nchannels; n++, i++) {
      char *str;
      int len;

      if (n >= objc) break;
      str = Tcl_GetStringFromObj(objv[n], &len);
      if (strcmp(str, "?") == 0) continue;
      if (Tcl_GetIntFromObj(interp, objv[n], &val) != TCL_OK) return TCL_ERROR;
      if (val < -32768 || val > 32767) {
	Tcl_AppendResult(interp, "Sample value not in range -32768, 32767",
			 NULL);
	return TCL_ERROR;
      }
      switch (s->sampformat) {
      case LIN16:
	SSAMPLE(s, i) = val;
	break;
      case ALAW:
	UCSAMPLE(s, i) = Snack_Lin2Alaw((short)val);
	break;
      case MULAW:
	UCSAMPLE(s, i) = Snack_Lin2Mulaw((short)val);
	break;
      case LIN8OFFSET:
	if (val < 0 || val > 255) {
	  Tcl_AppendResult(interp, "Sample value not in range 0, 255", NULL);
	  return TCL_ERROR;
	}
	UCSAMPLE(s, i) = val;
	break;
      case LIN8:
	if (val < -128 || val > 127) {
	  Tcl_AppendResult(interp, "Sample value not in range -128, 127",
			   NULL);
	  return TCL_ERROR;
	}
	CSAMPLE(s, i) = val;
	break;
      }
    }
  }

  return TCL_OK;
}

static int
changedCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "changed new|more");
    return TCL_ERROR;
  }
  if (s->storeType == SOUND_IN_MEMORY) {
    Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  }
  if (objc > 2) {
    char *string = Tcl_GetStringFromObj(objv[2], NULL);
	
    if (strcasecmp(string, "new") == 0) {
      Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
      return TCL_OK;
    }
    if (strcasecmp(string, "more") == 0) {
      Snack_ExecCallbacks(s, SNACK_MORE_SOUND);
      return TCL_OK;
    }
    Tcl_AppendResult(interp, "unknow option, must be new or more",
		     (char *) NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

static int
byteswapCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "byteswap only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (s->sampsize == 2) {
    Snack_StopSound(s, interp);
    ByteSwapSound(s);
    Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
    Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
  }

  return TCL_OK;
}

extern Tcl_HashTable jkSoundTable;

static int
destroyCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *string = Tcl_GetStringFromObj(objv[0], NULL);

  s->destroy = 1;
  s->length = 0;
  if (wop == IDLE) {
    Snack_StopSound(s, interp);
    Snack_DeleteSound(s);
  }
  Tcl_DeleteCommand(interp, string);
  Tcl_DeleteHashEntry(Tcl_FindHashEntry(&jkSoundTable, string));

  return TCL_OK;
}

int
flushCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "flush only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
  
  Snack_StopSound(s, interp);
  Snack_ResizeSoundStorage(s, 0);
  s->length  = 0;
  s->maxsamp = 0;
  s->minsamp = 0;
  s->abmax  = 0;
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

static int
configureCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int arg, filearg = 0;
  static char *optionStrings[] = {
    "-load", "-file", "-channel", "-frequency", "-channels", "-format",
    "-byteorder", "-buffersize", "-skiphead", "-guessproperties",
    "-debug", NULL
  };
  enum options {
    OPTLOAD, OPTFILE, CHANNEL, FREQUENCY, CHANNELS, FORMAT, BYTEORDER,
    BUFFERSIZE, SKIPHEAD, GUESSPROPS, OPTDEBUG
  };

  if (s->debug == 1) { Snack_WriteLog("Enter configureCmd\n"); }

  if (objc == 2) { /* get all options */
    Tcl_Obj *objs[6];
    
    objs[0] = Tcl_NewIntObj(s->length);
    objs[1] = Tcl_NewIntObj(s->sampfreq);
    objs[2] = Tcl_NewIntObj(s->maxsamp);
    objs[3] = Tcl_NewIntObj(s->minsamp);
    objs[4] = Tcl_NewStringObj(ssfmt[s->sampformat], -1);
    objs[5] = Tcl_NewIntObj(s->nchannels);
    
    Tcl_SetObjResult(interp, Tcl_NewListObj(6, objs));

    return TCL_OK;
  } else if (objc == 3) { /* get option */
    int index;
    if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings, "option", 0,
			    &index) != TCL_OK) {
      return TCL_ERROR;
    }

    switch ((enum options) index) {
    case OPTLOAD:
      {
	if (s->storeType == SOUND_IN_MEMORY) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case OPTFILE:
      {
	if (s->storeType == SOUND_IN_FILE) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case CHANNEL:
      {
	if (s->storeType == SOUND_IN_CHANNEL) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case FREQUENCY:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->sampfreq));
	break;
      }
    case CHANNELS:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->nchannels));
	break;
      }
    case FORMAT:
      {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(ssfmt[s->sampformat], -1));
	break;
      }
    case BYTEORDER:
      if (s->sampformat == LIN16) {
#ifdef LE
	if (s->swap) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("bigEndian", -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("littleEndian", -1));
	}
#else
	if (s->swap) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("littleEndian", -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("bigEndian", -1));
	}
#endif
      } else {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
      }
      break;
    case BUFFERSIZE:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->buffersize));
	break;
      }
    case SKIPHEAD:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->skipBytes));
	break;
      }
    case GUESSPROPS:
      break;
    case OPTDEBUG:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->debug));
	break;
      }
    }
  } else { /* set option */

    s->guessFormat = -1;
    s->guessFrequency = -1;

    for (arg = 2; arg < objc; arg+=2) {
      int index;
      if (Tcl_GetIndexFromObj(interp, objv[arg], optionStrings, "option", 0,
			      &index) != TCL_OK) {
	return TCL_ERROR;
      }
	  
      switch ((enum options) index) {
      case OPTLOAD:
	{
	  if (arg+1 == objc) {
	    Tcl_AppendResult(interp, "No filename given", NULL);
	    return TCL_ERROR;
	  }
	  filearg = arg + 1;
	  s->storeType = SOUND_IN_MEMORY;
	  break;
	}
      case OPTFILE:
	{
	  if (arg+1 == objc) {
	    Tcl_AppendResult(interp, "No filename given", NULL);
	    return TCL_ERROR;
	  }
	  filearg = arg + 1;
	  s->storeType = SOUND_IN_FILE;
	  break;
	}
      case CHANNEL:
	{
	  if (arg+1 == objc) {
	    Tcl_AppendResult(interp, "No channel name given", NULL);
	    return TCL_ERROR;
	  }
	  filearg = arg + 1;
	  s->storeType = SOUND_IN_CHANNEL;
	  break;
	}
      case FREQUENCY:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->sampfreq) != TCL_OK)
	    return TCL_ERROR;
	  s->guessFrequency = 0;
	  break;
	}
      case CHANNELS:
	{
	  int oldn = s->nchannels;

	  if (GetChannels(interp, objv[arg+1], &s->nchannels) != TCL_OK)
	    return TCL_ERROR;
	  if (oldn != s->nchannels) {
	    s->length = s->length * oldn / s->nchannels;
	  }
	  break;
	}
      case FORMAT:
	{
	  int length;
	  char *str = Tcl_GetStringFromObj(objv[arg+1], &length);

	  if (strncasecmp(str, "LIN16", length) == 0) {
	    s->sampformat = LIN16;
	    if (s->sampsize == 1) s->length = s->length / 2;
	    s->sampsize = 2;
	  } else if (strncasecmp(str, "ALAW", length) == 0) {
	    s->sampformat = ALAW;
	    if (s->sampsize == 2) s->length = s->length * 2;
	    s->sampsize = 1;
	  } else if (strncasecmp(str, "MULAW", length) == 0) {
	    s->sampformat = MULAW;
	    if (s->sampsize == 2) s->length = s->length * 2;
	    s->sampsize = 1;
	  } else if (strncasecmp(str, "LIN8", length) == 0) {
	    s->sampformat = LIN8;
	    if (s->sampsize == 2) s->length = s->length * 2;
	    s->sampsize = 1;
	  } else if (strncasecmp(str, "LIN8OFFSET", length) == 0) {
	    s->sampformat = LIN8OFFSET;
	    if (s->sampsize == 2) s->length = s->length * 2;
	    s->sampsize = 1;
	  } else {
	    Tcl_AppendResult(interp, "Unknown format", NULL);
	    return TCL_ERROR;
	  }
	  if (s->storeType == SOUND_IN_MEMORY) {
	    Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
	  }
	  s->guessFormat = 0;
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
	    Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
	    return TCL_ERROR;
	  }
	  s->guessFormat = 0;
	  break;
	}
      case BUFFERSIZE:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->buffersize) != TCL_OK)
	    return TCL_ERROR;   
	  break;
	}
      case SKIPHEAD: 
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->skipBytes) != TCL_OK)
	    return TCL_ERROR;
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
      case OPTDEBUG:
	{
	  if (arg+1 == objc) {
	    Tcl_AppendResult(interp, "No debug flag given", NULL);
	    return TCL_ERROR;
	  }
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->debug) != TCL_OK) {
	    return TCL_ERROR;
	  }
	  break;
	}
      }
    }
    if (s->guessFormat == -1) s->guessFormat = 0;
    if (s->guessFrequency == -1) s->guessFrequency = 0;

    if (filearg > 0) {
      if (Tcl_IsSafe(interp)) {
	Tcl_AppendResult(interp, "can't read a sound in a safe interpreter",
			 (char *) NULL);
	return TCL_ERROR;
      }
      if (SetFcname(s, interp, objv[filearg]) != TCL_OK) {
	return TCL_ERROR;
      }
    }

    if (filearg > 0 && strlen(s->fcname) > 0) {
      if (s->storeType == SOUND_IN_MEMORY) {
	char *type = LoadSound(s, interp, NULL, 0, -1);
	
	if (type == NULL) {
	  return TCL_ERROR;
	}
      } else if (s->storeType == SOUND_IN_FILE) {
	if (GetHeader(s, interp, NULL) != TCL_OK) {
	  s->fileType = NameGuessFileType(s->fcname);
	}
	Snack_ResizeSoundStorage(s, 0);
	if (s->sampformat == LIN8OFFSET) {
	  s->maxsamp = 128;
	  s->minsamp = 128;
	} else {
	  s->maxsamp = 0;
	  s->minsamp = 0;
	}
	if (s->linkInfo.linkCh != NULL) {
	  CloseLinkedFile(&s->linkInfo);
	  s->linkInfo.linkCh = NULL;
	}
      } else if (s->storeType == SOUND_IN_CHANNEL) {
	int mode = 0;

	Snack_ResizeSoundStorage(s, 0);
	s->rwchan = Tcl_GetChannel(interp, s->fcname, &mode);
	if (!(mode & TCL_READABLE)) {
	  s->rwchan = NULL;
	}
	if (s->rwchan != NULL) {
	  Tcl_SetChannelOption(interp, s->rwchan, "-translation", "binary");
#ifdef TCL_81_API
	  Tcl_SetChannelOption(interp, s->rwchan, "-encoding", "binary");
#endif
	}
      }
    }
    if (filearg > 0 && strlen(s->fcname) == 0) {
      if (s->storeType == SOUND_IN_FILE) {
	s->length = 0;
      }
    }
    Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
  }
  if (s->debug == 1) { Snack_WriteLog("Exit configureCmd\n"); }

  return TCL_OK;
}

static int
cgetCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  static char *optionStrings[] = {
    "-load", "-file", "-channel", "-frequency", "-channels", "-format",
    "-byteorder", "-buffersize", "-skiphead", "-guessproperties",
    "-debug", NULL
  };
  enum options {
    OPTLOAD, OPTFILE, CHANNEL, FREQUENCY, CHANNELS, FORMAT, BYTEORDER,
    BUFFERSIZE, SKIPHEAD, GUESSPROPS, OPTDEBUG
  };

  if (objc == 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "cget option");
    return TCL_ERROR;
  } else if (objc == 3) { /* get option */
    int index;
    if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings, "option", 0,
			    &index) != TCL_OK) {
      return TCL_ERROR;
    }

    switch ((enum options) index) {
    case OPTLOAD:
      {
	if (s->storeType == SOUND_IN_MEMORY) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case OPTFILE:
      {
	if (s->storeType == SOUND_IN_FILE) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case CHANNEL:
      {
	if (s->storeType == SOUND_IN_CHANNEL) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj(s->fcname, -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	}
	break;
      }
    case FREQUENCY:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->sampfreq));
	break;
      }
    case CHANNELS:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->nchannels));
	break;
      }
    case FORMAT:
      {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(ssfmt[s->sampformat], -1));
	break;
      }
    case BYTEORDER:
      if (s->sampformat == LIN16) {
#ifdef LE
	if (s->swap) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("bigEndian", -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("littleEndian", -1));
	}
#else
	if (s->swap) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("littleEndian", -1));
	} else {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("bigEndian", -1));
	}
#endif
      } else {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
      }
      break;
    case BUFFERSIZE:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->buffersize));
	break;
      }
    case SKIPHEAD:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->skipBytes));
	break;
      }
    case GUESSPROPS:
      break;
    case OPTDEBUG:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(s->debug));
	break;
      }
    }
  }

  return TCL_OK;
}

static int
pauseCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (s->active == WRITE) {
    if (wop == WRITE) {
      int tmp = SnackAudioPause(&ado);

      startTime = SnackCurrentTime() - startTime;
      wop = PAUSED;
      Tcl_DeleteTimerHandler(sCurr->ptoken);
      if (tmp != -1) {
	jkQueuedSound *p;
	int count = 0;
	
	for (p = soundQueue; p != NULL && p->done == 1; p = p->next) {
	  count += p->totLen;
          if (count > tmp) {
	    sCurr = p->sound;
	    sCurr->nPlayed = tmp - (count - p->totLen);
	    corr = count - p->totLen;
	    break;
	  }
	}
	for (p = p->next; p != NULL && p->done == 1; p = p->next) {
	  p->done = 0;
	}
      }
    } else if (wop == PAUSED) {
      startTime = SnackCurrentTime() - startTime;
      wop = WRITE;
      SnackAudioResume(&ado);
      sCurr->ptoken = Tcl_CreateTimerHandler(IPLAYGRAIN, (Tcl_TimerProc *) PlayCallback, (int *)sCurr);
    }
  } else {
    if (rop == READ) {
      Snack_StopSound(s, interp);
      rop = PAUSED;
      s->active = READ;
      Tcl_DeleteTimerHandler(s->rtoken);
    } else if (rop == PAUSED) {
      rop = READ;
      if (s->grab == 0) {
	if (SnackAudioOpen(&adi, interp, RECORD, s->sampfreq, s->nchannels,
			   s->sampformat) != TCL_OK) {
	  rop = IDLE;
	  s->active = IDLE;
	  return TCL_ERROR;
	}
      }
      SnackAudioFlush(&adi);
      SnackAudioResume(&adi);
      Snack_ExecCallbacks(s, SNACK_MORE_SOUND); 
      s->rtoken = Tcl_CreateTimerHandler(RECGRAIN, (Tcl_TimerProc *) RecCallback, (int *)s);
    }
  }

  return TCL_OK;
}

#define NSOUNDCOMMANDS   31
#define MAXSOUNDCOMMANDS 50

static int nSoundCommands   = NSOUNDCOMMANDS;
static int maxSoundCommands = MAXSOUNDCOMMANDS;

char *sndCmdNames[MAXSOUNDCOMMANDS] = {
  "play",
  "read",
  "record",
  "grab",
  "release",
  "stop",
  "write",
  "data",
  "crop",
  "info",
  "length",
  "current_position",
  "max",
  "min",
  "sample",
  "changed",
  "copy",
  "append",
  "concatenate",
  "insert",
  "cut",
  "byteswap",
  "destroy",
  "flush",
  "configure",
  "cget",
  "pause",
  "convert",
  "fft",
  "pitch",
  "reverse",
  NULL
};

/* NOTE: NSOUNDCOMMANDS needs updating when new commands are added. */

soundCmd *sndCmdProcs[MAXSOUNDCOMMANDS] = {
  playCmd,
  readCmd,
  recordCmd,
  grabCmd,
  releaseCmd,
  stopCmd,
  writeCmd,
  dataCmd,
  cropCmd,
  infoCmd,
  lengthCmd,
  current_positionCmd,
  maxCmd,
  minCmd,
  sampleCmd,
  changedCmd,
  copyCmd,
  appendCmd,
  concatenateCmd,
  insertCmd,
  cutCmd,
  byteswapCmd,
  destroyCmd,
  flushCmd,
  configureCmd,
  cgetCmd,
  pauseCmd,
  convertCmd,
  fftCmd,
  pitchCmd,
  reverseCmd
};

soundDelCmd *sndDelCmdProcs[MAXSOUNDCOMMANDS] = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static int
SoundCmd(ClientData clientData, Tcl_Interp *interp, int objc,
	 Tcl_Obj *CONST objv[])
{
  register Sound *s = (Sound *) clientData;
  int index;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[1], sndCmdNames, "option", 0,
			  &index) != TCL_OK) {
    return TCL_ERROR;
  }

  return((sndCmdProcs[index])(s, interp, objc, objv)); 
}

Sound *
Snack_NewSound(int frequency, int format, int nchannels)
{
  Sound *s = (Sound *) ckalloc(sizeof(Sound));

  if (s == NULL) {
    return NULL;
  }

  /* Default sound specifications */

  s->sampfreq   = frequency;
  s->sampformat = format;
  if (s->sampformat == LIN16) {
    s->sampsize = 2;
  } else {
    s->sampsize = 1;
  }
  if (s->sampformat == LIN8OFFSET) {
    s->maxsamp = 128;
    s->minsamp = 128;
  } else {
    s->maxsamp = 0;
    s->minsamp = 0;
  }
  s->nchannels = nchannels;
  s->length    = 0;
  s->maxlength = 0;
  s->abmax     = 0;
  s->active    = IDLE;
  s->firstCB   = NULL;
  s->grab      = 0;
  s->fileType  = RAW_STRING;
  s->recchan   = NULL;
  s->playchan  = NULL;
  s->tmpbuf    = NULL;
  s->swap      = 0;
  s->headSize  = 0;
  s->skipBytes = 0;
  s->storeType = SOUND_IN_MEMORY;
  s->fcname    = NULL;
  s->cmdPtr    = NULL;
  s->blocks    = (short **) ckalloc(MAXNBLKS * sizeof(short*));
  if (s->blocks == NULL) {
    ckfree((char *) s);
    return NULL;
  }
  s->blocks[0] = NULL;
  s->maxblks   = MAXNBLKS;
  s->nblks     = 0;
  s->blockingPlay = 0;
  s->debug     = 0;
  s->destroy   = 0;
  s->guessFormat = 0;
  s->guessFrequency = 0;
  s->rwchan     = NULL;
  s->firstNRead = 0;
  s->buffersize = 0;
  s->forceFormat = 0;
  s->userFlag   = 0;
  s->userData   = NULL;
  s->itemRefCnt = 0;
  s->validStart = 0;
  s->linkInfo.linkCh = NULL;
  /*  s->monitorCmdPtr = NULL;*/

  return s;
}

void
CleanSound(Sound *s, Tcl_Interp *interp, char *name)
{
  Snack_DeleteSound(s);
  Tcl_DeleteHashEntry(Tcl_FindHashEntry(&jkSoundTable, name));
}

int
Snack_SoundCmd(ClientData cdata, Tcl_Interp *interp, int objc,
	       Tcl_Obj *CONST objv[])
{
  Sound *s;
  int arg, arg1, filearg = 0, flag;
  static int id = 0;
  int sampfreq = -1, nchannels = -1, sampformat = -1, sampsize = -1;
  char ids[20], *name;
  Tcl_HashEntry *hPtr;
  int length = 0;
  char *string = NULL;
  static char *optionStrings[] = {
    "-load", "-file", "-frequency", "-channels", "-format",
    "-channel", "-byteorder", "-buffersize", "-skiphead", "-debug",
    "-guessproperties", "-fileformat", /*"-command",*/ NULL
  };
  enum options {
    OPTLOAD, OPTFILE, FREQUENCY, CHANNELS, FORMAT, CHANNEL, BYTEORDER,
    BUFFERSIZE, SKIPHEAD, OPTDEBUG, GUESSPROPS, FILEFORMAT/*, COMMAND*/
  };

  if (objc > 1) {
    string = Tcl_GetStringFromObj(objv[1], &length);
  }
  if ((objc == 1) || (string[0] == '-')) {
    do {
      sprintf(ids, "sound%d", ++id);
    } while (Tcl_FindHashEntry(&jkSoundTable, ids) != NULL);
    name = ids;
    arg1 = 1;
  } else {
    name = string;
    arg1 = 2;
  }

  if (Tcl_FindHashEntry(&jkSoundTable, name) != NULL) {
    Tcl_AppendResult(interp, "Sound \"", name, "\" already exists", NULL);
    return TCL_ERROR;
  }

  if ((s = Snack_NewSound(16000, LIN16, SNACK_MONO)) == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate new sound!", NULL);
    return TCL_ERROR;
  }

  hPtr = Tcl_CreateHashEntry(&jkSoundTable, name, &flag);
  Tcl_SetHashValue(hPtr, (ClientData) s);

  s->interp = interp;
  s->guessFormat = -1;
  s->guessFrequency = -1;

  for (arg = arg1; arg < objc; arg += 2) {
    int index;
    if (Tcl_GetIndexFromObj(interp, objv[arg], optionStrings, "option", 0,
			    &index) != TCL_OK) {
      CleanSound(s, interp, name);
      return TCL_ERROR;
    }

    switch ((enum options) index) {
    case OPTLOAD:
      {
	if (arg+1 == objc) {
	  Tcl_AppendResult(interp, "No filename given", NULL);
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	filearg = arg + 1;
	s->storeType = SOUND_IN_MEMORY;
	break;
      }
    case OPTFILE:
      {
	if (arg+1 == objc) {
	  Tcl_AppendResult(interp, "No filename given", NULL);
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	filearg = arg + 1;
	s->storeType = SOUND_IN_FILE;
	break;
      }
    case FREQUENCY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &sampfreq) != TCL_OK) {
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	s->guessFrequency = 0;
	break;
      }
    case CHANNELS:
      {
	if (GetChannels(interp, objv[arg+1], &nchannels) != TCL_OK) {
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	break;
      }
    case FORMAT:
      {
	if (GetFormat(interp, objv[arg+1], &sampformat, &sampsize) != TCL_OK) {
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	s->guessFormat = 0;
	break;
      }
    case CHANNEL:
      {
	if (arg+1 == objc) {
	  Tcl_AppendResult(interp, "No channel name given", NULL);
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	filearg = arg + 1;
	s->storeType = SOUND_IN_CHANNEL;
	break;
      }
    case OPTDEBUG:
      {
	if (arg+1 == objc) {
	  Tcl_AppendResult(interp, "No debug flag given", NULL);
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->debug) != TCL_OK) {
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	break;
      }
    case FILEFORMAT:
      {
	if (GetFileFormat(interp, objv[arg+1], &s->fileType) != TCL_OK) {
	  CleanSound(s, interp, name);
	  return TCL_ERROR;
	}
	s->forceFormat = 1;
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
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
	  return TCL_ERROR;
	}
	s->guessFormat = 0;
	break;
      }
    case BUFFERSIZE:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->buffersize) != TCL_OK)
	  return TCL_ERROR;   
	break;
      }
    case SKIPHEAD: 
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &s->skipBytes) != TCL_OK)
	  return TCL_ERROR;
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
    /*    case COMMAND:
	  {
	  Tcl_IncrRefCount(objv[arg+1]);
	  s->monitorCmdPtr = objv[arg+1];
	  break;
	  }*/
    }
  }
  if (s->guessFormat == -1) s->guessFormat = 0;
  if (s->guessFrequency == -1) s->guessFrequency = 0;

  if (nchannels != -1) {
    s->nchannels = nchannels;
  }
  if (sampfreq != -1) {
    s->sampfreq = sampfreq;
  }
  if (sampformat != -1) {
    s->sampformat = sampformat;
    s->sampsize = sampsize;
  }
  
  if (filearg > 0) {
    if (Tcl_IsSafe(interp)) {
      Tcl_AppendResult(interp, "can't read a sound in a safe interpreter",
		       (char *) NULL);
      CleanSound(s, interp, name);
      return TCL_ERROR;
    }
    if (SetFcname(s, interp, objv[filearg]) != TCL_OK) {
      CleanSound(s, interp, name);
      return TCL_ERROR;
    }
  }

  if (filearg > 0 && strlen(s->fcname) > 0) {
    if (s->storeType == SOUND_IN_MEMORY) {
      char *type = LoadSound(s, interp, NULL, 0, -1);
      
      if (type == NULL) {
	CleanSound(s, interp, name);
	return TCL_ERROR;
      }
    } else if (s->storeType == SOUND_IN_FILE) {
      if (GetHeader(s, interp, NULL) != TCL_OK) {
	s->fileType = NameGuessFileType(s->fcname);
      }
      if (s->sampformat == LIN8OFFSET) {
	s->maxsamp = 128;
	s->minsamp = 128;
      } else {
	s->maxsamp = 0;
	s->minsamp = 0;
      }
    } else if (s->storeType == SOUND_IN_CHANNEL) {
      int mode = 0;

      s->rwchan = Tcl_GetChannel(interp, s->fcname, &mode);
      if (!(mode & TCL_READABLE)) {
	s->rwchan = NULL;
      }
      if (s->rwchan != NULL) {
	Tcl_SetChannelOption(interp, s->rwchan, "-translation", "binary");
#ifdef TCL_81_API
	Tcl_SetChannelOption(interp, s->rwchan, "-encoding", "binary");
#endif
      }
    }
  }

  Tcl_CreateObjCommand(interp, name, SoundCmd, (ClientData) s,
		       (Tcl_CmdDeleteProc *) Snack_SoundDeleteCmd); 
  
  Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));

  return TCL_OK;
}

Sound *
Snack_GetSound(Tcl_Interp *interp, char *name)
{
  Tcl_CmdInfo infoPtr;

  if (Tcl_GetCommandInfo(interp, name, &infoPtr) == 0) {
    Tcl_AppendResult(interp, name, " : no such sound", (char *) NULL);
    return NULL;
  }

  return (Sound *)infoPtr.objClientData;
}

void
Snack_SoundDeleteCmd(ClientData clientData)
{
  register Sound *s = (Sound *) clientData;
  int i;

  for (i = 0; i < nSoundCommands; i++) {
    if (sndDelCmdProcs[i] != NULL) {
      (sndDelCmdProcs[i])(s);
    }
  }
  Snack_StopSound(s, s->interp);
}

void
Snack_ExitProc(ClientData clientData)
{
  if (rop != IDLE) {
    SnackAudioFlush(&adi);
    SnackAudioClose(&adi);
  }
  if (wop != IDLE) {
    SnackAudioFlush(&ado);
    SnackAudioClose(&ado);
  }
}

static int
formatsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackAudioGetFormats(tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));

  return TCL_OK;
}

static int
frequenciesCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackAudioGetFrequencies(tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));

  return TCL_OK;
}

static int
inputCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *jack, tmpstr[QUERYBUFSIZE];

  if (objc < 3) {
    SnackMixerGetInputJack(tmpstr, QUERYBUFSIZE);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  } else {
    jack = Tcl_GetStringFromObj(objv[2], NULL);
    if (objc == 3) {
      if (SnackMixerSetInputJack(interp, jack, "1")) {
	Tcl_AppendResult(interp, "Error setting input jack", NULL);
	return TCL_ERROR;
      };
    } else {
      SnackMixerLinkJacks(interp, jack, objv[3]);
    }
  }
  
  return TCL_OK;
}

static int
inputsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackMixerGetInputJacks(tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  
  return TCL_OK;
}

static int
outputCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *jack, tmpstr[QUERYBUFSIZE];
  
  if (objc < 3) {
    SnackMixerGetOutputJack(tmpstr, QUERYBUFSIZE);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  } else {
    jack = Tcl_GetStringFromObj(objv[2], NULL);
    if (objc == 3) {
      SnackMixerSetOutputJack(jack, "1");
    } else {
      SnackMixerLinkJacks(interp, jack, objv[3]);
    }
  }
  
  return TCL_OK;
}

static int
outputsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackMixerGetOutputJacks(tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  
  return TCL_OK;
}

static int
activeCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (wop == IDLE && rop == IDLE) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
  } else {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
  }
  
  return TCL_OK;
}

static int
play_gainCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int g;
  
  if (objc == 3) {
    if (Tcl_GetIntFromObj(interp, objv[2], &g) != TCL_OK) return TCL_ERROR;
    ASetPlayGain(g);
  } else {
#ifdef HPUX
    if (wop == IDLE)
#endif
      Tcl_SetObjResult(interp, Tcl_NewIntObj(AGetPlayGain()));
  }

  return TCL_OK;
}

static int
record_gainCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int g;
  
  if (objc == 3) {
    if (Tcl_GetIntFromObj(interp, objv[2], &g) != TCL_OK) return TCL_ERROR;
	ASetRecGain(g);
  } else {
#ifdef HPUX
    if (rop == IDLE)
#endif
      Tcl_SetObjResult(interp, Tcl_NewIntObj(AGetRecGain()));
  }

  return TCL_OK;
}

static int
elapsedTimeCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  double elapsedTime = SnackCurrentTime() - startTime;

  if (wop == IDLE) {
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(0.0));
  }  else if (wop == PAUSED) {
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(startTime));
  } else {
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(elapsedTime));
  }

  return TCL_OK;
}

static int
currentSoundCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  jkQueuedSound *p;
  char *res;
  Tcl_HashSearch hashSearch;
  Tcl_HashEntry *entryPtr;

  if (soundQueue == NULL) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
    return TCL_OK;
  }
  for (p = soundQueue; p->next != NULL && p->next->done == 1; p = p->next);

  entryPtr = Tcl_FirstHashEntry(&jkSoundTable, &hashSearch);

  if (p->sound != (Sound *) Tcl_GetHashValue(entryPtr)) {
    entryPtr = Tcl_NextHashEntry(&hashSearch);
  }
  res = Tcl_GetHashKey(&jkSoundTable, entryPtr);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(res, -1));

  return TCL_OK;
}

static int
channelsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *mixer, tmpstr[QUERYBUFSIZE];
  
  if (objc == 3) {
    mixer = Tcl_GetStringFromObj(objv[2], NULL);
    SnackMixerGetNumChannels(mixer, tmpstr, QUERYBUFSIZE);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  }

  return TCL_OK;
}

static int
volumeCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char *mixer, tmpstr[QUERYBUFSIZE];

  mixer = Tcl_GetStringFromObj(objv[2], NULL);

  if (objc == 3) {
#ifdef HPUX
    if (rop != IDLE || wop != IDLE) return TCL_OK;
#endif
    SnackMixerGetVolume(mixer, -1, tmpstr, QUERYBUFSIZE);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  } else if (objc == 4) {
  } else if (objc == 5) {
    SnackMixerGetNumChannels(mixer, tmpstr, QUERYBUFSIZE);
    if (strcmp("Mono", tmpstr) == 0) {
      Tcl_AppendResult(interp, "Mixer is single channel", NULL);
      return TCL_ERROR;
    }
  } else {
    Tcl_WrongNumArgs(interp, 1, objv, "audio volume mixer [leftVar] [rightVar]");
    return TCL_ERROR;
  }
  SnackMixerLinkVolume(interp, mixer, objc - 3, objv);

  return TCL_OK;
}

static int
mixersCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackMixerGetMixers(tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));
  
  return TCL_OK;
}

static int
updateCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  SnackMixerUpdateVars(interp);
  
  return TCL_OK;
}

#define NAUDIOCOMMANDS   15
#define MAXAUDIOCOMMANDS 20

static int nAudioCommands   = NAUDIOCOMMANDS;
static int maxAudioCommands = MAXAUDIOCOMMANDS;

char *audioCmdNames[MAXAUDIOCOMMANDS] = {
  "formats",
  "frequencies",
  "input",
  "inputs",
  "output",
  "outputs",
  "active",
  "play_gain",
  "record_gain",
  "elapsedTime",
  "currentSound",
  "channels",
  "volume",
  "mixers",
  "update",
  NULL
};

/* NOTE: NAUDIOCOMMANDS needs updating when new commands are added. */

audioCmd *audioCmdProcs[MAXAUDIOCOMMANDS] = {
  formatsCmd,
  frequenciesCmd,
  inputCmd,
  inputsCmd,
  outputCmd,
  outputsCmd,
  activeCmd,
  play_gainCmd,
  record_gainCmd,
  elapsedTimeCmd,
  currentSoundCmd,
  channelsCmd,
  volumeCmd,
  mixersCmd,
  updateCmd
};

audioDelCmd *audioDelCmdProcs[MAXAUDIOCOMMANDS] = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

int
Snack_AudioCmd(ClientData cdata, Tcl_Interp *interp, int objc,
	       Tcl_Obj *CONST objv[])
{
  int index;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg?");
    return TCL_ERROR;
  }
  
  if (Tcl_GetIndexFromObj(interp, objv[1], audioCmdNames, "option", 0,
			  &index) != TCL_OK) {
    return TCL_ERROR;
  }

  return((audioCmdProcs[index])(interp, objc, objv)); 
}

int
Snack_AddSubCmd(int snackCmd, char *cmdName, Snack_CmdProc *cmdProc,
		Snack_DelCmdProc *delCmdProc)
{
  int i;

  switch(snackCmd) {
  case SNACK_SOUND_CMD:
    if (nSoundCommands < maxSoundCommands) {
      for (i = 0; i < nSoundCommands; i++) {
	if (strcmp(sndCmdNames[i], cmdName) == 0) break;
      }
      sndCmdNames[i] = cmdName;
      sndCmdProcs[i] = (soundCmd *)cmdProc;
      sndDelCmdProcs[i] = (soundDelCmd *)delCmdProc;
      if (i == nSoundCommands) nSoundCommands++;
    }
    break;
  case SNACK_AUDIO_CMD:
    if (nAudioCommands < maxAudioCommands) {
      for (i = 0; i < nAudioCommands; i++) {
	if (strcmp(audioCmdNames[i], cmdName) == 0) break;
      }
      audioCmdNames[i] = cmdName;
      audioCmdProcs[i] = (audioCmd *)cmdProc;
      audioDelCmdProcs[i] = (audioDelCmd *)delCmdProc;
      if (i == nAudioCommands) nAudioCommands++;
    }
    break;
  }

  return TCL_OK;
}

void
Snack_AudioDeleteCmd(ClientData clientData)
{
  int i;

  for (i = 0; i < nAudioCommands; i++) {
    if (audioDelCmdProcs[i] != NULL) {
      (audioDelCmdProcs[i])();
    }
  }
}

int
SetFcname(Sound *s, Tcl_Interp *interp, Tcl_Obj *obj)
{
  int length;
  char *str = Tcl_GetStringFromObj(obj, &length);

  if (s->fcname != NULL) {
    ckfree((char *)s->fcname);
  }
  if ((s->fcname = (char *) ckalloc((unsigned) (length + 1))) == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate name buffer!", NULL);
    return TCL_ERROR;
  }
  strcpy(s->fcname, str);

  return TCL_OK;
}

Tcl_Channel snackDebugChannel = NULL;

void
Snack_WriteLog(char *s)
{
  Tcl_Write(snackDebugChannel, s, strlen(s));
  Tcl_Flush(snackDebugChannel);
}

void
Snack_WriteLogInt(char *s, int n)
{
  char buf[20];

  Tcl_Write(snackDebugChannel, s, strlen(s));
  sprintf(buf, " %d", n);
  Tcl_Write(snackDebugChannel, buf, strlen(buf));
  Tcl_Write(snackDebugChannel, "\n", 1);
  Tcl_Flush(snackDebugChannel);
}

/*
 *----------------------------------------------------------------------
 *
 * SnackCurrentTime --
 *
 *	Returns the current system time in seconds (with decimals)
 *	since the beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 * Results:
 *	Returns the current time.
 *
 *----------------------------------------------------------------------
 */

#ifdef WIN
#  include <sys/types.h>
#  include <sys/timeb.h>
#else
#  include <sys/time.h>
#endif

double
SnackCurrentTime()
{
#ifndef WIN
  struct timeval tv;
  struct timezone tz;
  
  (void) gettimeofday(&tv, &tz);

  return(tv.tv_sec + tv.tv_usec * 0.000001);

#else
  struct timeb t;
  
  ftime(&t);

  return(t.time + t.millitm * 0.001);

#endif
}

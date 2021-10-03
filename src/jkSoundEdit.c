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

#include "jkSound.h"
#include "jkAudIO.h"
#include <math.h>
#include <string.h>

static void
SmpMov(Sound *dest, int to, Sound *src, int from, int len)
{
  if (dest->storeType == SOUND_IN_MEMORY) {
    int sn, si, dn, di, tot = 0, blklen;
    
    to   *= src->nchannels;
    from *= src->nchannels;
    len  *= src->nchannels;
    
    if (dest == src && from < to) {
      tot = len;
      if (src->sampformat == LIN16) {
	while (tot > 0) {
	  sn = (from + tot) >> SEXP;
	  si = (from + tot) - (sn << SEXP);
	  dn = (to   + tot) >> SEXP;
	  di = (to   + tot) - (dn << SEXP);
	  
	  if (di == 0) {
	    blklen = si;
	  } else if (si == 0) {
	    blklen = di;
	  } else { 
	    blklen = min(si, di);
	  }
	  
	  blklen = min(blklen, tot);
	  si -= blklen;
	  di -= blklen;
	  
	  if (si < 0) {
	    si = SBLKSIZE + si;
	    sn--;
	  }
	  if (di < 0) {
	    di = SBLKSIZE + di;
	    dn--;
	  }
	  memmove(&dest->blocks[dn][di], &src->blocks[sn][si], 
		  blklen*sizeof(short));
	  tot -= blklen;
	}
      } else {
	while (tot > 0) {
	  sn = (from + tot) >> CEXP;
	  si = (from + tot) - (sn << CEXP);
	  dn = (to   + tot) >> CEXP;
	  di = (to   + tot) - (dn << CEXP);
	  
	  if (di == 0) {
	    blklen = si;
	  } else if (si == 0) {
	    blklen = di;
	  } else { 
	    blklen = min(si, di);
	  }
	  
	  blklen = min(blklen, tot);
	  si -= blklen;
	  di -= blklen;
	  
	  if (si < 0) {
	    si = CBLKSIZE + si;
	    sn--;
	  }
	  if (di < 0) {
	    di = CBLKSIZE + di;
	    dn--;
	  }
	  memmove(&((unsigned char **)dest->blocks)[dn][di],
		  &((unsigned char **)src->blocks)[sn][si], blklen);
	  tot -= blklen;
	}
      }
    } else {
      if (src->sampformat == LIN16) {
	while (tot < len) {
	  sn = (from + tot) >> SEXP;
	  si = (from + tot) - (sn << SEXP);
	  dn = (to   + tot) >> SEXP;
	  di = (to   + tot) - (dn << SEXP);
	  blklen = min(SBLKSIZE - si, SBLKSIZE - di);
	  blklen = min(blklen, len - tot);
	  memmove(&dest->blocks[dn][di], &src->blocks[sn][si], 
		  blklen*sizeof(short));
	  tot += blklen;
	}
      } else {
	while (tot < len) {
	  sn = (from + tot) >> CEXP;
	  si = (from + tot) - (sn << CEXP);
	  dn = (to   + tot) >> CEXP;
	  di = (to   + tot) - (dn << CEXP);
	  blklen = min(CBLKSIZE - si, CBLKSIZE - di);
	  blklen = min(blklen, len - tot);
	  memmove(&((unsigned char **)dest->blocks)[dn][di],
		  &((unsigned char **)src->blocks)[sn][si], blklen);
	  tot += blklen;
	}
      }
    }
  } else if (dest->storeType == SOUND_IN_FILE) {
  }
}

void
Snack_PutSoundData(Sound *s, int pos, void *buf, int nBytes)
{
  int dn, di, tot = 0, blklen, nSamples = nBytes / s->sampsize;

  if (s->storeType != SOUND_IN_MEMORY) {
    return;
  }

  if (s->sampformat == LIN16) {
    while (tot < nSamples) {
      dn = (pos + tot) >> SEXP;
      di = (pos + tot) - (dn << SEXP);
      blklen = min(SBLKSIZE - di, nSamples - tot);
      memmove(&s->blocks[dn][di], buf, blklen * sizeof(short));
      tot += blklen;
    }
  } else {
    while (tot < nSamples) {
      dn = (pos + tot) >> CEXP;
      di = (pos + tot) - (dn << CEXP);
      blklen = min(CBLKSIZE - di, nSamples - tot);
      memmove(&((unsigned char **)s->blocks)[dn][di], buf, blklen);
      tot += blklen;
    }
  }
}

void
Snack_GetSoundData(Sound *s, int pos, void *buf, int nBytes)
{
  int sn, si, tot = 0, blklen, nSamples = nBytes / s->sampsize;

  if (s->storeType == SOUND_IN_MEMORY) {
    if (s->sampformat == LIN16) {
      while (tot < nSamples) {
	sn = (pos + tot) >> SEXP;
	si = (pos + tot) - (sn << SEXP);
	blklen = min(SBLKSIZE - si, nSamples - tot);
	memmove(&((short *)buf)[tot], &s->blocks[sn][si], blklen*sizeof(short));
	tot += blklen;
      }
    } else {
      while (tot < nSamples) {
	sn = (pos + tot) >> CEXP;
	si = (pos + tot) - (sn << CEXP);
	blklen = min(CBLKSIZE - si, nSamples - tot);
	memmove(&((unsigned char *)buf)[tot],
		&((unsigned char **)s->blocks)[sn][si], blklen);
	tot += blklen;
      }
    }
  } else if (s->storeType == SOUND_IN_FILE) {
    SnackLinkedFileInfo info;
    int i;

    OpenLinkedFile(s, &info);

    for (i = 0; i < nSamples; i++) {
      if (s->sampformat == LIN16) {
	((short *)buf)[i] = (short) GetSample(&info, i);
      } else {
	((unsigned char *)buf)[i] = (unsigned char) GetSample(&info, i);
      }
    }

    CloseLinkedFile(&info);
  }
}

int
lengthCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int arg, len, type = 0, newlen = -1, i;
  char *string = NULL;
      
  if (objc >= 3) {
    for (arg = 2; arg < objc; arg++) {
      string = Tcl_GetStringFromObj(objv[arg], &len);
      
      if (strncmp(string, "-units", len) == 0) {
	string = Tcl_GetStringFromObj(objv[arg+1], &len);
	if (strncasecmp(string, "seconds", len) == 0) type = 1;
	if (strncasecmp(string, "samples", len) == 0) type = 0;
	arg++;
      } else if (Tcl_GetIntFromObj(interp, objv[2], &newlen) != TCL_OK) {
	return TCL_ERROR;
      }
    }
  }
  
  if (newlen < 0) {
    if (type == 0) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(s->length));
    } else {
      Tcl_SetObjResult(interp, Tcl_NewDoubleObj((float)s->length/s->sampfreq));
    }
  } else {
    if (type == 1) {
      newlen *= s->sampfreq;
    }
    if (newlen > s->length) {
      if (Snack_ResizeSoundStorage(s, newlen) != TCL_OK) {
	return TCL_ERROR;
      }
      for (i = s->length * s->nchannels; i < newlen * s->nchannels; i++) {
	switch (s->sampformat) {
	case LIN16:
	  SSAMPLE(s, i) = 0;
	  break;
	case ALAW:
	  UCSAMPLE(s, i) = Snack_Lin2Alaw(0);
	  break;
	case MULAW:
	  UCSAMPLE(s, i) = Snack_Lin2Mulaw(0);
	  break;
	case LIN8OFFSET:
	  UCSAMPLE(s, i) = 128;
	  break;
	case LIN8:
	  UCSAMPLE(s, i) = 0;
	  break;
	}
      }
    }
    s->length = newlen;
    Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
    Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
  }

  return TCL_OK;
}

int
insertCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Sound *ins;
  int inspoint, arg, startpos = 0, endpos = -1;
  char *string;
  static char *subOptionStrings[] = {
    "-start", "-end", NULL
  };
  enum subOptions {
    START, END
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "insert only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
      
  if (objc < 4) {
    Tcl_WrongNumArgs(interp, 1, objv, "insert sound sample");
    return TCL_ERROR;
  }

  string = Tcl_GetStringFromObj(objv[2], NULL);

  if ((ins = Snack_GetSound(interp, string)) == NULL) {
    return TCL_ERROR;
  }

  if (Tcl_GetIntFromObj(interp, objv[3], &inspoint) != TCL_OK) {
    return TCL_ERROR;
  }

  if (inspoint < 0 || inspoint > s->length) {
    Tcl_AppendResult(interp, "Insertion point out of bounds", NULL);
    return TCL_ERROR;
  }
      
  if (s->sampformat != ins->sampformat || s->nchannels != ins->nchannels) {
    Tcl_AppendResult(interp, "Sound format differs: ", string, NULL);
    return TCL_ERROR;
  }

  for (arg = 4; arg < objc; arg += 2) {
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
    }
  }
  if (startpos < 0) startpos = 0;
  if (endpos >= (ins->length - 1) || endpos == -1)
    endpos = ins->length - 1;
  if (startpos > endpos) return TCL_OK;

  if (Snack_ResizeSoundStorage(s, s->length + ins->length) != TCL_OK) {
    return TCL_ERROR;
  }
  SmpMov(s, inspoint + endpos - startpos + 1, s, inspoint, s->length - inspoint);
  SmpMov(s, inspoint, ins, startpos, endpos - startpos + 1);
  s->length += (endpos - startpos + 1);
  Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
cropCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int startpos, endpos, totlen;

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "crop only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 1, objv, "crop start end");
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp, objv[2], &startpos) != TCL_OK) return TCL_ERROR;
  if (Tcl_GetIntFromObj(interp, objv[3], &endpos) != TCL_OK) return TCL_ERROR;
      
  if ((endpos >= s->length - 1) || endpos < 0)
    endpos = s->length - 1;
  if (startpos >= endpos) return TCL_OK;
  totlen = endpos - startpos + 1;

  SmpMov(s, 0, s, startpos, totlen);
  s->length = totlen;      
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
copyCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int arg, startpos = 0, endpos = -1;
  Sound *master;
  char *string;
  static char *subOptionStrings[] = {
    "-start", "-end", NULL
  };
  enum subOptions {
    START, END
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "copy only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc < 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "copy sound");
    return TCL_ERROR;
  }
      
  string = Tcl_GetStringFromObj(objv[2], NULL);

  if ((master = Snack_GetSound(interp, string)) == NULL) {
    return TCL_ERROR;
  }

  if (master->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "can only copy from in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  for (arg = 3; arg < objc; arg += 2) {
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
    }
  }
  if (startpos < 0) startpos = 0;
  if (endpos >= (master->length - 1) || endpos == -1)
    endpos = master->length - 1;
  if (startpos > endpos) return TCL_OK;

  if (s->active == WRITE && s->sampformat != master->sampformat) {
    Snack_StopSound(s, interp);
  }
  s->sampfreq = master->sampfreq;
  s->sampformat = master->sampformat;
  s->sampsize = master->sampsize;
  s->nchannels = master->nchannels;
  s->length = endpos - startpos + 1;
  if (Snack_ResizeSoundStorage(s, s->length) != TCL_OK) {
    return TCL_ERROR;
  }
  SmpMov(s, 0, master, startpos, s->length);
  s->maxsamp = master->maxsamp;
  s->minsamp = master->minsamp;
  s->abmax = master->abmax;
  s->active = IDLE;
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
appendCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Sound *t;
  int arg, startpos = 0, endpos = -1;
  char *filetype;
  static char *subOptionStrings[] = {
    "-frequency", "-skiphead", "-byteorder", "-channels", "-format",
    "-start", "-end", "-fileformat", "-guessproperties", NULL
  };
  enum subOptions {
    FREQUENCY, SKIPHEAD, BYTEORDER, CHANNELS, FORMAT, START, END, FILEFORMAT,
    GUESSPROPS
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "append only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if ((t = Snack_NewSound(s->sampfreq, s->sampformat, s->nchannels)) == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate new sound!", NULL);
    return TCL_ERROR;
  }
  t->debug = s->debug;
  t->guessFrequency = 0;
  t->swap = 0;

  for (arg = 3; arg < objc; arg += 2) {
    int index;

    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			    "option", 0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    switch ((enum subOptions) index) {
    case FREQUENCY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &t->sampfreq) != TCL_OK) {
	  return TCL_ERROR;
	}
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
	  SwapIfBE(t);
	} else if (strncasecmp(str, "bigEndian", length) == 0) {
	  SwapIfLE(t);
	} else {
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
	  return TCL_ERROR;
	}
	t->guessFormat = 0;
	break;
      }
    case CHANNELS:
      {
	if (GetChannels(interp, objv[arg+1], &t->nchannels) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case FORMAT:
      {
	if (GetFormat(interp, objv[arg+1], &t->sampformat, &t->sampsize) != TCL_OK)
	  return TCL_ERROR;
	t->guessFormat = 0;
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
	t->forceFormat = 1;
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

  filetype = LoadSound(t, interp, objv[2], startpos, endpos);
  if (filetype == NULL) {
    Snack_DeleteSound(t);
    return TCL_ERROR;
  }
  if (s->sampformat != t->sampformat || s->nchannels != t->nchannels) {
    Snack_DeleteSound(t);
    Tcl_AppendResult(interp, "Sound format differs: ", NULL);
    return TCL_ERROR;
  }

  if (Snack_ResizeSoundStorage(s, s->length + t->length) != TCL_OK) {
    return TCL_ERROR;
  }
  SmpMov(s, s->length, t, 0, t->length);
  Snack_UpdateExtremes(s, s->length, s->length + t->length, SNACK_MORE_SOUND);
  s->length += t->length;
  Snack_ExecCallbacks(s, SNACK_MORE_SOUND);
  Snack_DeleteSound(t);

  return TCL_OK;
}

int
concatenateCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Sound *app;
  char *string;

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, 
		     "concatenate only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "concatenate sound");
    return TCL_ERROR;
  }
      
  string = Tcl_GetStringFromObj(objv[2], NULL);

  if ((app = Snack_GetSound(interp, string)) == NULL) {
    return TCL_ERROR;
  }

  if (s->sampformat != app->sampformat || s->nchannels != app->nchannels) {
    Tcl_AppendResult(interp, "Sound format differs: ", string, NULL);
    return TCL_ERROR;
  }

  if (Snack_ResizeSoundStorage(s, s->length + app->length) != TCL_OK) {
    return TCL_ERROR;
  }
  SmpMov(s, s->length, app, 0, app->length);
  Snack_UpdateExtremes(s, s->length, s->length + app->length, SNACK_MORE_SOUND);
  s->length += app->length;
  Snack_ExecCallbacks(s, SNACK_MORE_SOUND);

  return TCL_OK;
}

int
cutCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int start, end;

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "cut only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }
      
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 1, objv, "cut start end");
    return TCL_ERROR;
  }

  if (Tcl_GetIntFromObj(interp, objv[2], &start) != TCL_OK) {
    return TCL_ERROR;
  }

  if (Tcl_GetIntFromObj(interp, objv[3], &end) != TCL_OK) {
    return TCL_ERROR;
  }

  if (start < 0 || start > s->length - 1) {
    Tcl_AppendResult(interp, "Start point out of bounds", NULL);
    return TCL_ERROR;
  }

  if (end < start || end > s->length - 1) {
    Tcl_AppendResult(interp, "End point out of bounds", NULL);
    return TCL_ERROR;
  }

  SmpMov(s, start, s, end + 1, s->length - end - 1);
  s->length = s->length - (end - start + 1);
  Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
Lowpass(Sound *s, Tcl_Interp *interp, int freq)
{
  int c, i, last, insmp = 0, outsmp;
  double a = 6.28318530718 * freq / s->sampfreq;
  double b = exp(-a / (double)s->sampfreq);
  double out;

  for (c = 0; c < s->nchannels; c++) {
    last = 0;
    for (i = 0; i < s->length; i++) {
      switch (s->sampformat) {
      case LIN16:
	insmp = SSAMPLE(s, (i * s->nchannels + c));
	break;
      case ALAW:
	insmp = Snack_Alaw2Lin(UCSAMPLE(s, (i * s->nchannels + c)));
	break;
      case MULAW:
	insmp = Snack_Mulaw2Lin(UCSAMPLE(s, (i * s->nchannels + c)));
	break;
      case LIN8OFFSET:
	insmp = ((char)UCSAMPLE(s, (i * s->nchannels + c)) ^ 128) << 8;
	break;
      }
      
      out = (double) insmp * a + (double) last * b;
      last = insmp;
      outsmp = (int) (0.4 * out);

      if (outsmp > 32767) {
	outsmp = 32767;
      }
      if (outsmp < -32768) {
	outsmp = -32768;
      }

      switch (s->sampformat) {
      case LIN16:
	SSAMPLE(s, (i * s->nchannels + c)) = outsmp;
	break;
      case ALAW:
	UCSAMPLE(s, (i * s->nchannels + c)) = Snack_Lin2Alaw((short)outsmp);
	break;
      case MULAW:
	UCSAMPLE(s, (i * s->nchannels + c)) = Snack_Lin2Mulaw((short)outsmp);
	break;
      case LIN8OFFSET:
	UCSAMPLE(s, (i * s->nchannels + c)) = (outsmp >> 8) ^ 128;
	break;
      }
    }
  }

  return TCL_OK;
}

/* Fabrice Bellard's resampling algorithm from Sox-12.15 */

#define FRAC_BITS 16

static int
Resample(Sound *s, Tcl_Interp *interp, int newfreq)
{
  int i, j, c, last = 0, insmp = 0, outsmp, incr, tmp;
  int opos_frac = 0, opos = 0, opos_inc_frac, opos_inc;
  double f;
  Sound *t = NULL;

  if (s->length == 0) {
    s->sampfreq = newfreq;
    return TCL_OK;
  }

  incr = (int) ((double) s->sampfreq / (double) newfreq * 
	       (double) (1 << FRAC_BITS));
  opos_inc_frac = incr & ((1 << FRAC_BITS)-1);
  opos_inc = incr >> FRAC_BITS;

  if ((t = Snack_NewSound(newfreq, s->sampformat, s->nchannels)) == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate new sound!", NULL);
    return TCL_ERROR;
  }
  t->debug  = s->debug;
  t->length = (int) (s->length * (float) newfreq / s->sampfreq);
  if (Snack_ResizeSoundStorage(t, t->length) != TCL_OK) {
    return TCL_ERROR;
  }

  for (c = 0; c < s->nchannels; c++) {
    j = 0;

    for (i = 0; i < t->length; i++) {
      while (j <= opos) {
	if (j >= s->length) break;
	switch (s->sampformat) {
	case LIN16:
	  last = SSAMPLE(s, (j * s->nchannels + c));
	  break;
	case ALAW:
	  last = Snack_Alaw2Lin(UCSAMPLE(s, (j * s->nchannels + c)));
	  break;
	case MULAW:
	  last = Snack_Mulaw2Lin(UCSAMPLE(s, (j * s->nchannels + c)));
	  break;
	case LIN8OFFSET:
	  last = ((char)UCSAMPLE(s, (j * s->nchannels + c)) ^ 128) << 8;
	  break;
	}
	j++;
      }
      if (j >= s->length) break;

      switch (s->sampformat) {
      case LIN16:
	insmp = SSAMPLE(s, (j * s->nchannels + c));
	break;
      case ALAW:
	insmp = Snack_Alaw2Lin(UCSAMPLE(s, (j * s->nchannels + c)));
	break;
      case MULAW:
	insmp = Snack_Mulaw2Lin(UCSAMPLE(s, (j * s->nchannels + c)));
	break;
      case LIN8OFFSET:
	insmp = ((char)UCSAMPLE(s, (j * s->nchannels + c)) ^ 128) << 8;
	break;
      }
      
      f = (double) opos_frac / (1 << FRAC_BITS);
      outsmp = (int) ((double) last * (1.0 - f) + (double) insmp * f);

      switch (s->sampformat) {
      case LIN16:
	SSAMPLE(t, (i * s->nchannels + c)) = outsmp;
	break;
      case ALAW:
	UCSAMPLE(t, (i * s->nchannels + c)) = Snack_Lin2Alaw((short)outsmp);
	break;
      case MULAW:
	UCSAMPLE(t, (i * s->nchannels + c)) = Snack_Lin2Mulaw((short)outsmp);
	break;
      case LIN8OFFSET:
	UCSAMPLE(t, (i * s->nchannels + c)) = (outsmp >> 8) ^ 128;
	break;
      }
      
      tmp = opos_frac + opos_inc_frac;
      opos = opos + opos_inc + (tmp >> FRAC_BITS);
      opos_frac = tmp & ((1 << FRAC_BITS)-1);
    }
  }
  s->length = t->length;
  if (Snack_ResizeSoundStorage(s, s->length) != TCL_OK) {
    return TCL_ERROR;
  }
  SmpMov(s, 0, t, 0, t->length);
  Snack_DeleteSound(t);
  Lowpass(s, interp, (int) (0.425 * min(newfreq, s->sampfreq)));
  s->sampfreq = newfreq;

  return TCL_OK;
}

int
convertCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int arg, i, j, start = 0, inc = 1;
  int sampfreq = -1, nchannels = -1, sampformat = -1, sampsize = -1;
  static char *subOptionStrings[] = {
    "-frequency", "-channels", "-format", NULL
  };
  enum subOptions {
    FREQUENCY, CHANNELS, FORMAT
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "convert only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc < 4) {
    Tcl_WrongNumArgs(interp, 1, objv, "convert -option value");
    return TCL_ERROR;
  }

  for (arg = 2; arg < objc; arg += 2) {
    int index;

    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			    "option", 0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    switch ((enum subOptions) index) {
    case FREQUENCY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &sampfreq) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case CHANNELS:
      {
	if (GetChannels(interp, objv[arg+1], &nchannels) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case FORMAT:
      {
	if (GetFormat(interp, objv[arg+1], &sampformat, &sampsize) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }
  
  if (sampfreq != -1) {
    if (Resample(s, interp, sampfreq) != TCL_OK) {
      return TCL_ERROR;
    }
  }

  if (sampformat != -1) {
    if (sampformat == s->sampformat) {
      return TCL_OK;
    }
    if (sampsize > s->sampsize) {
      if (Snack_ResizeSoundStorage(s, s->length * sampsize) != TCL_OK) {
	return TCL_ERROR;
      }
      start = s->length * s->nchannels - 1;
      inc = -1;
    }
    for (i = start; i < s->length * s->nchannels && i >= 0; i += inc) {
      short value = 0;

      switch (s->sampformat) {
      case LIN16:
	value = SSAMPLE(s, i);
	break;
      case ALAW:
	value = Snack_Alaw2Lin(UCSAMPLE(s, i));
	break;
      case MULAW:
	value = Snack_Mulaw2Lin(UCSAMPLE(s, i));
	break;
      case LIN8OFFSET:
	value = ((char)UCSAMPLE(s, i) ^ 128) << 8;
	break;
      case LIN8:
	value = (((char)UCSAMPLE(s, i) ^ 128) + 128) << 8;
	break;
      }

      switch (sampformat) {
      case LIN16:
	SSAMPLE(s, i) = value;
	break;
      case ALAW:
	UCSAMPLE(s, i) = Snack_Lin2Alaw(value);
	break;
      case MULAW:
	UCSAMPLE(s, i) = Snack_Lin2Mulaw(value);
	break;
      case LIN8OFFSET:
	UCSAMPLE(s, i) = (value >> 8) ^ 128;
	break;
      case LIN8:
	UCSAMPLE(s, i) = ((value >> 8) ^ 128) - 128;
	break;
      }
    }

    s->sampformat = sampformat;
    s->sampsize = sampsize;
  }

  if (nchannels != -1) {
    if (nchannels == s->nchannels) {
      return TCL_OK;
    }
    if (nchannels > 1 && s->nchannels > 1) {
      Tcl_AppendResult(interp, "Can only convert n->1 or 1->n channels",
		       (char *) NULL);
      return TCL_ERROR;
    }
    if (nchannels == 1) {
      for (i = 0; i < s->length; i++) {
	int value = 0;

	for (j = 0; j < s->nchannels; j++) {
	  switch (s->sampformat) {
	  case LIN16:
	    value += SSAMPLE(s, i * s->nchannels + j);
	    break;
	  case ALAW:
	    value += Snack_Alaw2Lin(UCSAMPLE(s, i * s->nchannels + j));
	    break;
	  case MULAW:
	    value += Snack_Mulaw2Lin(UCSAMPLE(s, i * s->nchannels + j));
	    break;
	  case LIN8OFFSET:
	    value += ((char)UCSAMPLE(s, i * s->nchannels + j) ^ 128) << 8;
	    break;
	  case LIN8:
	    value += (((char)UCSAMPLE(s, i * s->nchannels + j)^128) +128) << 8;
	    break;
	  }
	}
	value = value / s->nchannels;
	
	switch (s->sampformat) {
	case LIN16:
	  SSAMPLE(s, i) = value;
	  break;
	case ALAW:
	  UCSAMPLE(s, i) = Snack_Lin2Alaw((short)value);
	  break;
	case MULAW:
	  UCSAMPLE(s, i) = Snack_Lin2Mulaw((short)value);
	  break;
	case LIN8OFFSET:
	  UCSAMPLE(s, i) = (value >> 8) ^ 128;
	  break;
	case LIN8:
	  UCSAMPLE(s, i) = ((value >> 8) ^ 128) - 128;
	  break;
	}
      }
    }
    if (s->nchannels == 1) {
      if (Snack_ResizeSoundStorage(s, s->length * nchannels) != TCL_OK) {
	return TCL_ERROR;
      }
      for (i = s->length - 1; i >= 0; i--) {
	int sn = (i * s->sampsize) >> CEXP;
	int si = (i * s->sampsize) - (sn << CEXP);
	for (j = 0; j < nchannels; j++) {
	  int dn = ((i * nchannels + j)* s->sampsize) >> CEXP;
	  int di = ((i * nchannels + j)* s->sampsize) - (dn << CEXP);
	  memmove(&((unsigned char **)s->blocks)[dn][di],
		  &((unsigned char **)s->blocks)[sn][si],
		  sizeof(char) * s->sampsize);
	}
      }
    }
    s->nchannels = nchannels;
  }
  
  Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
reverseCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int arg, startpos = 0, endpos = -1, i, j;
  static char *subOptionStrings[] = {
    "-start", "-end", NULL
  };
  enum subOptions {
    START, END
  };

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "reverse only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "reverse");
    return TCL_ERROR;
  }
      
  for (arg = 2; arg < objc; arg += 2) {
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
    }
  }
  if (startpos < 0) startpos = 0;
  if (endpos >= (s->length - 1) || endpos == -1)
    endpos = s->length - 1;
  if (startpos > endpos) return TCL_OK;

  if (s->active == WRITE) {
    Snack_StopSound(s, interp);
  }

  for (i = startpos * s->nchannels, j = endpos * s->nchannels;
       i <= (startpos + (endpos - startpos) / 2) * s->nchannels; i++, j--) {
    switch (s->sampformat) {
    case LIN16: {
      short swap = SSAMPLE(s, i);
      SSAMPLE(s, i) = SSAMPLE(s, j);
      SSAMPLE(s, j) = swap;
      break;
    }
    case ALAW:
    case MULAW: {
      char swap = UCSAMPLE(s, i);
      UCSAMPLE(s, i) = UCSAMPLE(s, j);
      UCSAMPLE(s, j) = swap;
      break;
    }
    case LIN8OFFSET:
    case LIN8: {
      unsigned char swap = UCSAMPLE(s, i);
      UCSAMPLE(s, i) = UCSAMPLE(s, j);
      UCSAMPLE(s, j) = swap;
    }
    break;
    }
  }
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);
  
  return TCL_OK;
}

/*

  The following functions add interoperability between Snack and the CSLU
  Speech Toolkit. Two functions are provided to convert Snack sound objects
  into CSLUsh wave objects and vice versa.

 */

#ifdef SNACK_CSLU_TOOLKIT

#include <dballoc.h>
#include <result.h>
#include <wave.h>

#include <vec.h>
#include <utils.h>
#include <cmds.h>
#include <obj.h>

int
fromCSLUshWaveCmd(Sound *s, Tcl_Interp *interp, int objc,Tcl_Obj *CONST objv[])
{
  Wave *w;
  char *handle;

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "fromCSLUshWave only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "fromCSLUshWave waveObj");
    return TCL_ERROR;
  }

  handle = Tcl_GetStringFromObj(objv[2], NULL);
  if (!(w = Obj_GetData(interp, WAVE, handle))) {
    Tcl_AppendResult(interp, "Failed getting data from waveObj: ",
		     handle, NULL);
    return TCL_ERROR;
  }
  if (w->attr[WAVE_TYPE] != WAVE_TYPE_LINEAR) {
    Tcl_AppendResult(interp, "waveObj must be WAVE_TYPE_LINEAR", NULL);
    return TCL_ERROR;
  }
  if (s->active == WRITE) {
    Snack_StopSound(s, interp);
  }
  s->sampfreq = (int) w->attr[WAVE_RATE];
  s->sampformat = LIN16;
  s->sampsize = sizeof(short);
  s->nchannels = 1;
  s->length = w->len;
  if (Snack_ResizeSoundStorage(s, s->length) != TCL_OK) {
    return TCL_ERROR;
  }
  Snack_PutSoundData(s, 0, w->samples, w->len * s->sampsize);
  Snack_UpdateExtremes(s, 0, s->length, SNACK_NEW_SOUND);
  Snack_ExecCallbacks(s, SNACK_NEW_SOUND);

  return TCL_OK;
}

int
toCSLUshWaveCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  Wave *w;
  float attr[WAVE_ATTRIBUTES];

  if (s->storeType != SOUND_IN_MEMORY) {
    Tcl_AppendResult(interp, "toCSLUshWave only works with in-memory sounds",
		     (char *) NULL);
    return TCL_ERROR;
  }

  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "toCSLUshWave");
    return TCL_ERROR;
  }
  if (s->sampformat != LIN16 || s->nchannels != 1) {
    Tcl_AppendResult(interp, "Sorry, only implemented for lin16, mono sounds",
		     NULL);
    return TCL_ERROR;
  }

  attr[WAVE_RATE] = (float) s->sampfreq;
  attr[WAVE_TYPE] = 0;
  /* Doesn't handle large sounds yet > 512kB */
  if(createWave(interp, s->length, WAVE_ATTRIBUTES, attr, s->blocks[0], &w) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}
#endif

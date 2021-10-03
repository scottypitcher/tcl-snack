/* 
 * Copyright (C) 1997-2001 Kare Sjolander <kare@speech.kth.se>
 *
 * This file is part of the Snack Sound Toolkit.
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

#include "tcl.h"
#include "snack.h"
#include <string.h>
#include <math.h>

#define SNACK_PI 3.141592653589793

void
Snack_InitWindow(float *win, int winlen, int fftlen, int type)
{
  int i;

  if (winlen > fftlen)
    winlen = fftlen;

  if (type == SNACK_WIN_RECT) {
    for (i = 0; i < winlen; i++)
      win[i] = (float) 1.0;
  } else if (type == SNACK_WIN_HANNING) {
    for (i = 0; i < winlen; i++)
      win[i] = (float)(0.5 * (1.0 - cos(i * 2.0 * SNACK_PI / (winlen - 1))));
  } else if (type == SNACK_WIN_BARTLETT) {
    for (i = 0; i < winlen/2; i++)
      win[i] = (float)(((2.0 * i) / (winlen - 1)));
    for (i = winlen/2; i < winlen; i++)
      win[i] = (float)(2.0 * (1.0 - ((float)i / (winlen - 1))));
  } else if (type == SNACK_WIN_BLACKMAN) {
    for (i = 0; i < winlen; i++)
      win[i] = (float)((0.42 - 0.5 * cos(i * 2.0 * SNACK_PI / (winlen - 1)) 
			+ 0.08 * cos(i * 4.0 * SNACK_PI / (winlen - 1))));
  } else {  /* default: Hamming window */
    for (i = 0; i < winlen; i++)
      win[i] = (float)(0.54 - 0.46 * cos(i * 2.0 * SNACK_PI / (winlen - 1)));
  }
  
  for (i = winlen; i < fftlen; i++)
    win[i] = 0.0;
}

int
CheckFFTlen(Tcl_Interp *interp, int fftlen)
{
  int n = NMIN;
  char str[10];

  while (n <= NMAX) {
    if (n == fftlen) return TCL_OK;
    n *= 2;
  }

  Tcl_AppendResult(interp, "-fftlength must be one of { ", NULL);

  for (n = NMIN; n <= NMAX; n*=2) {
    sprintf(str, "%d ", n);
    Tcl_AppendResult(interp, str, NULL);
  }
  Tcl_AppendResult(interp, "}", NULL);
  return TCL_ERROR;
}

int
CheckWinlen(Tcl_Interp *interp, int winlen)
{
  if (winlen < 1) {
    Tcl_AppendResult(interp, "-winlength must be > 0", NULL);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

static float xfft[NMAX];
static float ffts[NMAX];
static float hamwin[NMAX];
#define SNACK_DEFAULT_DBPWINTYPE SNACK_WIN_HAMMING

int
dBPowerSpectrumCmd(Sound *s, Tcl_Interp *interp, int objc,
		   Tcl_Obj *CONST objv[])
{
  double dpreemph = 0.0;
  float preemph = 0.0;
  int i, j, n = 0, arg;
  int channel = 0, winlen = 256, fftlen = 512;
  int startpos = 0, endpos = -1, skip = -1;
  Tcl_Obj *list;
  SnackLinkedFileInfo info;
  SnackWindowType wintype = SNACK_DEFAULT_DBPWINTYPE;
  static char *subOptionStrings[] = {
    "-start", "-end", "-channel", "-fftlength", "-winlength", "-windowlength",
    "-preemphasisfactor", "-skip", "-windowtype", NULL
  };
  enum subOptions {
    START, END, CHANNEL, FFTLEN, WINLEN, WINDOWLEN, PREEMPH, SKIP, WINTYPE
  };

  if (s->debug > 0) Snack_WriteLog("Enter dBPowerSpectrumCmd\n");

  for (arg = 2; arg < objc; arg += 2) {
    int index;
	
    if (Tcl_GetIndexFromObj(interp, objv[arg], subOptionStrings,
			    "option", 0, &index) != TCL_OK) {
      return TCL_ERROR;
    }
	
    if (arg + 1 == objc) {
      Tcl_AppendResult(interp, "No argument given for ",
		       subOptionStrings[index], " option", (char *) NULL);
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
    case FFTLEN:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &fftlen) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case WINDOWLEN:
    case WINLEN:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &winlen) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case PREEMPH:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &dpreemph) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case SKIP:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &skip) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case WINTYPE:
      {
	char *str = Tcl_GetStringFromObj(objv[arg+1], NULL);
	if (GetWindowType(interp, str, &wintype) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }

  if (CheckFFTlen(interp, fftlen) != TCL_OK) return TCL_ERROR;

  if (CheckWinlen(interp, winlen) != TCL_OK) return TCL_ERROR;

  preemph = (float) dpreemph;

  if (startpos < 0 || startpos > s->length - fftlen) {
    Tcl_AppendResult(interp, "FFT window out of bounds", NULL);
    return TCL_ERROR;
  }

  if (endpos < 0) 
    endpos = startpos;

  if (endpos > s->length - 1) {
    Tcl_AppendResult(interp, "FFT window out of bounds", NULL);
    return TCL_ERROR;
  }

  for (i = 0; i < NMAX/2; i++) {
    ffts[i] = 0.0;
  }

  if (s->storeType != SOUND_IN_MEMORY) {
    if (OpenLinkedFile(s, &info) != TCL_OK) {
      return TCL_OK;
    }
  }

  Snack_InitWindow(hamwin, winlen, fftlen, wintype);

  Snack_InitFFT(fftlen);

  if (skip < 1) {
    skip = fftlen;
  }
  n = (endpos - startpos) / skip;
  if (n < 1) {
    n = 1;
  }

  if (s->nchannels == 1) {
    channel = 0;
  }

  for (j = 0; j < n; j++) {
    if (s->storeType == SOUND_IN_MEMORY) {
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * skip) * s->nchannels + channel;

	for (i = 0; i < fftlen; i++) {
	  xfft[i] = (float) ((FSAMPLE(s, p + s->nchannels)
			      - preemph * FSAMPLE(s, p))
			     * hamwin[i]);
	    p += s->nchannels;
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * skip) * s->nchannels + c;
	  
	  for (i = 0; i < fftlen; i++) {
	    xfft[i] += (float) ((FSAMPLE(s, p + s->nchannels)
				 - preemph * FSAMPLE(s, p))
				* hamwin[i]);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < fftlen; i++) {
	  xfft[i] /= s->nchannels;
	}
      }
    } else { /* storeType != SOUND_IN_MEMORY */
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * skip) * s->nchannels + channel;

	for (i = 0; i < fftlen; i++) {
	  xfft[i] = (float) ((GetSample(&info, p + s->nchannels)
			      - preemph * GetSample(&info, p))
			     * hamwin[i]);
	  p += s->nchannels;
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * skip) * s->nchannels + c;
	  
	  for (i = 0; i < fftlen; i++) {
	    xfft[i] += (float) ((GetSample(&info, p + s->nchannels)
				 - preemph * GetSample(&info, p))
				* hamwin[i]);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < fftlen; i++) {
	  xfft[i] /= s->nchannels;
	}
      }
    }

    Snack_DBPowerSpectrum(xfft);
    
    for (i = 0; i < fftlen/2; i++) {
      ffts[i] += xfft[i];
    }
  }

  if (s->storeType != SOUND_IN_MEMORY) {
    CloseLinkedFile(&info);
  }

  for (i = 0; i < fftlen/2; i++) {
    ffts[i] = ffts[i] / (float) n;
  }

  list = Tcl_NewListObj(0, NULL);
  for (i = 0; i < fftlen/2; i++) {
    Tcl_ListObjAppendElement(interp, list, Tcl_NewDoubleObj(ffts[i]));
  }

  Tcl_SetObjResult(interp, list);

  if (s->debug > 0) Snack_WriteLog("Exit dBPowerSpectrumCmd\n");

  return TCL_OK;
}

int
GetChannel(Tcl_Interp *interp, char *str, int nchan, int *channel)
{
  int n = -2;
  int len = strlen(str);

  if (strncasecmp(str, "left", len) == 0) {
    n = 0;
  } else if (strncasecmp(str, "right", len) == 0) {
    n = 1;
  } else if (strncasecmp(str, "all", len) == 0) {
    n = -1;
  } else if (strncasecmp(str, "both", len) == 0) {
    n = -1;
  } else {
    Tcl_GetInt(interp, str, &n);
  }

  if (n < -1 || n >= nchan) {
    Tcl_AppendResult(interp, "-channel must be left, right, both, all, -1, or an integer between 0 and the number channels - 1", NULL);
    return TCL_ERROR;
  }

  *channel = n;

  return TCL_OK;
}

int
GetWindowType(Tcl_Interp *interp, char *str, SnackWindowType *wintype)
{
  SnackWindowType type = -1;
  int len = strlen(str);
  
  if (strncasecmp(str, "hamming", len) == 0) {
    type = SNACK_WIN_HAMMING;
  } else if (strncasecmp(str, "hanning", len) == 0) {
    type = SNACK_WIN_HANNING;
  } else if (strncasecmp(str, "bartlett", len) == 0) {
    type = SNACK_WIN_BARTLETT;
  } else if (strncasecmp(str, "blackman", len) == 0) {
    type = SNACK_WIN_BLACKMAN;
  } else if (strncasecmp(str, "rectangle", len) == 0) {
    type = SNACK_WIN_RECT;
  }
  
  if (type == -1) {
    Tcl_AppendResult(interp, "-windowtype must be hanning, hamming, bartlett,"
		     "blackman, or rectangle", NULL);
    return TCL_ERROR;
  }
  
  *wintype = type;

  return TCL_OK;
}

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

#include "tcl.h"
#include <string.h>
#include "jkAudIO.h"
#include "jkSound.h"
#include <math.h>

#define SNACK_PI 3.141592653589793

void
Snack_InitWindow(float *win, int winlen, int fftlen, int type)
{
  int i;

  for (i = 0; i < winlen; i++)
    win[i] = (float)((0.54 - 0.46 * cos(i * 2.0 * SNACK_PI / (winlen - 1))) / 32768.0);
  
  for (i = winlen; i < fftlen; i++)
    win[i] = 0.0;
}

int
fftCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  float xfft[NMAX];
  float ffts[NMAX];
  float hamwin[NMAX];
  double preemph = 0.0;
  int i, j, n = 1, arg;
  int channel = 0, winlen = 256, fftlen = 512, startpos = 0;
  int sampformat = s->sampformat;
  Tcl_Obj *list;
  SnackLinkedFileInfo info;
  static char *subOptionStrings[] = {
    "-start", "-channel", "-fftlength", "-winlength", "-windowlength",
    "-preemphasisfactor", NULL
  };
  enum subOptions {
    START, CHANNEL, FFTLEN, WINLEN, WINDOWLEN, PREEMPH
  };

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
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &preemph) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }

  if (startpos < 0 || startpos > s->length - fftlen) {
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

  Snack_InitWindow(hamwin, winlen, fftlen, SNACK_HAMMING);

  Snack_InitFFT(fftlen);

  for (j = 0; j < n; j++) {
    if (s->storeType == SOUND_IN_MEMORY) {
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * fftlen) * s->nchannels + channel;
	if (sampformat == LIN16) {
	  for (i = 0; i < fftlen; i++) {
	    xfft[i] = (float) ((SSAMPLE(s, p + s->nchannels)
				- preemph * SSAMPLE(s, p))
			       * hamwin[i]);
	    p += s->nchannels;
	  }
	} else {
	  for (i = 0; i < fftlen; i++) {
	    if (sampformat == MULAW) {
	      xfft[i] = (float) ((Snack_Mulaw2Lin(UCSAMPLE(s, p + s->nchannels)) - preemph * Snack_Mulaw2Lin(UCSAMPLE(s, p))) * hamwin[i]);
	    } else if (sampformat == ALAW) {
	      xfft[i] = (float) ((Snack_Alaw2Lin(UCSAMPLE(s, p + s->nchannels)) - preemph * Snack_Alaw2Lin(UCSAMPLE(s, p))) * hamwin[i]);
	    } else {
	      xfft[i] = (float) ((UCSAMPLE(s, p + s->nchannels) - preemph * UCSAMPLE(s, p)) * hamwin[i]);
	    }
	    p += s->nchannels;
	  }
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * fftlen) * s->nchannels + c;
	  
	  if (sampformat == LIN16) {
	    for (i = 0; i < fftlen; i++) {
	      xfft[i] += (float) ((SSAMPLE(s, p + s->nchannels)
				   - preemph * SSAMPLE(s, p))
				  * hamwin[i]);
	      p += s->nchannels;
	    }
	  } else {
	    for (i = 0; i < fftlen; i++) {
	      if (sampformat == MULAW) {
		xfft[i] += (float) ((Snack_Mulaw2Lin(UCSAMPLE(s, p + s->nchannels)) - preemph * Snack_Mulaw2Lin(UCSAMPLE(s, p))) * hamwin[i]);
	      } else if (sampformat == ALAW) {
		xfft[i] += (float) ((Snack_Alaw2Lin(UCSAMPLE(s, p + s->nchannels)) - preemph * Snack_Alaw2Lin(UCSAMPLE(s, p))) * hamwin[i]);
	      } else {
		xfft[i] += (float) ((UCSAMPLE(s, p + s->nchannels) - preemph * UCSAMPLE(s, p)) * hamwin[i]);
	      }
	      p += s->nchannels;
	    }
	  }
	}
	for (i = 0; i < fftlen; i++) {
	  xfft[i] /= s->nchannels;
	}
      }
    } else { /* storeType != SOUND_IN_MEMORY */
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * fftlen) * s->nchannels + channel;
	if (sampformat == LIN16) {
	  for (i = 0; i < fftlen; i++) {
	    xfft[i] = (float) ((GetSample(&info, p + s->nchannels)
				- preemph * GetSample(&info, p))
			       * hamwin[i]);
	    p += s->nchannels;
	  }
	} else {
	  for (i = 0; i < fftlen; i++) {
	    if (sampformat == MULAW) {
	      xfft[i] = (float) ((Snack_Mulaw2Lin((unsigned char)GetSample(&info, p + s->nchannels)) - preemph * Snack_Mulaw2Lin((unsigned char)GetSample(&info, p))) * hamwin[i]);
	    } else if (sampformat == ALAW) {
	      xfft[i] = (float) ((Snack_Alaw2Lin((unsigned char)GetSample(&info, p + s->nchannels)) - preemph * Snack_Alaw2Lin((unsigned char)GetSample(&info, p))) * hamwin[i]);
	    } else {
	      xfft[i] = (float) ((GetSample(&info, p + s->nchannels) - preemph * GetSample(&info, p)) * hamwin[i]);
	    }
	    p += s->nchannels;
	  }
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * fftlen) * s->nchannels + c;
	  
	  if (sampformat == LIN16) {
	    for (i = 0; i < fftlen; i++) {
	      xfft[i] += (float) ((GetSample(&info, p + s->nchannels)
				   - preemph * GetSample(&info, p))
				  * hamwin[i]);
	      p += s->nchannels;
	    }
	  } else {
	    for (i = 0; i < fftlen; i++) {
	      if (sampformat == MULAW) {
		xfft[i] += (float) ((Snack_Mulaw2Lin((unsigned char)GetSample(&info, p + s->nchannels)) - preemph * Snack_Mulaw2Lin((unsigned char)GetSample(&info, p))) * hamwin[i]);
	      } else if (sampformat == ALAW) {
		xfft[i] += (float) ((Snack_Alaw2Lin((unsigned char)GetSample(&info, p + s->nchannels)) - preemph * Snack_Alaw2Lin((unsigned char)GetSample(&info, p))) * hamwin[i]);
	      } else {
		xfft[i] += (float) ((GetSample(&info, p + s->nchannels) - preemph * GetSample(&info, p)) * hamwin[i]);
	      }
	      p += s->nchannels;
	    }
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

  list = Tcl_NewListObj(0, NULL);
  for (i = 0; i < fftlen/2; i++) {
    Tcl_ListObjAppendElement(interp, list, Tcl_NewDoubleObj(ffts[i]));
  }

  Tcl_SetObjResult(interp, list);

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

/* 
 * Copyright (C) 1997-2002 Kare Sjolander <kare@speech.kth.se>
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
CheckWinlen(Tcl_Interp *interp, int winlen, int fftlen)
{
  char str[10];

  if (winlen < 1) {
    Tcl_AppendResult(interp, "-winlength must be > 0", NULL);
    return TCL_ERROR;
  }
  if (winlen > fftlen) {
    Tcl_AppendResult(interp, "-winlength must be <= fftlength (", NULL);
    sprintf(str, "%d)", fftlen);
    Tcl_AppendResult(interp, str, NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

void
GetFloatMonoSig(Sound *s,SnackLinkedFileInfo *info,
		float *sig, int beg, int len, int channel) {
  /* sig buffer must be allocated, file must be open! */

  int i;

  if (s->storeType == SOUND_IN_MEMORY) {
    if (s->nchannels == 1 || channel != -1) {
      int p = beg * s->nchannels + channel;

      for (i = 0; i < len; i++) {
	sig[i] = (float) (FSAMPLE(s, p));
	p += s->nchannels;
      }
    } else {
      int c;

      for (i = 0; i < len; i++) {
	sig[i] = 0.0;
      }
      for (c = 0; c < s->nchannels; c++) {
	int p = beg * s->nchannels + c;
	  
	for (i = 0; i < len; i++) {
	  sig[i] += (float) (FSAMPLE(s, p));
	  p += s->nchannels;
	}
      }
      for (i = 0; i < len; i++) {
	sig[i] /= s->nchannels;
      }
    }
  } else { /* storeType != SOUND_IN_MEMORY */
    if (s->nchannels == 1 || channel != -1) {
      int p = beg * s->nchannels + channel;

      for (i = 0; i < len; i++) {
	sig[i] = (float) (GetSample(info, p));
	p += s->nchannels;
      }
    } else {
      int c;
	
      for (i = 0; i < len; i++) {
	sig[i] = 0.0;
      }
      for (c = 0; c < s->nchannels; c++) {
	int p = beg * s->nchannels + c;
	  
	for (i = 0; i < len; i++) {
	  sig[i] += (float) (GetSample(info, p));
	  p += s->nchannels;
	}
      }
      for (i = 0; i < len; i++) {
	sig[i] /= s->nchannels;
      }
    }
  }
}

static float xfft[NMAX];
static float ffts[NMAX];
static float hamwin[NMAX];
#define SNACK_DEFAULT_DBPWINTYPE SNACK_WIN_HAMMING
#define SNACK_DEFAULT_DBP_LPC_ORDER 20
#define SNACK_MAX_LPC_ORDER  40

int
CheckLPCorder(Tcl_Interp *interp, int lpcorder)
{
  char str[10];
  if (lpcorder < 1) {
    Tcl_AppendResult(interp, "-lpcorder must be > 0", NULL);
    return TCL_ERROR;
  }
  if (lpcorder > SNACK_MAX_LPC_ORDER) {
    Tcl_AppendResult(interp, "-lpcorder must be <= ", NULL);
    sprintf(str, "%d)", SNACK_MAX_LPC_ORDER);
    Tcl_AppendResult(interp, str, NULL);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

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
    "-preemphasisfactor", "-skip", "-windowtype", "-analysistype",
    "-lpcorder", NULL
  };
  enum subOptions {
    START, END, CHANNEL, FFTLEN, WINLEN, WINDOWLEN, PREEMPH, SKIP, WINTYPE,
    ANATYPE, LPCORDER
  };
  float *sig_lpc;
  float presample = 0.0;
  int siglen, type = 0, lpcOrder = SNACK_DEFAULT_DBP_LPC_ORDER;
  float g_lpc;

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
    case ANATYPE:
      {
	int len;
 	char *str = Tcl_GetStringFromObj(objv[arg+1], &len);
	
	if (strncasecmp(str, "lpc", len) == 0) {
	  type = 1;
	} else if (strncasecmp(str, "fft", len) == 0) {
	  type = 0;
	} else {
	  Tcl_AppendResult(interp, "-type should be FFT or LPC", (char *)NULL);
	  return TCL_ERROR;
	}
	break;
      }
    case LPCORDER:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &lpcOrder) != TCL_OK)
	  return TCL_ERROR;
	if (CheckLPCorder(interp, lpcOrder) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }

  if (CheckFFTlen(interp, fftlen) != TCL_OK) return TCL_ERROR;

  if (CheckWinlen(interp, winlen, fftlen) != TCL_OK) return TCL_ERROR;

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
  siglen = endpos - startpos;
  n = siglen / skip;
  if (n < 1) {
    n = 1;
  }

  if (s->nchannels == 1) {
    channel = 0;
  }

  if (type != 0 && n > 0) { /* LPC + FFT */
    if (siglen < fftlen) siglen = fftlen;
    sig_lpc = (float *) ckalloc(siglen * sizeof(float));

    GetFloatMonoSig(s, &info, sig_lpc, startpos, siglen, channel);
    if (startpos > 0)
      GetFloatMonoSig(s, &info, &presample, startpos - 1, 1, channel);
    PreEmphase(sig_lpc, presample, siglen, preemph);

    /* windowing signal to make lpc look more like the fft spectrum ??? */
    for (i = 0; i < winlen/2; i++) {
      sig_lpc[i] = sig_lpc[i] * hamwin[i];
    }
    for (i = winlen/2; i < winlen; i++) {
      sig_lpc[i + siglen - winlen] = sig_lpc[i + siglen - winlen] * hamwin[i];
    }

    g_lpc = LpcAnalysis(sig_lpc, siglen, xfft, lpcOrder);
    ckfree((char *) sig_lpc);

    for (i = 0; i <= lpcOrder; i++) {
      /* the factor is a guess, try looking for analytical value */
      xfft[i] = xfft[i] * 5000000000.0f;
    }
    for (i = lpcOrder + 1; i < fftlen; i++) {
      xfft[i] = 0.0;
    }
    
    Snack_DBPowerSpectrum(xfft);
    
    for (i = 0; i < fftlen/2; i++) {
      ffts[i] = -xfft[i];
    }

  } else {  /* usual FFT */

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

float  
LpcAnalysis (float *data, int N, float *f, int order) {
   int    i,m;
   float  sumU = 0.0;
   float  sumD = 0.0;
   float  b[SNACK_MAX_LPC_ORDER+1];
   float KK;

   float ParcorCoeffs[SNACK_MAX_LPC_ORDER];
   /* PreData should be used when signal is not windowed */
   float PreData[SNACK_MAX_LPC_ORDER];
   float *errF;
   float *errB;
   float errF_m = 0.0;

   if ((order < 1) || (order > SNACK_MAX_LPC_ORDER))  return 0.0f;

   errF = (float *) ckalloc((N+SNACK_MAX_LPC_ORDER) * sizeof(float));
   errB = (float *) ckalloc((N+SNACK_MAX_LPC_ORDER) * sizeof(float));

   for (i=0; i<order; i++) {
     ParcorCoeffs[i] = 0.0;
     PreData[i] = 0.0; /* delete here and use as argument when sig not windowed */
   };

   for (m=0; m<order; m++)
     errF[m] = PreData[m];
   for (m=0; m<N; m++)
     errF[m+order] = data[m] ;

   errB[0] = 0.0;
   for (m=1; m<N+order; m++)
     errB[m] = errF[m-1];

   for (i=0; i<order; i++) {

     sumU=0.0;
     sumD=0.0;
     for (m=i+1; m<N+order; m++) {
       sumU -= errF[m] * errB[m];
       sumD += errF[m] * errF[m] + errB[m] * errB[m];
     };

     if (sumD != 0) KK = 2.0f * sumU / sumD;   
     else KK = 0;
     ParcorCoeffs[i] = KK;


     for (m=N+order-1; m>i; m--) {
       errF[m] += KK * errB[m];
       errB[m] = errB[m-1] + KK * errF[m-1];
     };
   };

   for (i=order; i<N+order; i++) {
     errF_m += errF[i]*errF[i];
   }
   errF_m /= N;

   ckfree((char *)errF);
   ckfree((char *)errB);

   /* convert to direct filter coefficients */
   f[0] = 1.0;    
   for (m=1; m<=order; m++) {
     f[m] = ParcorCoeffs[m-1];
     for (i=1; i<m; i++)
       b[i] = f[i];
     for (i=1; i<m; i++)
       f[i] = b[i] + ParcorCoeffs[m-1] * b[m-i];
   }

   return (float)sqrt(errF_m);
}


void PreEmphase(float *sig, float presample, int len, float preemph) {
  int i;
  float temp;

  if (preemph == 0.0)  return;

  for (i = 0; i < len; i++) {
    temp = sig[i];
    sig[i] = temp - preemph * presample;
    presample = temp;
  }
}

#define SNACK_DEFAULT_POWERWINTYPE SNACK_WIN_HAMMING
#define DB 4.34294481903251830000000 /*  = 10 / ln(10)  */

int
powerCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  double dpreemph = 0.0, dscale = 1.0, dframelen = -1.0;
  float preemph = 0.0, scale = 1.0;
  int i, j, n = 0;
  int channel = 0, winlen = 256;
  int arg, startpos = 0, endpos = -1, framelen;
  float *powers = NULL;
  Tcl_Obj *list;
  SnackLinkedFileInfo info;
  SnackWindowType wintype = SNACK_DEFAULT_POWERWINTYPE;
  static char *subOptionStrings[] = {
    "-start", "-end", "-channel", "-framelength", "-windowlength",
    "-windowtype", "-preemphasisfactor", "-scale", "-progress", NULL
  };
  enum subOptions {
    START, END, CHANNEL, FRAMELEN, WINDOWLEN, WINTYPE, PREEMPH, SCALE,
    PROGRESS
  };

  if (s->debug > 0) { Snack_WriteLog("Enter powerCmd\n"); }

  if (s->cmdPtr != NULL) {
    Tcl_DecrRefCount(s->cmdPtr);
    s->cmdPtr = NULL;
  }

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
    case FRAMELEN:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &dframelen) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case WINDOWLEN:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &winlen) != TCL_OK)
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
    case PREEMPH:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &dpreemph) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case SCALE:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &dscale) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case PROGRESS:
      {
	char *str = Tcl_GetStringFromObj(objv[arg+1], NULL);
	
	if (strlen(str) > 0) {
	  Tcl_IncrRefCount(objv[arg+1]);
	  s->cmdPtr = objv[arg+1];
	}
	break;
      }
    }
  } 

  if (winlen < 1) {
    Tcl_AppendResult(interp, "-windowlength must be > 0", NULL);
    return TCL_ERROR;
  }
  if (winlen > NMAX) {
    char str[10];

    sprintf(str, "%d", NMAX);
    Tcl_AppendResult(interp, "-windowlength must be <= ", str, NULL);
    return TCL_ERROR;
  }
  
  preemph = (float) dpreemph;
  scale   = (float) dscale;
  
  if (startpos < 0) startpos = 0;
  if (endpos >= (s->length - 1) || endpos == -1)
    endpos = s->length - 1;
  if (startpos > endpos) return TCL_OK;

  if (s->storeType != SOUND_IN_MEMORY) {
    if (OpenLinkedFile(s, &info) != TCL_OK) {
      return TCL_OK;
    }
  }

  Snack_InitWindow(hamwin, winlen, winlen, wintype);

  if (dframelen == -1.0) {
    dframelen = 0.01;
  }
  framelen = (int) (dframelen * s->samprate);
  n = (endpos - startpos - winlen / 2) / framelen + 1;
  if (n < 1) {
    n = 1;
  }
  if (s->nchannels == 1) {
    channel = 0;
  }

  powers = (float *) ckalloc(sizeof(float) * n);

  Snack_ProgressCallback(s->cmdPtr, interp, "Computing power", 0.0);

  for (j = 0; j < n; j++) {
    float power;
    int winstart = 0;
    if (s->storeType == SOUND_IN_MEMORY) {
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * framelen - winlen/2) * s->nchannels + channel;

	if (p < 0) p = 0;
	if (p < winlen / 2) winstart = winlen / 2 - p;	
	for (i = winstart; i < winlen; i++) {
	  xfft[i] = (float) ((FSAMPLE(s, p + s->nchannels)
			      - preemph * FSAMPLE(s, p))
			     * hamwin[i]);
	  p += s->nchannels;
	}
      } else {
	int c;
	
	for (i = 0; i < winlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * framelen - winlen/2) * s->nchannels + c;
	  
	  if (p < 0) p = 0;
	  if (p < winlen / 2) winstart = winlen / 2 - p;	
	  for (i = winstart; i < winlen; i++) {
	    xfft[i] += (float) ((FSAMPLE(s, p + s->nchannels)
				 - preemph * FSAMPLE(s, p))
				* hamwin[i]);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < winlen; i++) {
	  xfft[i] /= s->nchannels;
	}
      }
    } else { /* storeType != SOUND_IN_MEMORY */
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * framelen - winlen/2) * s->nchannels + channel;
	
	if (p < 0) p = 0;
	if (p < winlen / 2) winstart = winlen / 2 - p;	
	for (i = winstart; i < winlen; i++) {
	  xfft[i] = (float) ((GetSample(&info, p + s->nchannels)
			      - preemph * GetSample(&info, p))
			     * hamwin[i]);
	  p += s->nchannels;
	}
      } else {
	int c;
	
	for (i = 0; i < winlen; i++) {
	  xfft[i] = 0.0;
	}
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * framelen - winlen/2) * s->nchannels + c;
	  
	  if (p < 0) p = 0;
	  if (p < winlen / 2) winstart = winlen / 2 - p;	
	  for (i = winstart; i < winlen; i++) {
	    xfft[i] += (float) ((GetSample(&info, p + s->nchannels)
				 - preemph * GetSample(&info, p))
				* hamwin[i]);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < winlen; i++) {
	  xfft[i] /= s->nchannels;
	}
      }
    }

    power = 0.0f;
    for (i = winstart; i < winlen; i++) {
      power += xfft[i] * xfft[i];
    }
    if (power < 1.0) power = 1.0;
    powers[j] = (float) (DB * log(scale * power / (float)(winlen - winstart)));

    if ((j % 10000) == 9999) {
      int res = Snack_ProgressCallback(s->cmdPtr, interp, "Computing power", 
				       (double) j / n);
      if (res != TCL_OK) {
	ckfree((char *) powers);
	return TCL_ERROR;
      }
    }
  }

  Snack_ProgressCallback(s->cmdPtr, interp, "Computing power", 1.0);

  list = Tcl_NewListObj(0, NULL);
  for (i = 0; i < n; i++) {
    Tcl_ListObjAppendElement(interp,list, Tcl_NewDoubleObj((double)powers[i]));
  }
  Tcl_SetObjResult(interp, list);

  if (s->storeType != SOUND_IN_MEMORY) {
    CloseLinkedFile(&info);
  }
  ckfree((char *) powers);

  if (s->debug > 0) { Snack_WriteLog("Exit powerCmd\n"); }

  return TCL_OK;
}

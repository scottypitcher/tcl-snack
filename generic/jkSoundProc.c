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


extern void Snack_PowerSpectrum(float *z);

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
  static CONST84 char *subOptionStrings[] = {
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
  static CONST84 char *subOptionStrings[] = {
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

float
mel(float f)
{
  return((float)(1127.0 * log(1.0f + f / 700.0f)));
}

int
speaturesCmd(Sound *s, Tcl_Interp *interp, int objc,
	     Tcl_Obj *CONST objv[])
{
  double tmpdouble = 0.0, dframelen = 0.01;
  float preemph = 0.97f, lowcut = 0.0f, highcut = (float) (s->samprate / 2.0);
  int i, j, k, n = 1, arg;
  int channel = 0;
  double dwinlen = 0.0256;
  int winlen;
  int fftlen = 2;
  int startpos = 0, endpos = -1, framelen;
  SnackLinkedFileInfo info;
  SnackWindowType wintype = SNACK_DEFAULT_DBPWINTYPE;
  static CONST84 char *subOptionStrings[] = {
    "-start", "-end", "-channel", "-framelength", "-windowlength",
    "-preemphasisfactor", "-windowtype",
    "-nchannels", "-ncoeff", "-cepstrallifter", "-energy", "-zeromean",
    "-zerothcepstralcoeff", "-lowcutoff", "-highcutoff", NULL
  };
  enum subOptions {
    START, END, CHANNEL, FRAMELEN, WINDOWLEN, PREEMPH, WINTYPE,
    NCHANNELS, NCOEFF, CEPLIFT, ENERGY, ZEROMEAN, ZEROTHCC,
    LOWCUT, HIGHCUT
  };
  float *outarr;
  int numchannels = 20, ncoeff = 12;
  int lowInd, highInd;
  float lowMel, highMel;
  float cf[40];
  float fb[40];
  float lifter[20];
  float mfcc[20];
  int   map[512];
  float fbw[512];
  float frame[1024];
  double ceplifter = 22.0;
  int doEnergy = 0, doZeroMean = 0, doZerothCC = 0;
  int vectorLength;
  Sound *outsnd;
  char *string;

  if (s->debug > 0) Snack_WriteLog("Enter speaturesCmd\n");

  string = Tcl_GetStringFromObj(objv[2], NULL);

  if ((outsnd = Snack_GetSound(interp, string)) == NULL) {
    return TCL_ERROR;
  }

  for (arg = 3; arg < objc; arg += 2) {
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
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &dwinlen) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case PREEMPH:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	preemph = (float) tmpdouble;
	break;
      }
    case WINTYPE:
      {
	char *str = Tcl_GetStringFromObj(objv[arg+1], NULL);
	if (GetWindowType(interp, str, &wintype) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case NCHANNELS:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &numchannels) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case NCOEFF:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &ncoeff) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case CEPLIFT:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &ceplifter) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case ENERGY:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &doEnergy) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case ZEROMEAN:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &doZeroMean) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case ZEROTHCC:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &doZerothCC) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    case LOWCUT:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	lowcut = (float) tmpdouble;
	break;
      }
    case HIGHCUT:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	highcut = (float) tmpdouble;
	break;
      }
    }
  }

  winlen = (int) (s->samprate * dwinlen);
  
  if (startpos < 0 || startpos > s->length - fftlen) {
    Tcl_AppendResult(interp, "FFT window out of bounds", NULL);
    return TCL_ERROR;
  }

  if (startpos < 0) startpos = 0;
  if (endpos >= (s->length - 1) || endpos == -1)
    endpos = s->length - 1;
  if (startpos > endpos) return TCL_OK;

  if (s->storeType != SOUND_IN_MEMORY) {
    if (OpenLinkedFile(s, &info) != TCL_OK) {
      return TCL_OK;
    }
  }

  while (fftlen < winlen) fftlen *= 2;

  framelen = (int) (dframelen * s->samprate);
  n = (endpos - startpos - winlen) / framelen + 1;
  if (n < 1) {
    n = 1;
  }

  vectorLength  = ncoeff;
  if (doEnergy) vectorLength++;
  if (doZerothCC) vectorLength++;

  outarr = (float *) ckalloc(sizeof(float) * vectorLength * n);

  lowMel = (float) mel(lowcut);
  lowInd = (int) ((lowcut * fftlen / s->samprate) + 1.5);
  if (lowInd < 1) lowInd = 1;
  highMel = (float) mel(highcut);
  highInd = (int) ((highcut * fftlen / s->samprate) - 0.5);
  if (highInd > fftlen / 2 - 1) highInd = fftlen / 2 - 1;

  for (i = 0; i < numchannels + 1; i++) {
    cf[i] = ((float) (i+1) / (numchannels+1)) * (highMel - lowMel) + lowMel;
  }

  for (i = 0; i < fftlen / 2; i++) {
    float melInd = mel((float) i * s->samprate / fftlen);
    float new;
    int ch = 0;

    if (i < lowInd || i > highInd) {
      map[i] = -1;
      new = 0.0;
    } else {
      while (cf[ch] < melInd && ch < numchannels + 1) ++ch;
      map[i] = ch - 1;
      if (ch - 1 >= 0) {
        new = ((cf[ch] - mel((float) i*s->samprate/fftlen)) / 
	       (cf[ch] - cf[ch-1]));
      } else {
	new = (cf[0] - mel((float) i*s->samprate/fftlen)) / (cf[0] - lowMel);
      }
    }
    fbw[i] = new;
  }

  for (i = 0; i <= ncoeff; i++) {
    lifter[i] = (float)(1.0 + ceplifter /2.0 * sin(SNACK_PI * i / ceplifter));
  }

  Snack_InitWindow(hamwin, winlen, fftlen, wintype);

  Snack_InitFFT(fftlen);

  for (j = 0; j < n; j++) {
    float sum = 0.0;

    for (i = 0; i < fftlen; i++) {
      frame[i] = 0.0f;
    }
    if (s->storeType == SOUND_IN_MEMORY) {
      if (s->nchannels == 1 || channel != -1) {
	  int p = (startpos + j * framelen) * s->nchannels + channel;

	  for (i = 0; i < winlen; i++) {
	    frame[i] = FSAMPLE(s, p);
	    p += s->nchannels;
	  }
      } else {
	int c;
	
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * framelen) * s->nchannels + c;

	  for (i = 0; i < winlen; i++) {
	    frame[i] += FSAMPLE(s, p);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < winlen; i++) {
	  frame[i] /= s->nchannels;
	}
      }
    } else { /* storeType != SOUND_IN_MEMORY */
      if (s->nchannels == 1 || channel != -1) {
	int p = (startpos + j * framelen) * s->nchannels + channel;

	for (i = 0; i < winlen; i++) {
	  frame[i] = GetSample(&info, p);
	  p += s->nchannels;
	}
      } else {
	int c;
	
	for (c = 0; c < s->nchannels; c++) {
	  int p = (startpos + j * framelen) * s->nchannels + c;
	  
	  for (i = 0; i < winlen; i++) {
	    frame[i] += GetSample(&info, p);
	    p += s->nchannels;
	  }
	}
	for (i = 0; i < winlen; i++) {
	  frame[i] /= s->nchannels;
	}
      }
    }

    for (i = 0; i < winlen; i++) {
      float sample = frame[i];
      sum += sample * sample;
    }
    if (sum < 1.0) sum = 1.0f;

    xfft[0] = (float) (1.0f - preemph) * frame[0] * hamwin[0];
    for (i = 1; i < fftlen; i++) {
      xfft[i] = (float) ((frame[i] - preemph * frame[i - 1]) * hamwin[i]);
    }
   
    Snack_PowerSpectrum(xfft);

    for (i = 0; i < numchannels + 1; i++) {
      fb[i] = 0.0;
    }
    for (i = lowInd; i <= highInd; i++) {
      float e = (float) (0.5 * sqrt(xfft[i]));
      int bin = map[i];
      float we = fbw[i] * e;

      if (bin > -1) fb[bin] += we;
      if (bin < numchannels - 1) fb[bin + 1] += e - we;
    }
    for (i = 0; i < numchannels; i++) {
      if (fb[i] < 1.0) {
	fb[i] = 0.0f;
      } else {
	fb[i] = (float) log(fb[i]);
      }
    }

    for (i = 0; i < ncoeff; i++) {
      mfcc[i] = 0.0f;
      for (k = 0; k < numchannels; k++) {
	mfcc[i] += (float) (fb[k] * cos(SNACK_PI * (i + 1) / numchannels *
					(k + 0.5f)));
      }
      mfcc[i] *= (float) (sqrt(2.0f / numchannels) * lifter[i + 1]);
    }
    mfcc[ncoeff] = (float) log(sum);
    for (i = 0; i < ncoeff + 1; i++) {
      outarr[j * vectorLength + i] = mfcc[i];
    }
    if (doZerothCC) {
      float sum = 0.0f;
      
      for (i = 0; i < numchannels; i++) {
	sum += fb[i];
      }
      outarr[j*vectorLength+ncoeff] = (float) (sum * sqrt(2.0 / numchannels));
    }
  } /* for (j = 0;... */

  if (doZeroMean) {
    for (i = 0; i < ncoeff; i++) {
      float sum = 0.0f, offset;

      for (j = 0; j < n; j++) {
	sum += outarr[j * vectorLength + i];
      }
      offset = sum / n;
      for (j = 0; j < n; j++) {
	outarr[j * vectorLength + i] -= offset;
      }
    }
  }
  if (doEnergy) {
    float max = -1000000.0f, min;

    for (i = ncoeff; i < n * vectorLength; i += vectorLength) {
      if (outarr[i] > max) max = outarr[i];
    }
    min = (float) (max - 50.0f * log(10.0f) / 10.0f);
    for (i = ncoeff; i < n * vectorLength; i += vectorLength) {
      if (outarr[i] < min) outarr[i] = min;
      outarr[i] = 1.0f - 0.1f * (max - outarr[i]);
    }
  }
  
  if (s->storeType != SOUND_IN_MEMORY) {
    CloseLinkedFile(&info);
  }

  outsnd->nchannels = vectorLength;
  if (Snack_ResizeSoundStorage(outsnd, n) != TCL_OK) {
    return TCL_ERROR;
  }
  outsnd->length = n;
  
  for (i = 0; i < n; i++) {
    for (k = 0; k < vectorLength; k++) {
      FSAMPLE(outsnd, i*vectorLength+k) = outarr[i*vectorLength+k];
    }
  }

  if (s->debug > 0) Snack_WriteLog("Exit speaturesCmd\n");

  return TCL_OK;
}

extern int cPitch(Sound *s, Tcl_Interp *interp, int **outlist, int *length);

static int
searchZX(Sound *s, int pos)
{
  int i, j;

  for(i = 0; i < 20000; i++) {
    j = pos + i;
    if (j < s->length && FSAMPLE(s, j-1) < 0.0 && FSAMPLE(s, j) >= 0.0) {
      return(j);
    }
    j = pos - i;
    if (j > 0 && FSAMPLE(s, j-1) < 0.0 && FSAMPLE(s, j) >= 0.0) {
      return(j);
    }
  }
  return(-1);
}

static void
copy(Sound *s, float *buf, int buflen, int pos, int index, int length)
{
  int i, ii;
  float z, win, newsmp;
  
  index = index - length/2;
  pos = pos - length/2;
  length = 2 * length;
  
  for (i = 0; i < length; i++) {
    if (i >= length / 2) {
      ii = length - 1 - i;
    } else {
      ii = i;
    }
    z = (float) ((0.5 + length / 2 - 1 - ii) * SNACK_PI / length);
    win = (float) exp(-3.0 * z * z);
    if (pos < 0 || pos >= buflen || index < 0 || index >= s->length) return;
    newsmp = buf[pos] + win * FSAMPLE(s, index);
    if (newsmp >= 32767.0) {
      newsmp = 32767.0;
    }
    if (newsmp <= -32768.0) {
      newsmp = -32768.0;
    }
    buf[pos] = newsmp;
    index++;
    pos++;
  }
}

int
xoCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  double tmpdouble = 0.0;
  float px = 1.0f, rx = 1.0f, tx = 1.0f;
  int i, ind, last, start, inpos, outpos, newlen, arg, segOnly = 0;
  static CONST84 char *subOptionStrings[] = {
    "-p", "-r", "-t", "-s", NULL
  };
  enum subOptions {
    P, R, T, S
  };
  int *pitchList;
  int *segs;
  int *sege;
  int length = 0;
  float *buf;
  int skip = s->samprate / 100;

  if (s->debug > 0) Snack_WriteLog("Enter xoCmd\n");

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
    case P:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	px = (float) tmpdouble;
	break;
      }
    case R:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	rx = (float) tmpdouble;
	break;
      }
    case T:
      {
	if (Tcl_GetDoubleFromObj(interp, objv[arg+1], &tmpdouble) != TCL_OK)
	  return TCL_ERROR;
	tx = (float) tmpdouble;
	break;
      }
    case S:
      {
	if (Tcl_GetIntFromObj(interp, objv[arg+1], &segOnly) != TCL_OK)
	  return TCL_ERROR;
	break;
      }
    }
  }

  cPitch(s, interp, &pitchList, &length);

  ind = 0;
  start = 0;
  last = 0;
  segs = (int *) ckalloc(sizeof(int) * length);
  sege = (int *) ckalloc(sizeof(int) * length);
  for (i = 1; i < s->length; i++) {
    int point;

    if ((int)(i/(float)skip+.5) >= length) break;

    point = pitchList[(int)(i/(float)skip+.5)];
    if (point == 0.0) {
      i += 9;
      continue;
    }
    if (start == 0) {
      i = searchZX(s, i);
      segs[ind] = start;
      sege[ind] = i;
      start = i;
      ind++;
    } else {
      i = searchZX(s, (int)(i+s->samprate/point));
      if (i == last) {
	int j = i + 10;
	while (last == i) {
	  i = searchZX(s, j);
	  j += 10;
	}
      }
      last = i;
      if (i > 0) {
	segs[ind] = start;
	sege[ind] = i;
	start = i;
	ind++;
      } else {
	segs[ind] = start;
	sege[ind] = s->length;
	start = s->length;
	ind++;
	break;
      }
    }
  }
  newlen = (int)(rx * s->length);
  buf = (float *) ckalloc(sizeof(float) * newlen);
  for (i = 0; i < newlen; i++) {
    buf[i] = 0.0f;
  }
  inpos = 0;
  outpos = 0;
  
  while (inpos < s->length) {
    int frame = inpos / skip;
    int point = pitchList[frame];
  
    if (point == 0) {
      for (i = 1; i < 5; i++) {
	frame++;
	if (pitchList[frame] != 0) break;
      }
      
      copy(s, buf, newlen, outpos, inpos, skip * i);
   
      inpos  += (int) (skip * i / rx);
      outpos += (skip * i);
      
    } else {
   
      int index = -1;

      for (i = 0; i < ind; i++) {
	if (segs[i] <= inpos && inpos < sege[i] && (sege[i]-segs[i]) < 200) {
	  index = i;
	  break;
	}
      }

      if (index > 0) {
	copy(s, buf, newlen, outpos, segs[index], sege[index] - segs[index]);
	inpos  += (int) (px * (sege[index] - segs[index]) / rx);
	outpos += (int) (px * (sege[index] - segs[index]));
      } else {
	inpos  += (int) (skip / rx);
	outpos += skip;
      }
    }
  }

  for (i = 0; i < s->length; i++) {
    FSAMPLE(s, i) = 0.0f;
  }
  Snack_ResizeSoundStorage(s, newlen);
  s->length = newlen;
  Snack_PutSoundData(s, 0, buf, newlen);
  ckfree((char *)segs);
  ckfree((char *)sege);
  ckfree((char *)buf);
  ckfree((char *)pitchList);

  if (s->debug > 0) Snack_WriteLog("Exit xoCmd\n");

  return TCL_OK;
}

int
ocCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  return TCL_OK;
}

int
alCmd(Sound *s, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  return TCL_OK;
}

int
Snack_HSetCmd(ClientData cdata, Tcl_Interp *interp, int objc,
		Tcl_Obj *CONST objv[])
{

  return TCL_OK;
}

void
Snack_HSetDeleteCmd(ClientData clientData)
{
}

Tcl_HashTable *hsetHashTable;

int
Snack_arCmd(ClientData cdata, Tcl_Interp *interp, int objc,
		Tcl_Obj *CONST objv[])
{
  return TCL_OK;
}

void
Snack_arDeleteCmd(ClientData clientData)
{
}

Tcl_HashTable *arHashTable;

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
#include "jkAudIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define USE_OLD_CANVAS /* To keep Tk8.3 happy */
#include "tk.h"
#include "jkSound.h"
#include "jkCanvItems.h"
#include <string.h>

#define SNACK_DEFAULT_SECTWINTYPE      SNACK_WIN_HAMMING
#define SNACK_DEFAULT_SECTWINTYPE_NAME "hamming"

/*
 * Section item structure
 */

typedef struct SectionItem  {

  Tk_Item header;
  Tk_Canvas canvas;
  double x, y;
  Tk_Anchor anchor;
  int nPoints;
  double *coords;
  XColor *fg;
  Pixmap fillStipple;
  GC gc;
  char *newSoundName;
  char *soundName;
  Sound *sound;
  SnackItemInfo si;
  float *xfft;
  double *ffts;
  int height;
  int width;
  int startSmp;
  int endSmp;
  int ssmp;
  int esmp;
  int frame;
  int id;
  XPoint fpts[5];
  char *channelstr;
  int debug;
  double topFrequency;
  double maxValue;
  double minValue;
  char *windowTypeStr;
  Tcl_Interp *interp;

} SectionItem;

Tk_CustomOption sectTagsOption = { (Tk_OptionParseProc *) NULL,
				   (Tk_OptionPrintProc *) NULL,
				   (ClientData) NULL };

typedef enum {
  OPTION_ANCHOR,
  OPTION_TAGS,
  OPTION_SOUND,
  OPTION_HEIGHT,
  OPTION_WIDTH,
  OPTION_FFTLEN,
  OPTION_WINLEN,
  OPTION_PREEMP,
  OPTION_START,
  OPTION_END,
  OPTION_FILL,
  OPTION_STIPPLE,
  OPTION_FRAME,
  OPTION_TOPFREQUENCY,
  OPTION_CHANNEL,
  OPTION_MAXVAL,
  OPTION_MINVAL,
  OPTION_SKIP,
  OPTION_WINTYPE
} ConfigSpec;

static Tk_ConfigSpec configSpecs[] = {

  {TK_CONFIG_ANCHOR, "-anchor", (char *) NULL, (char *) NULL,
   "nw", Tk_Offset(SectionItem, anchor), TK_CONFIG_DONT_SET_DEFAULT},

  {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
   (char *) NULL, 0, TK_CONFIG_NULL_OK, &sectTagsOption},
  
  {TK_CONFIG_STRING, "-sound", (char *) NULL, (char *) NULL,
   "", Tk_Offset(SectionItem, newSoundName), TK_CONFIG_NULL_OK},
  
  {TK_CONFIG_INT, "-height", (char *) NULL, (char *) NULL,
   "256", Tk_Offset(SectionItem, height), 0},
  
  {TK_CONFIG_PIXELS, "-width", (char *) NULL, (char *) NULL,
   "256", Tk_Offset(SectionItem, width), 0},
  
  {TK_CONFIG_INT, "-fftlength", (char *) NULL, (char *) NULL,
   "512", Tk_Offset(SectionItem, si.fftlen), 0},
  
  {TK_CONFIG_INT, "-winlength", (char *) NULL, (char *) NULL,
   "256", Tk_Offset(SectionItem, si.winlen), 0},
  
  {TK_CONFIG_DOUBLE, "-preemphasisfactor", (char *) NULL, (char *) NULL,
   "0.0", Tk_Offset(SectionItem, si.preemph), 0},
  
  {TK_CONFIG_INT, "-start", (char *) NULL, (char *) NULL,
   "0", Tk_Offset(SectionItem, startSmp), 0},
  
  {TK_CONFIG_INT, "-end", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(SectionItem, endSmp), 0},
  
  {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
   "black", Tk_Offset(SectionItem, fg), TK_CONFIG_NULL_OK},
  
  {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
   (char *) NULL, Tk_Offset(SectionItem, fillStipple), TK_CONFIG_NULL_OK},

  {TK_CONFIG_BOOLEAN, "-frame", (char *) NULL, (char *) NULL,
   "no", Tk_Offset(SectionItem, frame), TK_CONFIG_NULL_OK},

  {TK_CONFIG_DOUBLE, "-topfrequency", (char *) NULL, (char *) NULL,
   "0.0", Tk_Offset(SectionItem, topFrequency), 0},
  
  {TK_CONFIG_STRING, "-channel", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(SectionItem, channelstr), TK_CONFIG_NULL_OK},
  
  {TK_CONFIG_DOUBLE, "-maxvalue", (char *) NULL, (char *) NULL,
   "0.0", Tk_Offset(SectionItem, maxValue), 0},

  {TK_CONFIG_DOUBLE, "-minvalue", (char *) NULL, (char *) NULL,
   "-80.0", Tk_Offset(SectionItem, minValue), 0},

  {TK_CONFIG_INT, "-skip", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(SectionItem, si.skip), 0},

  {TK_CONFIG_STRING, "-windowtype", (char *) NULL, (char *) NULL,
   SNACK_DEFAULT_SECTWINTYPE_NAME, Tk_Offset(SectionItem, windowTypeStr), 0},

  {TK_CONFIG_INT, "-debug", (char *) NULL, (char *) NULL,
   "0", Tk_Offset(SectionItem, debug), 0},

  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
   (char *) NULL, 0, 0}

};

/*
 * Protos
 */

static void   ComputeSectionBbox(Tk_Canvas canvas, SectionItem *sectPtr);

static int    ComputeSectionCoords(Tk_Item *itemPtr);

static int    ConfigureSection(Tcl_Interp *interp, Tk_Canvas canvas,
			       Tk_Item *itemPtr, int argc,
			       char **argv, int flags);

static int    CreateSection(Tcl_Interp *interp,	Tk_Canvas canvas,
			    struct Tk_Item *itemPtr, int argc, char **argv);

static void   DeleteSection(Tk_Canvas canvas, Tk_Item *itemPtr,
			    Display *display);

static void   DisplaySection(Tk_Canvas canvas, Tk_Item *itemPtr,
			     Display *display, Drawable dst,
			     int x, int y, int width, int height);

static void   ScaleSection(Tk_Canvas canvas, Tk_Item *itemPtr,
			   double originX, double originY,
			   double scaleX, double scaleY);

static int    SectionCoords(Tcl_Interp *interp,	Tk_Canvas canvas,
			    Tk_Item *itemPtr, int argc, char **argv);

static int    SectionToArea(Tk_Canvas canvas, Tk_Item *itemPtr,
			    double *rectPtr);

static double SectionToPoint(Tk_Canvas canvas, Tk_Item *itemPtr,
			     double *coords);

static int    SectionToPS(Tcl_Interp *interp, Tk_Canvas canvas,
			  Tk_Item *itemPtr, int prepass);

static void   TranslateSection(Tk_Canvas canvas, Tk_Item *itemPtr,
			       double deltaX, double deltaY);

/*
 * Section item type
 */

Tk_ItemType snackSectionType = {
  "section",
  sizeof(SectionItem),
  CreateSection,
  configSpecs,
  ConfigureSection,
  SectionCoords,
  DeleteSection,
  DisplaySection,
  0,
  SectionToPoint,
  SectionToArea,
  SectionToPS,
  ScaleSection,
  TranslateSection,
  (Tk_ItemIndexProc *) NULL,
  (Tk_ItemCursorProc *) NULL,
  (Tk_ItemSelectionProc *) NULL,
  (Tk_ItemInsertProc *) NULL,
  (Tk_ItemDCharsProc *) NULL,
  (Tk_ItemType *) NULL
};

static int
CreateSection(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr,
	   int argc, char **argv)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;

  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     Tk_PathName(Tk_CanvasTkwin(canvas)), " create ",
		     itemPtr->typePtr->name, " x y ?opts?\"", (char *) NULL);
    return TCL_ERROR;
  }

  sectPtr->canvas = canvas;
  sectPtr->anchor = TK_ANCHOR_NW;
  sectPtr->nPoints = 0;
  sectPtr->coords = NULL;
  sectPtr->fg = None;
  sectPtr->fillStipple = None;
  sectPtr->gc = None;
  sectPtr->newSoundName = NULL;
  sectPtr->soundName = NULL;
  sectPtr->sound = NULL;
  sectPtr->si.sampfreq = 16000;
  sectPtr->si.BufPos = 0;
  sectPtr->si.fftlen = 512;
  sectPtr->si.winlen = 256;
  sectPtr->si.preemph = 0.0;
  sectPtr->si.fftmax = -10000;
  sectPtr->si.fftmin = 10000;
  sectPtr->si.hamwin = (float *) ckalloc(NMAX * sizeof(float));
  sectPtr->si.abmax = 0;
  sectPtr->xfft = (float *)  ckalloc(NMAX * sizeof(float));
  sectPtr->ffts = (double *) ckalloc(NMAX / 2 * sizeof(double));
  sectPtr->height = 256;
  sectPtr->width = 256;
  sectPtr->startSmp = 0;
  sectPtr->endSmp = -1;
  sectPtr->ssmp = 0;
  sectPtr->esmp = -1;
  sectPtr->id = 0;
  sectPtr->frame = 0;
  sectPtr->debug = 0;
  sectPtr->x = 0;
  sectPtr->y = 0;
  sectPtr->topFrequency = 0.0;
  sectPtr->channelstr = NULL;
  sectPtr->si.channel = -1;
  sectPtr->si.channelSet = -1;
  sectPtr->si.nchannels = 1;
  sectPtr->maxValue = 0.0;
  sectPtr->minValue = -80.0;
  sectPtr->si.validStart = 0;
  sectPtr->si.skip = -1;
  sectPtr->si.windowType = SNACK_DEFAULT_SECTWINTYPE;
  sectPtr->si.windowTypeSet = SNACK_DEFAULT_SECTWINTYPE;
  sectPtr->windowTypeStr = NULL;
  sectPtr->interp = interp;

  if (sectPtr->si.hamwin == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate analysis window buffer!",NULL);
    return TCL_ERROR;
  }

  if (sectPtr->xfft == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate fft buffer!", NULL);
    ckfree((char *)sectPtr->si.hamwin);
    return TCL_ERROR;
  }

  if (sectPtr->ffts == NULL) {
    Tcl_AppendResult(interp, "Couldn't allocate fft buffer!", NULL);
    ckfree((char *)sectPtr->si.hamwin);
    ckfree((char *)sectPtr->xfft);
    return TCL_ERROR;
  }


  if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &sectPtr->x) != TCL_OK) ||
      (Tk_CanvasGetCoord(interp, canvas, argv[1], &sectPtr->y) != TCL_OK))
    return TCL_ERROR;
  
  if (ConfigureSection(interp, canvas, itemPtr, argc-2, argv+2, 0) == TCL_OK)
    return TCL_OK;

  DeleteSection(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
  return TCL_ERROR;
}

static int
SectionCoords(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr,
	      int argc, char **argv)
{
  SectionItem *wPtr = (SectionItem *) itemPtr;
  char xc[TCL_DOUBLE_SPACE], yc[TCL_DOUBLE_SPACE];

  if (argc == 0) {
    Tcl_PrintDouble(interp, wPtr->x, xc);
    Tcl_PrintDouble(interp, wPtr->y, yc);
    Tcl_AppendResult(interp, xc, " ", yc, (char *) NULL);
  } else if (argc == 2) {
    if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &wPtr->x) != TCL_OK) ||
	(Tk_CanvasGetCoord(interp, canvas, argv[1], &wPtr->y) != TCL_OK)) {
      return TCL_ERROR;
    }
    ComputeSectionBbox(canvas, wPtr);
  } else {
    char buf[80];

    sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", argc);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return TCL_ERROR;
  }
  return TCL_OK;
}

static int
ComputeSectionCoords(Tk_Item *itemPtr)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  int i;
  int nPoints = sectPtr->nPoints;
  float xscale = (float) (sectPtr->width) / nPoints;
  float yscale = (float) ((float) (sectPtr->height - 1) /
    (sectPtr->minValue - sectPtr->maxValue));
  float fscale = (float) (sectPtr->si.topfrequency / (sectPtr->si.sampfreq / 2.0));

  if (sectPtr->debug) Snack_WriteLogInt("Enter ComputeSectionCoords", nPoints);

  if (sectPtr->coords != NULL) ckfree((char *) sectPtr->coords);
  sectPtr->coords = (double *) ckalloc((unsigned)
			       (sizeof(double) * (2 * nPoints)));

  for (i = 0; i < nPoints; i++) {
    double t = (double) (sectPtr->ffts[(int)((float)i*fscale)] -
			 sectPtr->maxValue) * yscale;
    if (t > sectPtr->height-1) t = (double) (sectPtr->height-1);
    if (t < 0.0) t = 0.0;
    sectPtr->coords[i*2]   = (double) i * xscale;
    sectPtr->coords[i*2+1] = t;
  }

  ComputeSectionBbox(sectPtr->canvas, sectPtr);

  if (sectPtr->debug) Snack_WriteLog("Exit ComputeSectionCoords\n"); 

  return TCL_OK;
}

void
ComputeSection(Tk_Item *itemPtr)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  SnackItemInfo *siPtr = &sectPtr->si;
  int i, j;
  int fftlen     = siPtr->fftlen;
  double preemph = siPtr->preemph;
  int RestartPos = siPtr->RestartPos - siPtr->validStart;
  int sampformat = siPtr->sampformat;
  int storeType  = siPtr->storeType;
  int n, skip = siPtr->skip;
  double Max = -1000.0, Min = 1000.0;
  SnackLinkedFileInfo info;

  if (sectPtr->debug) Snack_WriteLogInt("Enter ComputeSection", sectPtr->ssmp); 
  if (skip < 1) {
    skip = fftlen;
  }
  n = (sectPtr->esmp - siPtr->RestartPos) / skip;

  if (n == 0) return;

  for (i = 0; i < fftlen/2; i++) {
    sectPtr->ffts[i] = 0.0;
  }

  Snack_InitFFT(siPtr->fftlen);

  if (storeType != SOUND_IN_MEMORY) {
    if (OpenLinkedFile(sectPtr->sound, &info) != TCL_OK) {
      return;
    }
  }

  for (j = 0; j < n; j++) {
    if (storeType == SOUND_IN_MEMORY) {
      if (siPtr->nchannels == 1 || siPtr->channel != -1) {
	int p = (RestartPos + j * skip) * siPtr->nchannels + siPtr->channel;
	if (sampformat == LIN16) {
	  for (i = 0; i < fftlen; i++) {
	    sectPtr->xfft[i] = (float) ((SSAMPLE(siPtr, p + siPtr->nchannels)
					 - preemph * SSAMPLE(siPtr, p))
					* siPtr->hamwin[i]);
	    p += siPtr->nchannels;
	  }
	} else {
	  for (i = 0; i < fftlen; i++) {
	    if (sampformat == MULAW) {
	      sectPtr->xfft[i] = (float) ((Snack_Mulaw2Lin(UCSAMPLE(siPtr, p + siPtr->nchannels)) - preemph * Snack_Mulaw2Lin(UCSAMPLE(siPtr, p))) * siPtr->hamwin[i]);
	    } else if (sampformat == ALAW) {
	      sectPtr->xfft[i] = (float) ((Snack_Alaw2Lin(UCSAMPLE(siPtr, p + siPtr->nchannels)) - preemph * Snack_Alaw2Lin(UCSAMPLE(siPtr, p))) * siPtr->hamwin[i]);
	    } else if (sampformat == LIN8OFFSET) {
	      sectPtr->xfft[i] = (float) ((UCSAMPLE(siPtr, p + siPtr->nchannels) - preemph * UCSAMPLE(siPtr, p)) * siPtr->hamwin[i]);
	    } else if (sampformat == LIN8) {
	      sectPtr->xfft[i] = (float) ((CSAMPLE(siPtr, p + siPtr->nchannels) - preemph * CSAMPLE(siPtr, p)) * siPtr->hamwin[i]);
	    }
	    p += siPtr->nchannels;
	  }
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  sectPtr->xfft[i] = 0.0;
	}
	for (c = 0; c < siPtr->nchannels; c++) {
	  int p = (RestartPos + j * skip) * siPtr->nchannels + c;
	  
	  if (sampformat == LIN16) {
	    for (i = 0; i < fftlen; i++) {
	      sectPtr->xfft[i] += (float) ((SSAMPLE(siPtr, p + siPtr->nchannels)
					    - preemph * SSAMPLE(siPtr, p))
					   * siPtr->hamwin[i]);
	      p += siPtr->nchannels;
	    }
	  } else {
	    for (i = 0; i < fftlen; i++) {
	      if (sampformat == MULAW) {
		sectPtr->xfft[i] += (float) ((Snack_Mulaw2Lin(UCSAMPLE(siPtr, p + siPtr->nchannels)) - preemph * Snack_Mulaw2Lin(UCSAMPLE(siPtr, p))) * siPtr->hamwin[i]);
	      } else if (sampformat == ALAW) {
		sectPtr->xfft[i] += (float) ((Snack_Alaw2Lin(UCSAMPLE(siPtr, p + siPtr->nchannels)) - preemph * Snack_Alaw2Lin(UCSAMPLE(siPtr, p))) * siPtr->hamwin[i]);
	      } else if (sampformat == LIN8OFFSET) {
		sectPtr->xfft[i] += (float) ((UCSAMPLE(siPtr, p + siPtr->nchannels) - preemph * UCSAMPLE(siPtr, p)) * siPtr->hamwin[i]);
	      } else if (sampformat == LIN8) {
		sectPtr->xfft[i] += (float) ((CSAMPLE(siPtr, p + siPtr->nchannels) - preemph * CSAMPLE(siPtr, p)) * siPtr->hamwin[i]);
	      }
	      p += siPtr->nchannels;
	    }
	  }
	}
	for (i = 0; i < fftlen; i++) {
	  sectPtr->xfft[i] /= siPtr->nchannels;
	}
      }
    } else { /* storeType != SOUND_IN_MEMORY */
      if (siPtr->nchannels == 1 || siPtr->channel != -1) {
	int p = (RestartPos + j * skip) * siPtr->nchannels + siPtr->channel;
	if (sampformat == LIN16) {
	  for (i = 0; i < fftlen; i++) {
	    sectPtr->xfft[i] = (float) ((GetSample(&info, p + siPtr->nchannels)
					 - preemph * GetSample(&info, p))
					* siPtr->hamwin[i]);
	    p += siPtr->nchannels;
	  }
	} else {
	  for (i = 0; i < fftlen; i++) {
	    if (sampformat == MULAW) {
	      sectPtr->xfft[i] = (float) ((Snack_Mulaw2Lin((unsigned char)GetSample(&info, p + siPtr->nchannels)) - preemph * Snack_Mulaw2Lin((unsigned char)GetSample(&info, p))) * siPtr->hamwin[i]);
	    } else if (sampformat == ALAW) {
	      sectPtr->xfft[i] = (float) ((Snack_Alaw2Lin((unsigned char)GetSample(&info, p + siPtr->nchannels)) - preemph * Snack_Alaw2Lin((unsigned char)GetSample(&info, p))) * siPtr->hamwin[i]);
	    } else {
	      sectPtr->xfft[i] = (float) ((GetSample(&info, p + siPtr->nchannels) - preemph * GetSample(&info, p)) * siPtr->hamwin[i]);
	    }
	    p += siPtr->nchannels;
	  }
	}
      } else {
	int c;
	
	for (i = 0; i < fftlen; i++) {
	  sectPtr->xfft[i] = 0.0;
	}
	for (c = 0; c < siPtr->nchannels; c++) {
	  int p = (RestartPos + j * skip) * siPtr->nchannels + c;
	  
	  if (sampformat == LIN16) {
	    for (i = 0; i < fftlen; i++) {
	      sectPtr->xfft[i] += (float) ((GetSample(&info, p + siPtr->nchannels)
					    - preemph * GetSample(&info, p))
					   * siPtr->hamwin[i]);
	      p += siPtr->nchannels;
	    }
	  } else {
	    for (i = 0; i < fftlen; i++) {
	      if (sampformat == MULAW) {
		sectPtr->xfft[i] += (float) ((Snack_Mulaw2Lin((unsigned char)GetSample(&info, p + siPtr->nchannels)) - preemph * Snack_Mulaw2Lin((unsigned char)GetSample(&info, p))) * siPtr->hamwin[i]);
	      } else if (sampformat == ALAW) {
		sectPtr->xfft[i] += (float) ((Snack_Alaw2Lin((unsigned char)GetSample(&info, p + siPtr->nchannels)) - preemph * Snack_Alaw2Lin((unsigned char)GetSample(&info, p))) * siPtr->hamwin[i]);
	      } else {
		sectPtr->xfft[i] += (float) ((GetSample(&info, p + siPtr->nchannels) - preemph * GetSample(&info, p)) * siPtr->hamwin[i]);
	      }
	      p += siPtr->nchannels;
	    }
	  }
	}
	for (i = 0; i < fftlen; i++) {
	  sectPtr->xfft[i] /= siPtr->nchannels;
	}
      }
    }

    Snack_DBPowerSpectrum(sectPtr->xfft);

    for (i = 0; i < fftlen/2; i++) {
      sectPtr->ffts[i] += sectPtr->xfft[i];
    }
  }

  for (i = 0; i < fftlen/2; i++) {
    sectPtr->ffts[i] = sectPtr->ffts[i] / (float) n;
    if (sectPtr->ffts[i] > Max) Max = sectPtr->ffts[i];
    if (sectPtr->ffts[i] < Min) Min = sectPtr->ffts[i];
  }  

  if (storeType != SOUND_IN_MEMORY) {
    CloseLinkedFile(&info);
  }

  if (sectPtr->debug) Snack_WriteLog("Exit ComputeSection"); 
}

static void
UpdateSection(ClientData clientData, int flag)
{
  SectionItem *sectPtr = (SectionItem *) clientData;
  Sound *s = sectPtr->sound;

  if (sectPtr->debug) Snack_WriteLogInt("Enter UpdateSection", flag);

  if (sectPtr->canvas == NULL) return;

  Tk_CanvasEventuallyRedraw(sectPtr->canvas,
			    sectPtr->header.x1, sectPtr->header.y1,
			    sectPtr->header.x2, sectPtr->header.y2);

  sectPtr->si.blocks = s->blocks;
  sectPtr->si.BufPos = s->length;
  sectPtr->si.storeType = s->storeType;
  sectPtr->si.sampfreq = s->sampfreq;
  sectPtr->si.sampformat = s->sampformat;
  sectPtr->si.nchannels = s->nchannels;
  
  if (flag == SNACK_MORE_SOUND) {
    sectPtr->esmp = sectPtr->si.BufPos - 1;
    sectPtr->ssmp = sectPtr->esmp - sectPtr->si.fftlen;
    
    if (sectPtr->ssmp < 0) {
      sectPtr->ssmp = 0;
    }
    
    sectPtr->si.RestartPos = sectPtr->ssmp;
  }
  
  if (flag == SNACK_NEW_SOUND) {
    sectPtr->esmp = sectPtr->endSmp;
    
    if (sectPtr->endSmp < 0)
      sectPtr->esmp = sectPtr->si.BufPos - 1;
    
    if (sectPtr->endSmp > sectPtr->si.BufPos - 1)
      sectPtr->esmp = sectPtr->si.BufPos - 1;
    
    if (sectPtr->startSmp > sectPtr->endSmp && sectPtr->endSmp >= 0)
      sectPtr->startSmp = sectPtr->endSmp;
    
    if (sectPtr->startSmp < 0)
      sectPtr->startSmp = 0;
    
    sectPtr->ssmp = sectPtr->startSmp;
    
    if (sectPtr->ssmp > sectPtr->esmp)
      sectPtr->ssmp = sectPtr->esmp;
    
    if (sectPtr->ssmp > sectPtr->esmp - sectPtr->si.fftlen) {
      sectPtr->esmp = sectPtr->ssmp + sectPtr->si.fftlen;
      if (sectPtr->esmp > sectPtr->si.BufPos - 1) {
	sectPtr->esmp = sectPtr->si.BufPos - 1;
	sectPtr->ssmp = sectPtr->esmp - sectPtr->si.fftlen;
	if (sectPtr->ssmp < 0) {
	  sectPtr->ssmp = 0;
	}
      }
    }

    if (sectPtr->topFrequency <= 0.0) {
      sectPtr->si.topfrequency = sectPtr->si.sampfreq / 2.0;
    } else if (sectPtr->topFrequency > sectPtr->si.sampfreq / 2.0) {
      sectPtr->si.topfrequency = sectPtr->si.sampfreq / 2.0;
    } else {
      sectPtr->si.topfrequency = sectPtr->topFrequency;
    }
  }
  sectPtr->si.channel = sectPtr->si.channelSet;
  if (sectPtr->si.nchannels == 1) {
    sectPtr->si.channel = 0;
  }
  
  sectPtr->si.validStart = s->validStart;

  sectPtr->si.fftmax = -10000;
  sectPtr->si.fftmin = 10000;
  Snack_InitWindow(sectPtr->si.hamwin, sectPtr->si.winlen, sectPtr->si.fftlen,
		   sectPtr->si.windowType);
  ComputeSection((Tk_Item *)sectPtr);
  
  if (ComputeSectionCoords((Tk_Item *)sectPtr) != TCL_OK) {
    return;
  }
  
  Tk_CanvasEventuallyRedraw(sectPtr->canvas,
			    sectPtr->header.x1, sectPtr->header.y1,
			    sectPtr->header.x2, sectPtr->header.y2);
  
  if (sectPtr->debug) Snack_WriteLog("Exit UpdateSection\n");
}

static int
ConfigureSection(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, 
	      int argc, char **argv, int flags)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  Sound *s = sectPtr->sound;
  Tk_Window tkwin = Tk_CanvasTkwin(canvas);
  XGCValues gcValues;
  GC newGC;
  unsigned long mask;
  int DoCompute = 0;
#if defined(MAC)
  int i;
#endif

  if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, argv,
			 (char *) sectPtr, flags) != TCL_OK) return TCL_ERROR;

  if (sectPtr->debug) Snack_WriteLog("Enter ConfigureSection\n");

#if defined(MAC)
  for (i = 0; i < argc; i++) {
    if (strncmp(argv[i], "-anchor", strlen(argv[i])) == 0) {
      i++;
      if (strcmp(argv[i], "ne") == 0) {
	sectPtr->anchor = 1;
      } else if (strcmp(argv[i], "nw") == 0) {
	sectPtr->anchor = 7;
      } else if (strcmp(argv[i], "n") == 0) {
	sectPtr->anchor = 0;
      } else if (strcmp(argv[i], "e") == 0) {
	sectPtr->anchor = 2;
      } else if (strcmp(argv[i], "se") == 0) {
	sectPtr->anchor = 3;
      } else if (strcmp(argv[i], "sw") == 0) {
	sectPtr->anchor = 5;
      } else if (strcmp(argv[i], "s") == 0) {
	sectPtr->anchor = 4;
      } else if (strcmp(argv[i], "w") == 0) {
	sectPtr->anchor = 6;
      } else if (strncmp(argv[i], "center", strlen(argv[i])) == 0) {
	sectPtr->anchor = 8;
      }
      break;
    }
  }
#endif

  if (CheckFFTlen(interp, sectPtr->si.fftlen) != TCL_OK) return TCL_ERROR;

  if (CheckWinlen(interp, sectPtr->si.winlen) != TCL_OK) return TCL_ERROR;

  if (OptSpecified(OPTION_SOUND)) {
    if (sectPtr->newSoundName == NULL) {
      sectPtr->sound = NULL;
      if (sectPtr->id) Snack_RemoveCallback(s, sectPtr->id);
      sectPtr->id = 0;
      sectPtr->si.BufPos = 0;
      DoCompute = 1;
    } else {
      if ((s = Snack_GetSound(interp, sectPtr->newSoundName)) == NULL) {
	return TCL_ERROR;
      }
      if (s->storeType == SOUND_IN_CHANNEL) {
	Tcl_AppendResult(interp, sectPtr->newSoundName, 
			 " can not be linked to a channel", (char *) NULL);
	return TCL_ERROR;
      }
      if (s->storeType == SOUND_IN_FILE) {
	s->itemRefCnt++;
      }
      sectPtr->sound = s;
      if (sectPtr->soundName == NULL) {
	sectPtr->soundName = ckalloc(strlen(sectPtr->newSoundName)+1);
	strcpy(sectPtr->soundName, sectPtr->newSoundName);
      }
      if (strcmp(sectPtr->soundName, sectPtr->newSoundName) != 0) {
	Sound *t = Snack_GetSound(interp, sectPtr->soundName);
	ckfree(sectPtr->soundName);
	sectPtr->soundName = ckalloc(strlen(sectPtr->newSoundName)+1);
	strcpy(sectPtr->soundName, sectPtr->newSoundName);
	sectPtr->nPoints = 0;
	sectPtr->ssmp    = 0;
	sectPtr->esmp    = -1;
	Snack_RemoveCallback(t, sectPtr->id);
	sectPtr->id = 0;
      }
      if (!sectPtr->id) 
	sectPtr->id = Snack_AddCallback(s, UpdateSection, (int *)sectPtr);
      
      sectPtr->si.blocks = s->blocks;
      sectPtr->si.BufPos = s->length;
      sectPtr->si.sampfreq = s->sampfreq;
      sectPtr->si.sampformat = s->sampformat;
      sectPtr->si.nchannels = s->nchannels;
      sectPtr->si.storeType = s->storeType;
      DoCompute = 1;
    }
  }
  sectPtr->esmp = sectPtr->endSmp;

  if (sectPtr->endSmp < 0) 
    sectPtr->esmp = sectPtr->si.BufPos - 1;

  if (sectPtr->endSmp > sectPtr->si.BufPos - 1)
    sectPtr->esmp = sectPtr->si.BufPos - 1;

  if (sectPtr->startSmp > sectPtr->endSmp && sectPtr->endSmp >= 0)
    sectPtr->startSmp = sectPtr->endSmp;

  if (sectPtr->startSmp < 0)
    sectPtr->startSmp = 0;

  sectPtr->ssmp = sectPtr->startSmp;

  if (sectPtr->ssmp > sectPtr->esmp)
    sectPtr->ssmp = sectPtr->esmp;

  if (sectPtr->ssmp > sectPtr->esmp - sectPtr->si.fftlen) {
    sectPtr->esmp = sectPtr->ssmp + sectPtr->si.fftlen;
    if (sectPtr->esmp > sectPtr->si.BufPos - 1) {
      sectPtr->esmp = sectPtr->si.BufPos - 1;
      sectPtr->ssmp = sectPtr->esmp - sectPtr->si.fftlen;
      if (sectPtr->ssmp < 0) {
	sectPtr->ssmp = 0;
      }
    }
  }

  if (OptSpecified(OPTION_WINLEN))
    DoCompute = 1;

  if (OptSpecified(OPTION_FFTLEN)) {
    DoCompute = 1;
  }

  if (OptSpecified(OPTION_SKIP)) {
    DoCompute = 1;
  }

  if (OptSpecified(OPTION_START)) {
    DoCompute = 1;
  }

  if (OptSpecified(OPTION_END)) {
    DoCompute = 1;
  }

  if (sectPtr->topFrequency <= 0.0) {
    sectPtr->si.topfrequency = sectPtr->si.sampfreq / 2.0;
  } else if (sectPtr->topFrequency > sectPtr->si.sampfreq / 2.0) {
    sectPtr->si.topfrequency = sectPtr->si.sampfreq / 2.0;
  } else {
    sectPtr->si.topfrequency = sectPtr->topFrequency;
  }

  if (OptSpecified(OPTION_CHANNEL)) {
    if (GetChannel(interp, sectPtr->channelstr, sectPtr->si.nchannels, 
		   &sectPtr->si.channelSet) != TCL_OK) {
      return TCL_ERROR;
    }
    DoCompute = 1;
  }
  sectPtr->si.channel = sectPtr->si.channelSet;
  if (sectPtr->si.nchannels == 1) {
    sectPtr->si.channel = 0;
  }

  if (OptSpecified(OPTION_WINTYPE)) {
    if (GetWindowType(interp, sectPtr->windowTypeStr,
		      &sectPtr->si.windowTypeSet)
	!= TCL_OK) {
      return TCL_ERROR;
    }
    DoCompute = 1;
  }
  sectPtr->si.windowType = sectPtr->si.windowTypeSet;

  if (DoCompute) {
    sectPtr->nPoints = sectPtr->si.fftlen / 2;
    sectPtr->si.RestartPos = sectPtr->ssmp;
    sectPtr->si.fftmax = -10000;
    sectPtr->si.fftmin = 10000;
    Snack_InitWindow(sectPtr->si.hamwin, sectPtr->si.winlen,
		     sectPtr->si.fftlen, sectPtr->si.windowType);
    ComputeSection((Tk_Item *)sectPtr);
  }

  if (sectPtr->height <= 2) sectPtr->height = 0;

  if (sectPtr->fg == NULL) {
    newGC = None;
  } else {
    gcValues.foreground = sectPtr->fg->pixel;
    gcValues.line_width = 1;
    mask = GCForeground|GCLineWidth;
    if (sectPtr->fillStipple != None) {
      gcValues.stipple = sectPtr->fillStipple;
      gcValues.fill_style = FillStippled;
      mask |= GCStipple|GCFillStyle;
    }
    newGC = Tk_GetGC(tkwin, mask, &gcValues);
    gcValues.line_width = 0;
  }
  if (sectPtr->gc != None) {
    Tk_FreeGC(Tk_Display(tkwin), sectPtr->gc);
  }
  sectPtr->gc = newGC;
  
  ComputeSectionBbox(canvas, sectPtr);
  
  if (ComputeSectionCoords(itemPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  
  if (sectPtr->debug) Snack_WriteLog("Exit ConfigureSection\n");

  return TCL_OK;
}

static void
DeleteSection(Tk_Canvas canvas, Tk_Item *itemPtr, Display *display)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;

  if ((sectPtr->id) &&
      (Snack_GetSound(sectPtr->interp, sectPtr->soundName) != NULL)) {
    Snack_RemoveCallback(sectPtr->sound, sectPtr->id);
  }

  ckfree(sectPtr->soundName);

  if (sectPtr->coords != NULL) ckfree((char *) sectPtr->coords);

  if (sectPtr->si.hamwin != NULL) ckfree((char *)sectPtr->si.hamwin);

  if (sectPtr->xfft != NULL) ckfree((char *)sectPtr->xfft);

  if (sectPtr->ffts != NULL) ckfree((char *)sectPtr->ffts);

  if (sectPtr->fg != NULL) Tk_FreeColor(sectPtr->fg);

  if (sectPtr->fillStipple != None) Tk_FreeBitmap(display, sectPtr->fillStipple);

  if (sectPtr->gc != None) Tk_FreeGC(display, sectPtr->gc);

  if (sectPtr->sound->storeType == SOUND_IN_FILE) {
    sectPtr->sound->itemRefCnt--;
  }
}

static void
ComputeSectionBbox(Tk_Canvas canvas, SectionItem *sectPtr)
{
  int width = sectPtr->width;
  int height = sectPtr->height;
  int x = (int) (sectPtr->x + ((sectPtr->x >= 0) ? 0.5 : - 0.5));
  int y = (int) (sectPtr->y + ((sectPtr->y >= 0) ? 0.5 : - 0.5));
  
  switch (sectPtr->anchor) {
  case TK_ANCHOR_N:
    x -= width/2;
    break;
  case TK_ANCHOR_NE:
    x -= width;
    break;
  case TK_ANCHOR_E:
    x -= width;
    y -= height/2;
    break;
  case TK_ANCHOR_SE:
    x -= width;
    y -= height;
    break;
  case TK_ANCHOR_S:
    x -= width/2;
    y -= height;
    break;
  case TK_ANCHOR_SW:
    y -= height;
    break;
  case TK_ANCHOR_W:
    y -= height/2;
    break;
  case TK_ANCHOR_NW:
    break;
  case TK_ANCHOR_CENTER:
    x -= width/2;
    y -= height/2;
    break;
  }

  sectPtr->header.x1 = x;
  sectPtr->header.y1 = y;
  sectPtr->header.x2 = x + width;
  sectPtr->header.y2 = y + height;
}

static void
DisplaySection(Tk_Canvas canvas, Tk_Item *itemPtr, Display *display,
	    Drawable drawable, int x, int y, int width, int height)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  double *coords = sectPtr->coords;
  int i, nPoints = sectPtr->nPoints;
  XPoint *wpts = (XPoint *) ckalloc((unsigned)(nPoints * sizeof(XPoint)));
  XPoint *p = wpts;
  int xo = sectPtr->header.x1;
  int yo = sectPtr->header.y1;

  if (sectPtr->debug) Snack_WriteLogInt("Enter DisplaySection", nPoints);

  if (sectPtr->gc == None) return;

  if (sectPtr->fillStipple != None)
    Tk_CanvasSetStippleOrigin(canvas, sectPtr->gc);

  for (i = 0; i < sectPtr->nPoints; i++) {
    Tk_CanvasDrawableCoords(canvas, xo + coords[0], yo + coords[1],
			    &p->x, &p->y);
    coords += 2;
    p++;
  }

  XDrawLines(display, drawable, sectPtr->gc, wpts, nPoints,
	     CoordModeOrigin);
  
  if (sectPtr->frame) {
    Tk_CanvasDrawableCoords(canvas, (double) xo, (double) yo,
			    &sectPtr->fpts[0].x, &sectPtr->fpts[0].y);
    Tk_CanvasDrawableCoords(canvas, (double) (xo + sectPtr->width - 1),
			    (double) yo,
			    &sectPtr->fpts[1].x, &sectPtr->fpts[1].y);
    Tk_CanvasDrawableCoords(canvas, (double) (xo + sectPtr->width - 1),
			    (double) (yo + sectPtr->height - 1),
			    &sectPtr->fpts[2].x, &sectPtr->fpts[2].y);
    Tk_CanvasDrawableCoords(canvas, (double) xo, 
			    (double) (yo + sectPtr->height - 1),
			    &sectPtr->fpts[3].x, &sectPtr->fpts[3].y);
    Tk_CanvasDrawableCoords(canvas, (double) xo, (double) yo,
			    &sectPtr->fpts[4].x, &sectPtr->fpts[4].y);
    XDrawLines(display, drawable, sectPtr->gc, sectPtr->fpts, 5, CoordModeOrigin);
  }

  ckfree((char *) wpts);

  if (sectPtr->debug) Snack_WriteLog("Exit DisplaySection\n");
}

static double
SectionToPoint(Tk_Canvas canvas, Tk_Item *itemPtr, double *coords)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  double dx = 0.0, dy = 0.0;
  double x1 = sectPtr->header.x1;
  double y1 = sectPtr->header.y1;
  double x2 = sectPtr->header.x2;
  double y2 = sectPtr->header.y2;
  
  if (coords[0] < x1)
    dx = x1 - coords[0];
  else if (coords[0] > x2)
    dx = coords[0] - x2;
  else
    dx = 0;

  if (coords[1] < y1)
    dy = y1 - coords[1];
  else if (coords[1] > y2)
    dy = coords[1] - y2;
  else
    dy = 0;
  
  return hypot(dx, dy);
}

static int
SectionToArea(Tk_Canvas canvas, Tk_Item *itemPtr, double *rectPtr)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;

  if ((rectPtr[2] <= sectPtr->header.x1) ||
      (rectPtr[0] >= sectPtr->header.x2) ||
      (rectPtr[3] <= sectPtr->header.y1) ||
      (rectPtr[1] >= sectPtr->header.y2))
    return -1;

  if ((rectPtr[0] <= sectPtr->header.x1) &&
      (rectPtr[1] <= sectPtr->header.y1) &&
      (rectPtr[2] >= sectPtr->header.x2) &&
      (rectPtr[3] >= sectPtr->header.y2))
    return 1;
 
  return 0;
}

static void
ScaleSection(Tk_Canvas canvas, Tk_Item *itemPtr, double ox, double oy,
	  double sx, double sy)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  double *coords = sectPtr->coords;
  int i;
  
  for (i = 0; i < sectPtr->nPoints; i++) {
    coords[0] = ox + sx * (coords[0] - ox);
    coords[1] = oy + sy * (coords[1] - oy);
    coords += 2;
  }
  sectPtr->width  = (int) (sx * sectPtr->width);
  sectPtr->height = (int) (sy * sectPtr->height);

  ComputeSectionBbox(canvas, sectPtr);
}

static void
TranslateSection(Tk_Canvas canvas, Tk_Item *itemPtr, double dx, double dy)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;

  sectPtr->x += dx;
  sectPtr->y += dy;
  ComputeSectionBbox(canvas, sectPtr);
}

static int
SectionToPS(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, int prepass)
{
  SectionItem *sectPtr = (SectionItem *) itemPtr;
  double  *coords = sectPtr->coords;
  int     nPoints = sectPtr->nPoints;
  char buffer[100];
  int xo = sectPtr->header.x1;
  int yo = sectPtr->header.y1;
 
  if (sectPtr->fg == NULL) {
    return TCL_OK;
  }

  Tcl_AppendResult(interp, "%% SECT BEGIN\n", (char *) NULL);

  sprintf(buffer, "%.15g %.15g moveto\n", coords[0] + xo,
	  Tk_CanvasPsY(canvas, (double) (coords[1] + yo)));
  Tcl_AppendResult(interp, buffer, (char *) NULL);
  coords += 2;
  for (nPoints--; nPoints > 0; nPoints--) {
    sprintf(buffer, "%.15g %.15g lineto\n", coords[0] + xo,
	    Tk_CanvasPsY(canvas, (double) (coords[1] + yo)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    coords += 2;
  }

  if (sectPtr->frame) {
    sprintf(buffer, "%.15g %.15g moveto\n", (double) xo, Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo + sectPtr->width - 1,
	    Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo + sectPtr->width - 1,
	    Tk_CanvasPsY(canvas, (double) (yo + sectPtr->height - 1)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo,
	    Tk_CanvasPsY(canvas, (double) (yo + sectPtr->height - 1)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo,
	    Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);
  }

  Tcl_AppendResult(interp, "1 setlinewidth\n", (char *) NULL);
  Tcl_AppendResult(interp, "0 setlinecap\n0 setlinejoin\n", (char *) NULL);
  if (Tk_CanvasPsColor(interp, canvas, sectPtr->fg) != TCL_OK) {
    return TCL_ERROR;
  };
  if (sectPtr->fillStipple != None) {
    Tcl_AppendResult(interp, "StrokeClip ", (char *) NULL);
    if (Tk_CanvasPsStipple(interp, canvas, sectPtr->fillStipple) != TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    Tcl_AppendResult(interp, "stroke\n", (char *) NULL);
  }
  
  Tcl_AppendResult(interp, "%% SECT END\n", (char *) NULL);

  return TCL_OK;
}

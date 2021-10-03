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

/*
 * Wave item structure
 */

typedef struct WaveItem  {

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
  int channel;
  int channelSet;
  int nchannels;
  int sampfreq;
  int sampformat;
  short **blocks;
  int bufPos;
  int limit;
  int subSample;
  double pixpsec;
  int height;
  int width;
  int startSmp;
  int endSmp;
  int ssmp;
  int esmp;
  int zeroLevel;
  int frame;
  int id;
  int mode;
  XPoint fpts[5];
  int subSampleInt;
  char *channelStr;
  int debug;
  int storeType;
  char *preCompFile;
  struct WaveItem *preWI;
  Sound *preSound;
  int preCompInvalid;
  int validStart;
  char *progressCmd;
  Tcl_Obj *cmdPtr;
  Tcl_Interp *interp;

} WaveItem;

Tk_CustomOption waveTagsOption = { (Tk_OptionParseProc *) NULL,
				   (Tk_OptionPrintProc *) NULL,
				   (ClientData) NULL };

typedef enum {
  OPTION_ANCHOR,
  OPTION_TAGS,
  OPTION_SOUND,
  OPTION_HEIGHT,
  OPTION_WIDTH,
  OPTION_PIXPSEC,
  OPTION_START,
  OPTION_END,
  OPTION_FILL,
  OPTION_STIPPLE,
  OPTION_ZEROLEVEL,
  OPTION_FRAME,
  OPTION_LIMIT,
  OPTION_SUBSAMPLE,
  OPTION_CHANNEL,
  OPTION_PRECOMPWAVE,
  OPTION_PROGRESS
} ConfigSpec;

static Tk_ConfigSpec configSpecs[] = {

  {TK_CONFIG_ANCHOR, "-anchor", (char *) NULL, (char *) NULL,
   "nw", Tk_Offset(WaveItem, anchor), TK_CONFIG_DONT_SET_DEFAULT},

  {TK_CONFIG_CUSTOM, "-tags", (char *) NULL, (char *) NULL,
   (char *) NULL, 0, TK_CONFIG_NULL_OK, &waveTagsOption},
  
  {TK_CONFIG_STRING, "-sound", (char *) NULL, (char *) NULL,
   "", Tk_Offset(WaveItem, newSoundName), TK_CONFIG_NULL_OK},
  
  {TK_CONFIG_INT, "-height", (char *) NULL, (char *) NULL,
   "100", Tk_Offset(WaveItem, height), 0},
  
  {TK_CONFIG_PIXELS, "-width", (char *) NULL, (char *) NULL,
   "378", Tk_Offset(WaveItem, width), 0},
  
  {TK_CONFIG_DOUBLE, "-pixelspersecond", "pps", (char *) NULL,
   "250.0", Tk_Offset(WaveItem, pixpsec), 0},

  {TK_CONFIG_INT, "-start", (char *) NULL, (char *) NULL,
   "0", Tk_Offset(WaveItem, startSmp), 0},
  
  {TK_CONFIG_INT, "-end", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(WaveItem, endSmp), 0},
  
  {TK_CONFIG_COLOR, "-fill", (char *) NULL, (char *) NULL,
   "black", Tk_Offset(WaveItem, fg), TK_CONFIG_NULL_OK},
  
  {TK_CONFIG_BITMAP, "-stipple", (char *) NULL, (char *) NULL,
   (char *) NULL, Tk_Offset(WaveItem, fillStipple), TK_CONFIG_NULL_OK},

  {TK_CONFIG_BOOLEAN, "-zerolevel", "zerolevel", (char *) NULL,
   "yes", Tk_Offset(WaveItem, zeroLevel), TK_CONFIG_NULL_OK},

  {TK_CONFIG_BOOLEAN, "-frame", (char *) NULL, (char *) NULL,
   "no", Tk_Offset(WaveItem, frame), TK_CONFIG_NULL_OK},

  {TK_CONFIG_INT, "-limit", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(WaveItem, limit), 0},
  
  {TK_CONFIG_INT, "-subsample", "subsample", (char *) NULL,
   "1", Tk_Offset(WaveItem, subSampleInt), TK_CONFIG_NULL_OK},

  {TK_CONFIG_STRING, "-channel", (char *) NULL, (char *) NULL,
   "-1", Tk_Offset(WaveItem, channelStr), TK_CONFIG_NULL_OK},

  {TK_CONFIG_STRING, "-shapefile", (char *) NULL, (char *) NULL,
   "", Tk_Offset(WaveItem, preCompFile), TK_CONFIG_NULL_OK},

  {TK_CONFIG_STRING, "-progress", (char *) NULL, (char *) NULL,
   "", Tk_Offset(WaveItem, progressCmd), TK_CONFIG_NULL_OK},

  {TK_CONFIG_INT, "-debug", (char *) NULL, (char *) NULL,
   "0", Tk_Offset(WaveItem, debug), 0},

  {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
   (char *) NULL, 0, 0}

};

/*
 * Protos
 */

  static void   ComputeWaveBbox(Tk_Canvas canvas, WaveItem *wavePtr);

  static int    ComputeWaveCoords(Tk_Item *itemPtr);
  
  static int    ConfigureWave(Tcl_Interp *interp, Tk_Canvas canvas, 
			      Tk_Item *itemPtr, int argc,
			      char **argv, int flags);

  static int    CreateWave(Tcl_Interp *interp, Tk_Canvas canvas,
			   struct Tk_Item *itemPtr,
			   int argc, char **argv);

  static void   DeleteWave(Tk_Canvas canvas, Tk_Item *itemPtr,
			   Display *display);

  static void   DisplayWave(Tk_Canvas canvas, Tk_Item *itemPtr,
			    Display *display, Drawable dst,
			    int x, int y, int width, int height);

  static void   ScaleWave(Tk_Canvas canvas, Tk_Item *itemPtr,
			  double originX, double originY,
			  double scaleX, double scaleY);

  static void   TranslateWave(Tk_Canvas canvas, Tk_Item *itemPtr,
			      double deltaX, double deltaY);
  
  static int    WaveCoords(Tcl_Interp *interp, Tk_Canvas canvas,
			   Tk_Item *itemPtr, int argc, char **argv);
  
  static int    WaveToArea(Tk_Canvas canvas, Tk_Item *itemPtr,
			   double *rectPtr);
  
  static double WaveToPoint(Tk_Canvas canvas, Tk_Item *itemPtr,
			    double *coords);
  
  static int    WaveToPS(Tcl_Interp *interp, Tk_Canvas canvas,
			 Tk_Item *itemPtr, int prepass);

/*
 * Wave item type
 */

Tk_ItemType snackWaveType = {
  "waveform",
  sizeof(WaveItem),
  CreateWave,
  configSpecs,
  ConfigureWave,
  WaveCoords,
  DeleteWave,
  DisplayWave,
  0,
  WaveToPoint,
  WaveToArea,
  WaveToPS,
  ScaleWave,
  TranslateWave,
  (Tk_ItemIndexProc *) NULL,
  (Tk_ItemCursorProc *) NULL,
  (Tk_ItemSelectionProc *) NULL,
  (Tk_ItemInsertProc *) NULL,
  (Tk_ItemDCharsProc *) NULL,
  (Tk_ItemType *) NULL
};

static int
CreateWave(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr,
	   int argc, char **argv)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;

  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     Tk_PathName(Tk_CanvasTkwin(canvas)), " create ",
		     itemPtr->typePtr->name, " x y ?opts?\"", (char *) NULL);
    return TCL_ERROR;
  }

  wavePtr->canvas = canvas;
  wavePtr->anchor = TK_ANCHOR_NW;
  wavePtr->nPoints = 0;
  wavePtr->coords = NULL;
  wavePtr->fg = None;
  wavePtr->fillStipple = None;
  wavePtr->gc = None;
  wavePtr->newSoundName = NULL;
  wavePtr->soundName = NULL;
  wavePtr->sound = NULL;
  wavePtr->pixpsec = 250.0;
  wavePtr->height = 100;
  wavePtr->width = 378;
  wavePtr->startSmp = 0;
  wavePtr->endSmp = -1;
  wavePtr->ssmp = 0;
  wavePtr->esmp = -1;
  wavePtr->id = 0;
  wavePtr->mode = CONF_WIDTH;
  wavePtr->zeroLevel = 1;
  wavePtr->frame = 0;
  wavePtr->channelStr = NULL;
  wavePtr->channel = -1;
  wavePtr->channelSet = -1;
  wavePtr->nchannels = 1;
  wavePtr->sampfreq = 16000;
  wavePtr->sampformat = LIN16;
  wavePtr->bufPos = 0;
  wavePtr->limit = -1;
  wavePtr->subSampleInt = 1;
  wavePtr->subSample = 1;
  wavePtr->preCompFile = NULL;
  wavePtr->preSound = NULL;
  wavePtr->preWI = NULL;
  wavePtr->preCompInvalid = 0;
  wavePtr->validStart = 0;
  wavePtr->progressCmd = NULL;
  wavePtr->cmdPtr = NULL;
  wavePtr->interp = interp;
  wavePtr->debug = 0;
  wavePtr->x = 0;
  wavePtr->y = 0;

  if ((Tk_CanvasGetCoord(interp, canvas, argv[0], &wavePtr->x) != TCL_OK) ||
      (Tk_CanvasGetCoord(interp, canvas, argv[1], &wavePtr->y) != TCL_OK))
    return TCL_ERROR;
  
  if (ConfigureWave(interp, canvas, itemPtr, argc-2, argv+2, 0) == TCL_OK)
    return TCL_OK;

  DeleteWave(canvas, itemPtr, Tk_Display(Tk_CanvasTkwin(canvas)));
  return TCL_ERROR;
}

static void
WaveMaxMin(WaveItem *wavePtr, SnackLinkedFileInfo *info, int start, int stop,
	   int *maxi, int *mini)
{
  int i, j, maxval = -32768, minval = 32767, allFlag = 0, val;
  int nchan = wavePtr->nchannels, chan = wavePtr->channel;
  int inc = nchan * wavePtr->subSample;

  if (start < 0 || stop > wavePtr->bufPos - 1 || stop == 0 ||
      (wavePtr->blocks[0] == NULL && wavePtr->storeType == SOUND_IN_MEMORY)) {
    if (wavePtr->sampformat == LIN8OFFSET) {
      *maxi = 128;
      *mini = 128;
    } else {
      *maxi = 0;
      *mini = 0;
    }
    return;
  }
  if (chan == -1) {
    allFlag = 1;
    chan = 0;
  }

  start = start * wavePtr->nchannels + chan;
  stop  = stop  * wavePtr->nchannels + chan + wavePtr->nchannels - 1;

  switch (wavePtr->sampformat) {
  case LIN16:
    for (i = start; i <= stop; i += inc) {
      if (wavePtr->storeType == SOUND_IN_MEMORY) {
	val = SSAMPLE(wavePtr, i);
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += SSAMPLE(wavePtr, i + j);
	  }
	  val = val / nchan;
	}
      } else {
	val = GetSample(info, i);	
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += GetSample(info, i + j);
	  }
	  val = val / nchan;
	}
      }
      if (val > maxval) {
	maxval = val;
      }
      if (val < minval) {
	minval = val;
      }
    }
    break;
  case ALAW:
    for (i = start; i <= stop; i += inc) {
      if (wavePtr->storeType == SOUND_IN_MEMORY) {
	val = Snack_Alaw2Lin(UCSAMPLE(wavePtr, i));
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += Snack_Alaw2Lin(UCSAMPLE(wavePtr, i + j));
	  }
	  val = val / nchan;
	}
      } else {
	val = Snack_Alaw2Lin((unsigned char)GetSample(info, i));	
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += Snack_Alaw2Lin((unsigned char)GetSample(info, i + j));
	  }
	  val = val / nchan;
	}
      }
      if (val > maxval) {
	maxval = val;
      }
      if (val < minval) {
	minval = val;
      }
    }
    break;
  case MULAW:
    for (i = start; i <= stop; i += inc) {
      if (wavePtr->storeType == SOUND_IN_MEMORY) {
	val = Snack_Mulaw2Lin(UCSAMPLE(wavePtr, i));
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += Snack_Mulaw2Lin(UCSAMPLE(wavePtr, i + j));
	  }
	  val = val / nchan;
	}
      } else {
	val = Snack_Mulaw2Lin((unsigned char)GetSample(info, i));	
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += Snack_Mulaw2Lin((unsigned char)GetSample(info, i + j));
	  }
	  val = val / nchan;
	}
      }
      if (val > maxval) {
	maxval = val;
      }
      if (val < minval) {
	minval = val;
      }
    }
    break;
  case LIN8OFFSET:
    for (i = start; i <= stop; i += inc) {
      if (wavePtr->storeType == SOUND_IN_MEMORY) {
	val = UCSAMPLE(wavePtr, i);
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += UCSAMPLE(wavePtr, i + j);
	  }
	  val = val / nchan;
	}
      } else {
	val = GetSample(info, i);
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += GetSample(info, i + j);
	  }
	  val = val / nchan;
	}
      }
      if (val > maxval) {
	maxval = val;
      }
      if (val < minval) {
	minval = val;
      }
    }
    break;
  default:
    for (i = start; i <= stop; i += inc) {
      if (wavePtr->storeType == SOUND_IN_MEMORY) {
	val = CSAMPLE(wavePtr, i);
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += (int) CSAMPLE(wavePtr, i + j);
	  }
	  val = val / nchan;
	}
      } else {
	val = GetSample(info, i);
	if (allFlag) {
	  for (j = 1; j < nchan; j++) {
	    val += GetSample(info, i + j);
	  }
	  val = val / nchan;
	}
      }
      if (val > maxval) {
	maxval = val;
      }
      if (val < minval) {
	minval = val;
      }
    }
  }
  if (wavePtr->limit > 0) {
    if (maxval > wavePtr->limit) {
      maxval = wavePtr->limit;
    }
    if (minval < -wavePtr->limit) {
      minval = -wavePtr->limit;
    }
  }
  *maxi = maxval;
  *mini = minval;
}

static int
WaveCoords(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, int argc,
	   char **argv)
{
  WaveItem *wPtr = (WaveItem *) itemPtr;
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
    ComputeWaveBbox(canvas, wPtr);
  } else {
    char buf[80];

    sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", argc);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return TCL_ERROR;
  }

  return TCL_OK;
}

#define WIDEWAVE 100000

static int
ComputeWaveCoords(Tk_Item *itemPtr)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  int i = 0, maxv, minv;
  float scale = 1000000.0;
  int yh = wavePtr->height / 2;
  int nPoints = wavePtr->nPoints;
  SnackLinkedFileInfo info;
  Tcl_Interp *interp;
  int usePre = 0;

  if (wavePtr->debug == 1) Snack_WriteLog("Enter ComputeWaveCoords\n"); 

  if (wavePtr->coords != NULL) ckfree((char *) wavePtr->coords);
  wavePtr->coords = (double *) ckalloc(sizeof(double) * 2 * nPoints);

  if (wavePtr->sound == NULL) {
    for (i = 0; i < nPoints; i++) {
      wavePtr->coords[i*2]   = (double) i;
      wavePtr->coords[i*2+1] = (double) yh;
    }
    return TCL_OK;
  }

  maxv = wavePtr->sound->maxsamp;
  minv = wavePtr->sound->minsamp;
  interp = wavePtr->sound->interp;

  if (wavePtr->storeType != SOUND_IN_MEMORY) {
    if (OpenLinkedFile(wavePtr->sound, &info) != TCL_OK) {
      for (i = 0; i < nPoints; i++) {
	wavePtr->coords[i*2]   = (double) i;
	wavePtr->coords[i*2+1] = (double) yh;
      }
      return TCL_OK;
    }
  }

  if (wavePtr->preCompFile != NULL && wavePtr->sound->active != READ) {
    char *type = NULL;

    if (wavePtr->preSound != NULL) {
      wavePtr->preSound->fcname = NULL;
      Snack_DeleteSound(wavePtr->preSound);
    }
    wavePtr->preSound = Snack_NewSound(16000, LIN8, SNACK_MONO);
    if (wavePtr->preSound != NULL) {
      wavePtr->preSound->fcname = wavePtr->preCompFile;
      type = LoadSound(wavePtr->preSound, interp, NULL, 0, -1);
      if (wavePtr->preWI != NULL) ckfree((char *)wavePtr->preWI);
      wavePtr->preWI = (WaveItem *) ckalloc(sizeof(WaveItem));
      if (wavePtr->preWI != NULL) {
	wavePtr->preWI->nchannels = 1;
	wavePtr->preWI->channel =   0;
	wavePtr->preWI->subSample = 1;
	wavePtr->preWI->bufPos = wavePtr->preSound->length;
	wavePtr->preWI->blocks = wavePtr->preSound->blocks;
	wavePtr->preWI->storeType = SOUND_IN_MEMORY;
	wavePtr->preWI->sampformat = LIN8;
	wavePtr->preWI->limit = wavePtr->limit;
      }
    }

    if ((type == NULL || wavePtr->preCompInvalid) && wavePtr->preSound != NULL) {

      /* Compute and store wave */

      int nStore = (int) (200.0 * wavePtr->sound->length /
	wavePtr->sound->sampfreq);
      maxv = 0;
      minv = 0;
      if (wavePtr->debug == 1) Snack_WriteLog("Saving computed waveform\n");
      wavePtr->preCompInvalid = 0;
      Snack_ResizeSoundStorage(wavePtr->preSound, nStore);
      wavePtr->preSound->length = nStore;
      if (wavePtr->cmdPtr != NULL) {
	Snack_ProgressCallback(wavePtr->cmdPtr, interp,
			       "Computing waveform", 0.0);
      }
      for (i = 0; i < nStore / 2; i++) {
	float fraq = (float) wavePtr->sound->length / (nStore / 2);
	int start = (int) (i     * fraq);
	int stop  = (int) ((i+1) * fraq);
	int wtop, wbot;
	
	WaveMaxMin(wavePtr, &info, start, stop, &wtop, &wbot);

	if (maxv < wtop) maxv = wtop;
	if (minv > wbot) minv = wbot;

	switch (wavePtr->sampformat) {
	case LIN16:
	case MULAW:
	case ALAW:
	  wtop = wtop >> 8;
	  wbot = wbot >> 8;
	  break;
	case LIN8OFFSET:
	  wtop -= 128;
	  wbot -= 128;
	  break;
	case LIN8:
	  break;
	}
	Snack_SetSample(wavePtr->preSound, 0, i*2,   (char) wtop);
	Snack_SetSample(wavePtr->preSound, 0, i*2+1, (char) wbot);
	if ((wavePtr->cmdPtr != NULL) && ((i % 1000) == 999)) {
	  int res = Snack_ProgressCallback(wavePtr->cmdPtr, interp,
				 "Computing waveform", (double) i/(nStore/2));
	  if (res != TCL_OK) {
	    for (;i < nStore / 2; i++) {
	      Snack_SetSample(wavePtr->preSound, 0, i*2,   (char) 0);
	      Snack_SetSample(wavePtr->preSound, 0, i*2+1, (char) 0);
	    }
	    break;
	  }
	}
      }
      if (wavePtr->cmdPtr != NULL) {
	Snack_ProgressCallback(wavePtr->cmdPtr, interp,
			       "Computing waveform", 1.0);
      }
      if (SaveSound(wavePtr->preSound, interp, wavePtr->preCompFile, NULL,
		0, wavePtr->preSound->length, RAW_STRING) == TCL_ERROR) {
	if (wavePtr->debug == 1) Snack_WriteLog("Failed saving waveform\n");
	wavePtr->preCompFile = NULL;
      }
      wavePtr->preWI->bufPos = wavePtr->preSound->length;
      wavePtr->preWI->blocks = wavePtr->preSound->blocks;
      /*      if (wavePtr->sound->cmdPtr != NULL) {
	Tcl_DecrRefCount(wavePtr->sound->cmdPtr);
	wavePtr->sound->cmdPtr = NULL;
      }*/
    }

    if (wavePtr->preSound != NULL && wavePtr->preWI != NULL) {

      /* Use precomputed wave */

      float left  = ((float) wavePtr->ssmp / wavePtr->sound->length) *
	wavePtr->preSound->length;
      float right = ((float) wavePtr->esmp / wavePtr->sound->length) *
	wavePtr->preSound->length;
      float fraq  = (right - left) / nPoints;

      if (fraq > 1.0) {
	usePre = 1;
	switch (wavePtr->sampformat) {
	case LIN16:
	case MULAW:
	case ALAW:
	  maxv = maxv >> 8;
	  minv = minv >> 8;
	  break;
	case LIN8OFFSET:
	  maxv -= 128;
	  minv -= 128;
	  break;
	case LIN8:
	  break;
	}

	if (wavePtr->debug == 1) Snack_WriteLog("Using precomputed waveform\n");
	for (i = 0; i < nPoints/2; i++) {
	  int start = (int) (left + 2*(i * fraq));
	  int stop  = (int) (left + 2*(i+1)*fraq);
	  int wtop, wbot;

	  WaveMaxMin(wavePtr->preWI, &info, start, stop, &wtop, &wbot);

	  if (maxv < wtop) maxv = wtop;
	  if (minv > wbot) minv = wbot;

	  wavePtr->coords[i*4]   = i;
	  wavePtr->coords[i*4+2] = i;
	  if (i > 0 && wavePtr->coords[i*4-1] <= wtop) {
	    wavePtr->coords[i*4+1] = wtop;
	    wavePtr->coords[i*4+3] = wbot;
	  } else {
	    wavePtr->coords[i*4+1] = wbot;
	    wavePtr->coords[i*4+3] = wtop;
	  }
	}

	if (wavePtr->sampformat == LIN8OFFSET) {
	  maxv += 128;
	  minv += 128;
	}
      } else {
	usePre = 0;
      }
    }
  }

  if (!usePre) {
    /* int doCallback = ((wavePtr->esmp - wavePtr->ssmp) > WIDEWAVE) ? 1:0;*/

    if (wavePtr->debug == 1) Snack_WriteLog("Default waveform computation\n");
    /*
    if ((wavePtr->cmdPtr != NULL) && doCallback) {
      Snack_ProgressCallback(wavePtr->cmdPtr, interp,
			     "Drawing waveform", 0.0);
    }*/
    for (i = 0; i < nPoints / 2; i++) {
      float fraq = (float) (wavePtr->esmp - wavePtr->ssmp) / (nPoints / 2);
      int start = wavePtr->ssmp + (int) (i     * fraq) - wavePtr->validStart;
      int stop  = wavePtr->ssmp + (int) ((i+1) * fraq) - wavePtr->validStart;
      int wtop, wbot;
      /*      int interval = (int) (0.5 + 100000 / fraq);*/

      WaveMaxMin(wavePtr, &info, start, stop, &wtop, &wbot);

      if (maxv < wtop) maxv = wtop;
      if (minv > wbot) minv = wbot;

      if (wavePtr->sampformat == LIN8OFFSET) {
	wtop -= 128;
	wbot -= 128;
      }
      
      wavePtr->coords[i*4]   = i;
      wavePtr->coords[i*4+2] = i;
      if (i > 0 && wavePtr->coords[i*4-1] <= wtop) {
	wavePtr->coords[i*4+1] = wtop;
	wavePtr->coords[i*4+3] = wbot;
      } else {
	wavePtr->coords[i*4+1] = wbot;
	wavePtr->coords[i*4+3] = wtop;
      }
      /*if ((wavePtr->cmdPtr != NULL) && ((i % interval) == (interval-1))) {
	int res = Snack_ProgressCallback(wavePtr->cmdPtr, interp,
			    "Drawing waveform", (double) i/(nPoints/2));
	if (res != TCL_OK) {
	  for (;i < nPoints / 2; i++) {
	    wavePtr->coords[i*4]   = i;
	    wavePtr->coords[i*4+2] = i;
	    wavePtr->coords[i*4+1] = 0;
	    wavePtr->coords[i*4+3] = 0;
	  }
	  break;
	}
      }*/
    }
    /*if ((wavePtr->cmdPtr != NULL) && doCallback) {
      Snack_ProgressCallback(wavePtr->cmdPtr, interp,
			     "Drawing waveform", 1.0);
    }*/
  }

  if (maxv > wavePtr->sound->maxsamp) {
    wavePtr->sound->maxsamp = maxv;
  }
  if (minv < wavePtr->sound->minsamp) {
    wavePtr->sound->minsamp = minv;
  }

  if (wavePtr->limit > 0) {
    maxv = wavePtr->limit;
    minv = -wavePtr->limit;
  }
  if (wavePtr->sampformat == LIN8OFFSET) {
    maxv -= 128;
    minv -= 128;
  }
  if (wavePtr->height > 2) {
    scale = 2 * ((maxv > -minv) ? maxv : -minv) / (float)(wavePtr->height - 2);
  }
  if (scale < 0.00001) {
    scale = (float) 0.00001;
  }

  for (i = 0; i < nPoints / 2; i++) {
    wavePtr->coords[i*4+1] = yh - wavePtr->coords[i*4+1] / scale;
    wavePtr->coords[i*4+3] = yh - wavePtr->coords[i*4+3] / scale;
  }

  ComputeWaveBbox(wavePtr->canvas, wavePtr);

  if (usePre) {
    switch (wavePtr->sampformat) {
    case LIN16:
    case MULAW:
    case ALAW:
      maxv = maxv * 256;
      minv = minv * 256;
      break;
    case LIN8OFFSET:
      maxv += 128;
      minv += 128;
      break;
    case LIN8:
      break;
    }
    if (maxv > wavePtr->sound->maxsamp) {
      wavePtr->sound->maxsamp = maxv;
    }
    if (minv < wavePtr->sound->minsamp) {
      wavePtr->sound->minsamp = minv;
    }
  }

  if (wavePtr->storeType != SOUND_IN_MEMORY) {
    CloseLinkedFile(&info);
  }

  if (wavePtr->debug == 1) Snack_WriteLog("Exit ComputeWaveCoords\n"); 

  return TCL_OK;
}

#define NSAMPLES 100000

static void
UpdateWave(ClientData clientData, int flag)
{
  WaveItem *wavePtr = (WaveItem *) clientData;
  Sound *s = wavePtr->sound;

  if (wavePtr->debug == 1) Snack_WriteLogInt("Enter UpdateWave", flag);

  if (wavePtr->canvas == NULL || wavePtr->sound == NULL) return;

  Tk_CanvasEventuallyRedraw(wavePtr->canvas,
			    wavePtr->header.x1, wavePtr->header.y1, 
			    wavePtr->header.x2, wavePtr->header.y2);

  wavePtr->blocks = s->blocks;
  wavePtr->bufPos = s->length;
  wavePtr->storeType = s->storeType;

  if ((flag == SNACK_MORE_SOUND) || (wavePtr->endSmp < 0)) {
    wavePtr->esmp = wavePtr->bufPos - 1;
  }
  
  if (wavePtr->esmp > wavePtr->bufPos - 1)
    wavePtr->esmp = wavePtr->bufPos - 1;
  
  if (wavePtr->endSmp > 0) 
    wavePtr->esmp = wavePtr->endSmp;
  
  if (wavePtr->endSmp > wavePtr->bufPos - 1)
    wavePtr->esmp = wavePtr->bufPos - 1;
  
  wavePtr->ssmp = wavePtr->startSmp;
  
  if (wavePtr->ssmp > wavePtr->esmp)
    wavePtr->ssmp = wavePtr->esmp;

  wavePtr->sampfreq = s->sampfreq;
  wavePtr->sampformat = s->sampformat;
  wavePtr->nchannels = s->nchannels;

  wavePtr->channel = wavePtr->channelSet;
  if (wavePtr->nchannels == 1) {
    wavePtr->channel = 0;
  }

  if (wavePtr->mode == CONF_WIDTH) {
    if (wavePtr->esmp != wavePtr->ssmp) {
      wavePtr->pixpsec = (float) wavePtr->width * wavePtr->sampfreq /
	(wavePtr->esmp - wavePtr->ssmp);
    }
  }
  else if (wavePtr->mode == CONF_PPS) {

    wavePtr->nPoints = 2 * (int)((wavePtr->esmp - wavePtr->ssmp) * 
				 wavePtr->pixpsec / wavePtr->sampfreq);
    wavePtr->width = wavePtr->nPoints / 2;
  } 
  else if (wavePtr->mode == CONF_WIDTH_PPS) {
    wavePtr->ssmp = (int) (wavePtr->esmp - wavePtr->width *
			   wavePtr->sampfreq / wavePtr->pixpsec);
    wavePtr->nPoints = 2 * wavePtr->width;
  }

  if (wavePtr->subSampleInt == 0) {
    if (wavePtr->esmp - wavePtr->ssmp > NSAMPLES) { 
      wavePtr->subSample = (wavePtr->esmp - wavePtr->ssmp) / NSAMPLES;
    } else {
      wavePtr->subSample = 1;
    }
  } else {
    wavePtr->subSample = wavePtr->subSampleInt;
  }

  wavePtr->preCompInvalid = 1;
  wavePtr->validStart = s->validStart;

  if (ComputeWaveCoords((Tk_Item *)wavePtr) != TCL_OK) {
    return;
  }
  Tk_CanvasEventuallyRedraw(wavePtr->canvas,
			    wavePtr->header.x1, wavePtr->header.y1,
			    wavePtr->header.x2, wavePtr->header.y2);

  if (wavePtr->debug == 1) Snack_WriteLogInt("Exit UpdateWave", wavePtr->nPoints);
}

static int
ConfigureWave(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, 
	      int argc, char **argv, int flags)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  Sound *s = wavePtr->sound;
  Tk_Window tkwin = Tk_CanvasTkwin(canvas);
  XGCValues gcValues;
  GC newGC;
  unsigned long mask;
#if defined(MAC)
  int i;
#endif

  if (argc == 0) return TCL_OK;

  if (Tk_ConfigureWidget(interp, tkwin, configSpecs, argc, argv,
			 (char *) wavePtr, flags) != TCL_OK) return TCL_ERROR;

  if (wavePtr->debug == 1) Snack_WriteLog("Enter ConfigureWave\n");

#if defined(MAC)
  for (i = 0; i < argc; i++) {
    if (strncmp(argv[i], "-anchor", strlen(argv[i])) == 0) {
      i++;
      if (strcmp(argv[i], "ne") == 0) {
	wavePtr->anchor = 1;
      } else if (strcmp(argv[i], "nw") == 0) {
	wavePtr->anchor = 7;
      } else if (strcmp(argv[i], "n") == 0) {
	wavePtr->anchor = 0;
      } else if (strcmp(argv[i], "e") == 0) {
	wavePtr->anchor = 2;
      } else if (strcmp(argv[i], "se") == 0) {
	wavePtr->anchor = 3;
      } else if (strcmp(argv[i], "sw") == 0) {
	wavePtr->anchor = 5;
      } else if (strcmp(argv[i], "s") == 0) {
	wavePtr->anchor = 4;
      } else if (strcmp(argv[i], "w") == 0) {
	wavePtr->anchor = 6;
      } else if (strncmp(argv[i], "center", strlen(argv[i])) == 0) {
	wavePtr->anchor = 8;
      }
      break;
    }
  }
#endif

  if (OptSpecified(OPTION_SOUND)) {
    if (wavePtr->newSoundName == NULL) {
      wavePtr->sound = NULL;
      if (wavePtr->id) Snack_RemoveCallback(s, wavePtr->id);
      wavePtr->id = 0;
    } else {
      if ((s = Snack_GetSound(interp, wavePtr->newSoundName)) == NULL) {
	return TCL_ERROR;
      }
      if (s->storeType == SOUND_IN_CHANNEL) {
	Tcl_AppendResult(interp, wavePtr->newSoundName, 
			 " can not be linked to a channel", (char *) NULL);
	return TCL_ERROR;
      }
      if (s->storeType == SOUND_IN_FILE) {
	s->itemRefCnt++;
      }
      wavePtr->sound = s;
      if (wavePtr->soundName == NULL) {
	wavePtr->soundName = ckalloc(strlen(wavePtr->newSoundName)+1);
	strcpy(wavePtr->soundName, wavePtr->newSoundName);
      }
      if (strcmp(wavePtr->soundName, wavePtr->newSoundName) != 0) {
	Sound *t = Snack_GetSound(interp, wavePtr->soundName);
	ckfree(wavePtr->soundName);
	wavePtr->soundName = ckalloc(strlen(wavePtr->newSoundName)+1);
	strcpy(wavePtr->soundName, wavePtr->newSoundName);
	wavePtr->nPoints = 0;
	wavePtr->ssmp    = 0;
	wavePtr->esmp    = -1;
	Snack_RemoveCallback(t, wavePtr->id);
	wavePtr->id = 0;
      }
      
      if (!wavePtr->id)
	wavePtr->id = Snack_AddCallback(s, UpdateWave, (int *)wavePtr);
      
      wavePtr->blocks = s->blocks;
      wavePtr->bufPos = s->length;
      wavePtr->sampfreq = s->sampfreq;
      wavePtr->sampformat = s->sampformat;
      wavePtr->nchannels = s->nchannels;
      wavePtr->storeType = s->storeType;
    }
  }
  wavePtr->esmp = wavePtr->endSmp;

  if (wavePtr->endSmp < 0)
    wavePtr->esmp = wavePtr->bufPos - 1;

  if (wavePtr->endSmp > wavePtr->bufPos - 1)
    wavePtr->esmp = wavePtr->bufPos - 1;

  if (wavePtr->startSmp > wavePtr->endSmp && wavePtr->endSmp >= 0)
    wavePtr->startSmp = wavePtr->endSmp;

  if (wavePtr->startSmp < 0)
    wavePtr->startSmp = 0;

  wavePtr->ssmp = wavePtr->startSmp;

  if (wavePtr->ssmp > wavePtr->esmp)
    wavePtr->ssmp = wavePtr->esmp;

  if (OptSpecified(OPTION_PIXPSEC) && OptSpecified(OPTION_WIDTH)) {
    wavePtr->mode = CONF_WIDTH_PPS;
  }
  else if (OptSpecified(OPTION_PIXPSEC)) {
    wavePtr->mode = CONF_PPS;
  }
  else if (OptSpecified(OPTION_WIDTH)) {
    wavePtr->mode = CONF_WIDTH;
  }
  
  if (wavePtr->mode == CONF_WIDTH_PPS) {
    if (OptSpecified(OPTION_END) && !OptSpecified(OPTION_START)) {
      wavePtr->ssmp = (int) (wavePtr->esmp - wavePtr->width *
			     wavePtr->sampfreq / wavePtr->pixpsec);
    } else {
      wavePtr->esmp = (int) (wavePtr->ssmp + wavePtr->width *
			     wavePtr->sampfreq / wavePtr->pixpsec);
    }
    wavePtr->nPoints = 2 * wavePtr->width;
  }
  else if (wavePtr->mode == CONF_PPS) {
    wavePtr->nPoints = 2 * (int)((wavePtr->esmp - wavePtr->ssmp) * 
				 wavePtr->pixpsec / wavePtr->sampfreq);
    wavePtr->width = wavePtr->nPoints / 2;
  }
  else if (wavePtr->mode == CONF_WIDTH) {
    wavePtr->nPoints = 2 * wavePtr->width;
    if (wavePtr->esmp != wavePtr->ssmp) {
      wavePtr->pixpsec = (float) wavePtr->width * wavePtr->sampfreq /
	(wavePtr->esmp - wavePtr->ssmp);
    }
  }

  if (OptSpecified(OPTION_CHANNEL)) {
    if (GetChannel(interp, wavePtr->channelStr, wavePtr->nchannels, 
		   &wavePtr->channelSet) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  wavePtr->channel = wavePtr->channelSet;
  if (wavePtr->nchannels == 1) {
    wavePtr->channel = 0;
  }

  if (OptSpecified(OPTION_PROGRESS)) {
    wavePtr->cmdPtr = Tcl_NewStringObj(wavePtr->progressCmd, -1);
    Tcl_IncrRefCount(wavePtr->cmdPtr);
  }

  if (wavePtr->subSampleInt == 0) {
    if (wavePtr->esmp - wavePtr->ssmp > NSAMPLES) { 
      wavePtr->subSample = (wavePtr->esmp - wavePtr->ssmp) / NSAMPLES;
    } else {
      wavePtr->subSample = 1;
    }
  } else {
    wavePtr->subSample = wavePtr->subSampleInt;
  }
  if (wavePtr->fg == NULL) {
    newGC = None;
  } else {
    gcValues.foreground = wavePtr->fg->pixel;
    gcValues.line_width = 1;
    mask = GCForeground|GCLineWidth;
    if (wavePtr->fillStipple != None) {
      gcValues.stipple = wavePtr->fillStipple;
      gcValues.fill_style = FillStippled;
      mask |= GCStipple|GCFillStyle;
    }
    newGC = Tk_GetGC(tkwin, mask, &gcValues);
    gcValues.line_width = 0;
  }
  if (wavePtr->gc != None) {
    Tk_FreeGC(Tk_Display(tkwin), wavePtr->gc);
  }
  wavePtr->gc = newGC;
  
  ComputeWaveBbox(canvas, wavePtr);

  if (ComputeWaveCoords(itemPtr) != TCL_OK) {
    return TCL_ERROR;
  }

  if (wavePtr->debug == 1)
    Snack_WriteLogInt("Exit ConfigureWave", wavePtr->nPoints);

  return TCL_OK;
}

static void
DeleteWave(Tk_Canvas canvas, Tk_Item *itemPtr, Display *display)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;

  if ((wavePtr->id) &&
      (Snack_GetSound(wavePtr->interp, wavePtr->soundName) != NULL)) {
    Snack_RemoveCallback(wavePtr->sound, wavePtr->id);
  }
  
  ckfree(wavePtr->soundName);

  if (wavePtr->coords != NULL) ckfree((char *) wavePtr->coords);

  if (wavePtr->fg != NULL) Tk_FreeColor(wavePtr->fg);

  if (wavePtr->fillStipple != None) Tk_FreeBitmap(display, wavePtr->fillStipple);

  if (wavePtr->gc != None) Tk_FreeGC(display, wavePtr->gc);

  if (wavePtr->preWI != NULL) ckfree((char *)wavePtr->preWI);

  if (wavePtr->preSound != NULL) Snack_DeleteSound(wavePtr->preSound);

  if (wavePtr->sound->storeType == SOUND_IN_FILE) {
    wavePtr->sound->itemRefCnt--;
  }

  if (wavePtr->cmdPtr != NULL) Tcl_DecrRefCount(wavePtr->cmdPtr);
}

static void
ComputeWaveBbox(Tk_Canvas canvas, WaveItem *wavePtr)
{
  int width = wavePtr->width;
  int height = wavePtr->height;
  int x = (int) (wavePtr->x + ((wavePtr->x >= 0) ? 0.5 : - 0.5));
  int y = (int) (wavePtr->y + ((wavePtr->y >= 0) ? 0.5 : - 0.5));
  
  switch (wavePtr->anchor) {
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

  wavePtr->header.x1 = x;
  wavePtr->header.y1 = y;
  wavePtr->header.x2 = x + width;
  wavePtr->header.y2 = y + height;
}

static void
DisplayWave(Tk_Canvas canvas, Tk_Item *itemPtr, Display *display,
	    Drawable drawable, int x, int y, int width, int height)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  double *coords;
  int i, nPoints = width * 2;
  XPoint *wpts = (XPoint *) ckalloc((unsigned)((nPoints+2) * sizeof(XPoint)));
  XPoint *p = wpts;
  int xo = wavePtr->header.x1;
  int yo = wavePtr->header.y1;
  int dx = max(x - xo, 0);

  if (wavePtr->debug == 1) Snack_WriteLogInt("Enter DisplayWave", nPoints);

  if (wavePtr->height == 0) return;

  if (wavePtr->gc == None) return;

  if (wavePtr->fillStipple != None)
    Tk_CanvasSetStippleOrigin(canvas, wavePtr->gc);
  
  if (dx + width > wavePtr->nPoints / 2) {
    nPoints = wavePtr->nPoints - dx * 2;
  }
  if (dx > 0) {
    coords = &wavePtr->coords[dx * 4 - 2];
    if (nPoints < wavePtr->nPoints - dx * 2) nPoints++;
    nPoints++;
  } else {
    coords = &wavePtr->coords[dx * 4];
  }
  for (i = 0; i < nPoints; i++) {
    Tk_CanvasDrawableCoords(canvas, xo + coords[0], yo + coords[1],
			    &p->x, &p->y);
    coords += 2;
    p++;
  }

  XDrawLines(display, drawable, wavePtr->gc, wpts, nPoints, CoordModeOrigin);

  if (wavePtr->zeroLevel) {
    Tk_CanvasDrawableCoords(canvas, (double) xo, 
			    (double) (yo + wavePtr->height / 2),
			    &wavePtr->fpts[0].x, &wavePtr->fpts[0].y);
    Tk_CanvasDrawableCoords(canvas, (double) (xo + wavePtr->width - 1),
			    (double) (yo + wavePtr->height / 2),
			    &wavePtr->fpts[1].x, &wavePtr->fpts[1].y);
    XDrawLines(display, drawable, wavePtr->gc, wavePtr->fpts, 2, CoordModeOrigin);
  }

  if (wavePtr->frame) {
    Tk_CanvasDrawableCoords(canvas, (double) xo, (double) yo,
			    &wavePtr->fpts[0].x, &wavePtr->fpts[0].y);
    Tk_CanvasDrawableCoords(canvas, (double) (xo + wavePtr->width - 1), 
			    (double) yo,
			    &wavePtr->fpts[1].x, &wavePtr->fpts[1].y);
    Tk_CanvasDrawableCoords(canvas, (double) (xo + wavePtr->width - 1),
			    (double) (yo + wavePtr->height - 1),
			    &wavePtr->fpts[2].x, &wavePtr->fpts[2].y);
    Tk_CanvasDrawableCoords(canvas, (double) xo, 
			    (double) (yo + wavePtr->height - 1),
			    &wavePtr->fpts[3].x, &wavePtr->fpts[3].y);
    Tk_CanvasDrawableCoords(canvas, (double) xo, (double) yo,
			    &wavePtr->fpts[4].x, &wavePtr->fpts[4].y);
    XDrawLines(display, drawable, wavePtr->gc, wavePtr->fpts, 5, CoordModeOrigin);
  }

  ckfree((char *) wpts);

  if (wavePtr->debug == 1) Snack_WriteLog("Exit DisplayWave\n");
}

static double
WaveToPoint(Tk_Canvas canvas, Tk_Item *itemPtr, double *coords)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  double dx = 0.0, dy = 0.0;
  double x1 = wavePtr->header.x1;
  double y1 = wavePtr->header.y1;
  double x2 = wavePtr->header.x2;
  double y2 = wavePtr->header.y2;
  
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
WaveToArea(Tk_Canvas canvas, Tk_Item *itemPtr, double *rectPtr)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;

  if ((rectPtr[2] <= wavePtr->header.x1) ||
      (rectPtr[0] >= wavePtr->header.x2) ||
      (rectPtr[3] <= wavePtr->header.y1) ||
      (rectPtr[1] >= wavePtr->header.y2))
    return -1;

  if ((rectPtr[0] <= wavePtr->header.x1) &&
      (rectPtr[1] <= wavePtr->header.y1) &&
      (rectPtr[2] >= wavePtr->header.x2) &&
      (rectPtr[3] >= wavePtr->header.y2))
    return 1;
 
  return 0;
}

static void
ScaleWave(Tk_Canvas canvas, Tk_Item *itemPtr, double ox, double oy,
	  double sx, double sy)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  double *coords = wavePtr->coords;
  int i;
  
  for (i = 0; i < wavePtr->nPoints; i++) {
    coords[0] = ox + sx * (coords[0] - ox);
    coords[1] = oy + sy * (coords[1] - oy);
    coords += 2;
  }
  wavePtr->width  = (int) (sx * wavePtr->width) + 1;
  wavePtr->height = (int) (sy * wavePtr->height);
  if (wavePtr->bufPos > 0)
    wavePtr->pixpsec = (float) wavePtr->width * wavePtr->sampfreq /
      wavePtr->bufPos;

  ComputeWaveBbox(canvas, wavePtr);
}

static void
TranslateWave(Tk_Canvas canvas, Tk_Item *itemPtr, double dx, double dy)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;

  wavePtr->x += dx;
  wavePtr->y += dy;
  ComputeWaveBbox(canvas, wavePtr);
}

static int
WaveToPS(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, int prepass)
{
  WaveItem *wavePtr = (WaveItem *) itemPtr;
  double  *coords = wavePtr->coords;
  int     nPoints = wavePtr->nPoints;
  char buffer[100];
  int xo = wavePtr->header.x1;
  int yo = wavePtr->header.y1;

  if (wavePtr->fg == NULL) {
    return TCL_OK;
  }

  Tcl_AppendResult(interp, "%% WAVE BEGIN\n", (char *) NULL);

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

  if (wavePtr->zeroLevel) {
    sprintf(buffer, "%.15g %.15g moveto\n", (double) xo,
	    Tk_CanvasPsY(canvas, (double) (yo + wavePtr->height / 2)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo + wavePtr->width - 1,
	    Tk_CanvasPsY(canvas, (double) (yo + wavePtr->height / 2)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);
  }

  if (wavePtr->frame) {
    sprintf(buffer, "%.15g %.15g moveto\n", (double) xo, Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo + wavePtr->width - 1,
	    Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo + wavePtr->width - 1,
	    Tk_CanvasPsY(canvas, (double) (yo + wavePtr->height - 1)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo,
	    Tk_CanvasPsY(canvas, (double) (yo + wavePtr->height - 1)));
    Tcl_AppendResult(interp, buffer, (char *) NULL);

    sprintf(buffer, "%.15g %.15g lineto\n", (double) xo,
	    Tk_CanvasPsY(canvas, (double) yo));
    Tcl_AppendResult(interp, buffer, (char *) NULL);
  }

  Tcl_AppendResult(interp, "1 setlinewidth\n", (char *) NULL);
  Tcl_AppendResult(interp, "0 setlinecap\n0 setlinejoin\n", (char *) NULL);
  if (Tk_CanvasPsColor(interp, canvas, wavePtr->fg) != TCL_OK) {
    return TCL_ERROR;
  };
  if (wavePtr->fillStipple != None) {
    Tcl_AppendResult(interp, "StrokeClip ", (char *) NULL);
    if (Tk_CanvasPsStipple(interp, canvas, wavePtr->fillStipple) != TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    Tcl_AppendResult(interp, "stroke\n", (char *) NULL);
  }
  
  Tcl_AppendResult(interp, "%% WAVE END\n", (char *) NULL);

  return TCL_OK;
}

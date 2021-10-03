/*
 * Copyright (C) 2000 Claude Barras
 * contribution to the Snack sound extension for Tcl/Tk
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "snack.h"

#if defined Linux || defined WIN || defined _LITTLE_ENDIAN
#  define LE
#endif

extern int useOldObjAPI;

typedef struct {
   short max;
   short min;
} Maxmin;

/* --------------------------------------------------------------------- */

static short GetShortSample(Sound *s, long i, int c) {
   if (i >= Snack_GetLength(s) || s->storeType == SOUND_IN_CHANNEL)
      return 0;
   i = i * Snack_GetNumChannels(s) + c;
   if (s->storeType == SOUND_IN_MEMORY) {
      switch (Snack_GetSampleFormat(s)) {
      case LIN16:
	 return SSAMPLE(s, i);
      case ALAW:
	 return Snack_Alaw2Lin(UCSAMPLE(s, i));
      case MULAW:
	 return Snack_Mulaw2Lin(UCSAMPLE(s, i));
      case LIN8OFFSET:
	 return ((char)UCSAMPLE(s, i) ^ 128) << 8;
      case LIN8:
	 return UCSAMPLE(s, i) << 8;
      }
   } else {
      if (s->linkInfo.linkCh == NULL) {
	 OpenLinkedFile(s, &s->linkInfo);
      }
      switch (Snack_GetSampleFormat(s)) {
      case LIN16:
	 return GetSample(&s->linkInfo, i);
      case ALAW:
	 return Snack_Alaw2Lin((unsigned char)GetSample(&s->linkInfo, i));
      case MULAW:
	 return Snack_Mulaw2Lin((unsigned char)GetSample(&s->linkInfo, i));
      case LIN8OFFSET:
	 return (GetSample(&s->linkInfo, i) ^ 128) << 8;
      case LIN8:
	 return GetSample(&s->linkInfo, i) << 8;
      }
   }
   return 0;
}

/* --------------------------------------------------------------------- */

int shapeCmd(Sound *s, Tcl_Interp *interp, int objc,
	     Tcl_Obj *CONST objv[])
{
  int arg, width = 0, pps = 0, startpos = 0, endpos = -1, check = 0;
  int byteOrder = SNACK_NATIVE;
  int sampformat, sampsize;
  Sound *shp = NULL, *preshp = NULL;
  long k0, k1;
  int i, c, first, nc, nbytes = 0, mn, mx;
  double begin, len, Fe, hRatio, pos;
  Maxmin *q = NULL, *p = NULL;
  Tcl_Obj *resObj = NULL;
  char *string;

  if (objc < 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "shape ?sound? ?options?");
    return TCL_ERROR;
  }

  /* Decide where to store the 'shape' */
  string = Tcl_GetStringFromObj(objv[2], NULL);    
  if (string[0] != '-') {
    if ((shp = Snack_GetSound(interp, string)) == NULL) {
      return TCL_ERROR;
    }
    if (shp == s) {
      Tcl_AppendResult(interp, "source and target must differ", NULL);
      return TCL_ERROR;
    }
  }

  if (shp) {

    /* Store shape into sound */
    static char *subOptionStrings[] = {
      "-start", "-end", "-pixelspersecond", "-format", "-check", NULL
    };
    enum subOptions {
      START, END, PPS, FORMAT, CHECK
    };
    int index;
      
    /* default values for the sound target case */
    pps = 100;
    sampformat = Snack_GetSampleFormat(s);
    sampsize = Snack_GetBytesPerSample(s);

    for (arg = 3; arg < objc; arg += 2) {
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
      case PPS:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &pps) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case FORMAT:
	{
	  if (GetFormat(interp, objv[arg+1], &sampformat, &sampsize) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case CHECK:
	{
	  if (Tcl_GetBooleanFromObj(interp, objv[arg+1], &check) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      }
    }
  } else {

    /* Return shape as binary data */
    static char *subOptionStrings[] = {
      "-start", "-end", "-width", "-pixelspersecond",
      "-shape", "-byteorder", NULL
    };
    enum subOptions {
      START, END, WIDTH, PPS, SHAPE, BYTEORDER
    };
    int index;
    
    for (arg = 2; arg < objc; arg += 2) {

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
      case WIDTH:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &width) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case PPS:
	{
	  if (Tcl_GetIntFromObj(interp, objv[arg+1], &pps) != TCL_OK)
	    return TCL_ERROR;
	  break;
	}
      case SHAPE:
	{
	  int nchar = 0;
	  char *str = Tcl_GetStringFromObj(objv[arg+1], &nchar);
	  if (nchar > 0 && (preshp = Snack_GetSound(interp, str)) == NULL) {
	    return TCL_ERROR;
	  }
	  break;
	}
      case BYTEORDER:
	{
	  int length;
	  char *str = Tcl_GetStringFromObj(objv[arg+1], &length);
	  
	  if (strncasecmp(str, "littleEndian", length) == 0) {
	    byteOrder = SNACK_LITTLEENDIAN;
	  } else if (strncasecmp(str, "bigEndian", length) == 0) {
	    byteOrder = SNACK_BIGENDIAN;
	  } else {
	    Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
	    return TCL_ERROR;
	  }
	  break;
	}
      }
    }
  }

  /* Characteristics of 'shaped' sound */
  nc = Snack_GetNumChannels(s);
  Fe = Snack_GetFrequency(s);

  /* Adjust boundaries to fit the sound and satisfy the constraint: */
  /*    len = (endpos-startpos+1)/Fe = width/pps */
  if (startpos < 0) startpos = 0;
  if (width > 0 && pps > 0) {
    endpos = startpos + (int) (Fe * width / (float) pps - 1);
  }
  if (endpos >= (Snack_GetLength(s) - 1) || endpos == -1)
    endpos = Snack_GetLength(s) - 1;
  begin = (double) startpos / Fe;
  len = (double) (endpos - startpos + 1) / Fe;
  if (width <= 0 && pps > 0) {
     width = (int) ceil(len * pps);
  }
  if (width <= 0 || len <= 0) {
     Tcl_SetResult(interp, "Bad boundaries for shape", TCL_STATIC);
     return TCL_ERROR;
  }

  /* Only checks if existing shape seems compatible with sound then exits */
  if (shp && check) {
     float shpLen = (float) Snack_GetLength(shp) / Snack_GetFrequency(shp);
     if (Snack_GetNumChannels(shp) == 2 * nc
	 && fabs(len - shpLen) < 0.05 * len
	 && Snack_GetFrequency(shp) < 1000) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
     } else {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
     }
     return TCL_OK;
  }

  /* Try to use precomputed shape instead of original sound */
  if (preshp != NULL
      && Snack_GetNumChannels(preshp) == 2 * nc
      && Snack_GetFrequency(preshp) * len / width > 2.0) {
     Fe = Snack_GetFrequency(preshp);
  } else {
     Fe = Snack_GetFrequency(s);
     preshp = NULL;
  }

  /* number of samples per point */
  if (pps > 0) {
    hRatio = Fe / pps;
  } else {
    hRatio = Fe * len / width;
  }

  /* round first sample to be a multiple of hRatio */
  /* in order to avoid distortions during scrolling */
  pos = floor(begin * Fe / hRatio) * hRatio;

  if (shp) {

    /* update shape parameters and get free space into sound target */
    Tcl_Obj *empty = Tcl_NewStringObj("",-1);
    if (Snack_GetSoundStatus(shp) != IDLE) {
      Snack_StopSound(shp, interp);
    }
    SetFcname(shp, interp, empty);
    Tcl_DecrRefCount(empty);
    shp->storeType = SOUND_IN_MEMORY;
    Snack_SetFrequency(shp, pps);
    Snack_SetSampleFormat(shp, sampformat);
    Snack_SetBytesPerSample(shp, sampsize);
    Snack_SetNumChannels(shp, 2 * nc);
    Snack_SetLength(shp, (int) ceil((endpos+1-pos)/hRatio));
    if (Snack_ResizeSoundStorage(shp, Snack_GetLength(shp)) != TCL_OK) {
      return TCL_ERROR;
    }

  } else {

    /* Get memory for binary shape */
    resObj = Tcl_NewObj();
    nbytes = sizeof(Maxmin) * nc * width;
    if (useOldObjAPI) {
      Tcl_SetObjLength(resObj, nbytes);
      p = (Maxmin *) resObj->bytes;
    } else {
#ifdef TCL_81_API
      p = (Maxmin *) Tcl_SetByteArrayLength(resObj, nbytes);
#endif
    }
  }

  /* compute min/max for each point */
  q = (Maxmin *) Tcl_Alloc(sizeof(Maxmin) * nc);
  for (i=0; i<width; i++) {
     k0 = (long) pos; pos += hRatio; k1 = (long) pos;
     first = 1;
     while (k0 < k1 && k0 <= endpos) {
       for (c=0; c<nc; c++) {
	 if (preshp) {
	   mx = GetShortSample(preshp, k0, 2*c);
	   mn = GetShortSample(preshp, k0, 2*c+1);
	 } else {
	   mn = mx = GetShortSample(s, k0, c);
	 }
	 if (first || (mn < q[c].min))
	   q[c].min = mn; 
	 if (first || (mx > q[c].max))
	   q[c].max = mx; 
	 }
       first = 0;
       k0++;
     }
     if (first && k0 > endpos) break;
     if (shp) {
       for (c=0; c<nc; c++) {
	 Snack_SetSample(shp, 2*c,   i, q[c].max);
	 Snack_SetSample(shp, 2*c+1, i, q[c].min);
       }
     } else {
       for (c=0; c<nc; c++) {
	 p[c+i*nc] = q[c];
       }
     }
  }
  Tcl_Free((char *)q);

  /* return result */
  if (shp) {
    Snack_UpdateExtremes(shp, 0, Snack_GetLength(shp), SNACK_NEW_SOUND);
  } else {

    /* Use correct byte order */
#ifdef LE
    if (byteOrder == SNACK_BIGENDIAN) {
#else
    if (byteOrder == SNACK_LITTLEENDIAN) {
#endif
      /*swab(p, p, nbytes);*/
      for (i = 0; i < (int) (nbytes / sizeof(short)); i++)
	((short *)p)[i] = Snack_SwapShort(((short *)p)[i]);
    }
    
    Tcl_SetObjResult( interp, resObj);
  }
  return TCL_OK;
}

/* --------------------------------------------------------------------- */

int dataSamplesCmd(Sound *s, Tcl_Interp *interp, int objc,
            Tcl_Obj *CONST objv[])
{
  int arg, startpos = 0, endpos = -1;
  int byteOrder = SNACK_NATIVE;
  int pos, i, c, nbytes;
  Tcl_Obj *resObj = NULL;
  short *p = NULL;

  /* Get options */
  for (arg = 2; arg < objc; arg += 2) {
    static char *subOptionStrings[] = {
       "-start", "-end", "-byteorder",  NULL
    };
    enum subOptions {
       START, END, BYTEORDER
    };
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
    case BYTEORDER:
      {
	int length;
	char *str = Tcl_GetStringFromObj(objv[arg+1], &length);

	if (strncasecmp(str, "littleEndian", length) == 0) {
	   byteOrder = SNACK_LITTLEENDIAN;
	} else if (strncasecmp(str, "bigEndian", length) == 0) {
	   byteOrder = SNACK_BIGENDIAN;
	} else {
	  Tcl_AppendResult(interp, "-byteorder option should be bigEndian or littleEndian", NULL);
	  return TCL_ERROR;
	}
	break;
      }
    }
  }
   
  /* Adjust boundaries */
  if (startpos < 0) startpos = 0;
  if (endpos >= (Snack_GetLength(s) - 1) || endpos == -1)
    endpos = Snack_GetLength(s) - 1;
  if (startpos > endpos) return TCL_OK;

  /* Get memory */
  resObj = Tcl_NewObj();
  nbytes = sizeof(short) * Snack_GetNumChannels(s) * (endpos - startpos + 1);
  if (useOldObjAPI) {
    Tcl_SetObjLength(resObj, nbytes);
    p = (short *) resObj->bytes;
  } else {
#ifdef TCL_81_API
     p = (short *) Tcl_SetByteArrayLength(resObj, nbytes);
#endif
  }

  /* Put samples into array as binary shorts */
  i = 0;
  for (pos = startpos; pos <= endpos; pos++) {
    for (c = 0; c < Snack_GetNumChannels(s); c++) {
      p[i++] = GetShortSample(s, pos, c);
    }
  }

  /* Use correct byte order */
#ifdef LE
  if (byteOrder == SNACK_BIGENDIAN) {
#else
  if (byteOrder == SNACK_LITTLEENDIAN) {
#endif
    /*swab(p, p, nbytes);*/
    for (i = 0; i < (int) (nbytes / sizeof(short)); i++)
      p[i] = Snack_SwapShort(p[i]);
  }

  Tcl_SetObjResult( interp, resObj);
  return TCL_OK;
}

/* --------------------------------------------------------------------- */

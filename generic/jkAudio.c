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

extern int rop, wop;
extern double startTime;
extern struct jkQueuedSound *soundQueue;

char defaultOutDevice[MAX_DEVICE_NAME_LENGTH];
char defaultInDevice[MAX_DEVICE_NAME_LENGTH];

char *
SnackStrDup(const char *str) {
  char *new = ckalloc(strlen(str)+1);

  if (new) {
    strncpy(new, str, strlen(str)+1);
  }

  return new;
}

static int
outDevicesCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int i, n;
  char *arr[MAX_NUM_DEVICES];
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  n = SnackGetOutputDevices(arr, MAX_NUM_DEVICES);

  for (i = 0; i < n; i++) {
    Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj(arr[i], -1));
    ckfree(arr[i]);
  }

  Tcl_SetObjResult(interp, list);

  return TCL_OK;
}

static int
inDevicesCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int i, n;
  char *arr[MAX_NUM_DEVICES];
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  n = SnackGetInputDevices(arr, MAX_NUM_DEVICES);

  for (i = 0; i < n; i++) {
    Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj(arr[i], -1));
    ckfree(arr[i]);
  }

  Tcl_SetObjResult(interp, list);

  return TCL_OK;
}

static int
selectOutCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (objc == 3) {
    strncpy(defaultOutDevice, Tcl_GetStringFromObj(objv[2], NULL),
	    MAX_DEVICE_NAME_LENGTH);
    defaultOutDevice[MAX_DEVICE_NAME_LENGTH-1] = '\0';
  } else {
    Tcl_WrongNumArgs(interp, 1, objv, "selectOutput device");
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

static int
selectInCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if (objc == 3) {
    strncpy(defaultInDevice, Tcl_GetStringFromObj(objv[2], NULL),
	    MAX_DEVICE_NAME_LENGTH);
    defaultInDevice[MAX_DEVICE_NAME_LENGTH-1] = '\0';
  } else {
    Tcl_WrongNumArgs(interp, 1, objv, "selectInput device");
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

static int
formatsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackAudioGetFormats(defaultOutDevice, tmpstr, QUERYBUFSIZE);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(tmpstr, -1));

  return TCL_OK;
}

static int
frequenciesCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  char tmpstr[QUERYBUFSIZE];

  SnackAudioGetFrequencies(defaultOutDevice, tmpstr, QUERYBUFSIZE);
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

  entryPtr = Tcl_FirstHashEntry(p->sound->soundTable, &hashSearch);

  if (p->sound != (Sound *) Tcl_GetHashValue(entryPtr)) {
    entryPtr = Tcl_NextHashEntry(&hashSearch);
  }
  res = Tcl_GetHashKey(p->sound->soundTable, entryPtr);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(res, -1));

  return TCL_OK;
}

#define NAUDIOCOMMANDS   10
#define MAXAUDIOCOMMANDS 20

int nAudioCommands   = NAUDIOCOMMANDS;
int maxAudioCommands = MAXAUDIOCOMMANDS;

char *audioCmdNames[MAXAUDIOCOMMANDS] = {
  "outputDevices",
  "inputDevices",
  "selectOutput",
  "selectInput",
  "formats",
  "frequencies",
  "active",
  "play_gain",
  "record_gain",
  "elapsedTime",
  "currentSound",
  NULL
};

/* NOTE: NAUDIOCOMMANDS needs updating when new commands are added. */

audioCmd *audioCmdProcs[MAXAUDIOCOMMANDS] = {
  outDevicesCmd,
  inDevicesCmd,
  selectOutCmd,
  selectInCmd,
  formatsCmd,
  frequenciesCmd,
  activeCmd,
  play_gainCmd,
  record_gainCmd,
  elapsedTimeCmd,
  currentSoundCmd,
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

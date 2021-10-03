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

#include "snack.h"
#include "tk.h"
#include "jkCanvItems.h"

#if defined(__WIN32__)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN

BOOL APIENTRY
DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
  return TRUE;
}
#endif

extern Tk_ItemType snackWaveType;
extern Tk_ItemType snackSpectrogramType;
extern Tk_ItemType snackSectionType;
extern Tk_CustomOption waveTagsOption;
extern Tk_CustomOption spegTagsOption;
extern Tk_CustomOption sectTagsOption;

#define play_width 19
#define play_height 19
static unsigned char play_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
   0x78, 0x00, 0x00, 0xf8, 0x01, 0x00, 0xf8, 0x07, 0x00, 0xf8, 0x1f, 0x00,
   0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x1f, 0x00, 0xf8, 0x07, 0x00,
   0xf8, 0x01, 0x00, 0x78, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define rec_width 19
#define rec_height 19
static unsigned char rec_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x00,
   0xe0, 0x1f, 0x00, 0xf0, 0x3f, 0x00, 0xf0, 0x3f, 0x00, 0xf8, 0x7f, 0x00,
   0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf0, 0x3f, 0x00,
   0xf0, 0x3f, 0x00, 0xe0, 0x1f, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define stop_width 19
#define stop_height 19
static unsigned char stop_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00,
   0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00,
   0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00,
   0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define pause_width 19
#define pause_height 19
static unsigned char pause_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7c, 0x00,
   0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00,
   0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00,
   0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0xf8, 0x7c, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

extern Tcl_Channel snackDebugChannel;

int
Snack_InitCmd(ClientData cdata, Tcl_Interp *interp, int objc,
	      Tcl_Obj *CONST objv[])
{
  int len;
  char *str;

  if (objc == 2) {
    if (Tcl_IsSafe(interp)) {
      Tcl_AppendResult(interp, "can't open log file in a safe interpreter",
		       (char *) NULL);
      return TCL_ERROR;
    }
    str = Tcl_GetStringFromObj(objv[1], &len);
    snackDebugChannel = Tcl_OpenFileChannel(interp, str, "w", 420);
  }

  return TCL_OK;
}

Tcl_HashTable jkSoundTable;

#ifdef SNACK_CSLU_TOOLKIT
extern int fromCSLUshWaveCmd(Sound *s, Tcl_Interp *interp, int objc,
			     Tcl_Obj *CONST objv[]);
extern int toCSLUshWaveCmd(Sound *s, Tcl_Interp *interp, int objc,
			   Tcl_Obj *CONST objv[]);
#endif

int useOldObjAPI = 0;

extern SnackStubs *snackStubs;

int
Snack_Init(Tcl_Interp *interp)
{
  Tcl_CmdInfo infoPtr;
  char *version;

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8", 0) == NULL) {
    return TCL_ERROR;
  }
#endif

  version = Tcl_GetVar(interp, "tcl_version",
		       (TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG));
  
  if (strcmp(version, "8.0") == 0) {
    useOldObjAPI = 1;
  }

#ifdef TCL_81_API
  if (Tcl_PkgProvideEx(interp, "snack", SNACK_VERSION,
		       (ClientData) &snackStubs) != TCL_OK) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgProvide(interp, "snack", SNACK_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
#endif

  SnackMixerOpen();

  if (Tcl_GetCommandInfo(interp, "button", &infoPtr) != 0) {
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, "8", 0) == NULL) {
      return TCL_ERROR;
    }
#endif
    Tk_CreateItemType(&snackWaveType);
    Tk_CreateItemType(&snackSpectrogramType);
    Tk_CreateItemType(&snackSectionType);
    Tk_DefineBitmap(interp, Tk_GetUid("play"),   (char *) play_bits,
		    play_width, play_height);
    Tk_DefineBitmap(interp, Tk_GetUid("record"), (char *) rec_bits,
		    rec_width, rec_height);
    Tk_DefineBitmap(interp, Tk_GetUid("stop"),   (char *) stop_bits,
		    stop_width, stop_height);
    Tk_DefineBitmap(interp, Tk_GetUid("pause"),  (char *) pause_bits,
		    pause_width, pause_height);
    waveTagsOption.parseProc = Tk_CanvasTagsParseProc;
    waveTagsOption.printProc = Tk_CanvasTagsPrintProc;
    spegTagsOption.parseProc = Tk_CanvasTagsParseProc;
    spegTagsOption.printProc = Tk_CanvasTagsPrintProc;
    sectTagsOption.parseProc = Tk_CanvasTagsParseProc;
    sectTagsOption.printProc = Tk_CanvasTagsPrintProc;
  }

  Tcl_CreateObjCommand(interp, "sound", Snack_SoundCmd,
		       NULL, (Tcl_CmdDeleteProc *)NULL);
  
  Tcl_CreateObjCommand(interp, "audio", Snack_AudioCmd,
		       NULL, Snack_AudioDeleteCmd);

  Tcl_CreateObjCommand(interp, "initSnack", 
		       (Tcl_ObjCmdProc*) Snack_InitCmd,
		       NULL, (Tcl_CmdDeleteProc *)NULL);

  snackDebugChannel = Tcl_GetStdChannel(TCL_STDERR);

  Tcl_SetVar(interp, "snack_patchLevel", SNACK_PATCH_LEVEL, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "snack_version",    SNACK_VERSION,     TCL_GLOBAL_ONLY);
  
  AddSnackNativeFormats();

  Tcl_InitHashTable(&jkSoundTable, TCL_STRING_KEYS);

  Tcl_CreateExitHandler(Snack_ExitProc, (ClientData) NULL);

#ifdef SNACK_CSLU_TOOLKIT
  Snack_AddSubCmd(SNACK_SOUND_CMD, "fromCSLUshWave",
		  (Snack_CmdProc *) fromCSLUshWaveCmd, NULL);
  Snack_AddSubCmd(SNACK_SOUND_CMD, "toCSLUshWave",
		  (Snack_CmdProc *) toCSLUshWaveCmd, NULL);
#endif

  return TCL_OK;
}

int
Snack_SafeInit(Tcl_Interp *interp)
{
  return Snack_Init(interp);
}

/*
static double _t0;

void
TimerStart() {
  _t0 = SnackCurrentTime();
}

void
TimerStop() {
  char buf[20];
  sprintf(buf, "%f", SnackCurrentTime()-_t0);
  Tcl_Write(snackDebugChannel, buf, strlen(buf));
  Tcl_Write(snackDebugChannel, "\n", 1);
  Tcl_Flush(snackDebugChannel);
}
*/

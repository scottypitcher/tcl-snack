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
#include "tcl.h"

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
Sound_Init(Tcl_Interp *interp)
{
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
  if (Tcl_PkgProvideEx(interp, "sound", SNACK_VERSION,
		       (ClientData) &snackStubs) != TCL_OK) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgProvide(interp, "sound", SNACK_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
#endif

  SnackMixerOpen();
  
  Tcl_CreateObjCommand(interp, "sound", Snack_SoundCmd,
		       NULL, (Tcl_CmdDeleteProc *)NULL);
  
  Tcl_CreateObjCommand(interp, "audio", Snack_AudioCmd,
		       NULL, Snack_AudioDeleteCmd);
  
  Tcl_CreateObjCommand(interp, "initSound",
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
Sound_SafeInit(Tcl_Interp *interp)
{
  return Sound_Init(interp);
}

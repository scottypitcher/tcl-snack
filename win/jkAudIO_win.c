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
#include "jkSound.h"
#include <stdio.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" void Snack_WriteLogInt(char *s, int n);
extern "C" void Snack_WriteLog(char *s);
#else
extern void Snack_WriteLogInt(char *s, int n);
extern void Snack_WriteLog(char *s);
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define NBUFS 64

static HWAVEOUT      hWaveOut;
static HWAVEIN       hWaveIn;
static HMIXER        hMixer;
static WAVEFORMATEX  wFormatIn;
static WAVEHDR       waveHdrIn[NBUFS];
static char          *blockIn[NBUFS];
static WAVEFORMATEX  wFormatOut;
static WAVEHDR       waveHdrOut[NBUFS];
static HANDLE        blockOut[NBUFS];
static int           blockSizeOut[NBUFS];
static WAVEINCAPS    wInCaps;
static WAVEOUTCAPS   wOutCaps;
static MIXERCAPS     wMixCaps;
/*static ADesc  winad;*/
static int    correction = 1;

#define SNACK_NUMBER_MIXERS 1

struct MixerLink mixerLinks[SNACK_NUMBER_MIXERS][2];
static int selectedMixer = 0;

static char *outDeviceList[4];
static numOutDevs = 0;
static char *inDeviceList[4];
static numInDevs = 0;
static char *mixerDeviceList[4];
static numMixDevs = 0;

#include <dsound.h>

static char *DSOutDeviceList[4];
static GUID guidOut[4];
static numDSOutDevs = 0;
static char *DSInDeviceList[4];
static GUID guidIn[4];
static numDSInDevs = 0;

/*
 * The following structure contains pointers to all of the DirectSound API
 * entry points used by Snack. It is initialized by SnackAudioInit. Since we
 * dynamically load dsound.dll, we must use this function table to refer to 
 * functions in the DirectSound API.
 */

static struct {
  HRESULT (WINAPI * DirectSoundCreate)(LPGUID lpguid, 
			  LPDIRECTSOUND *ppDS, IUnknown FAR * pUnkOuter);
  BOOL (WINAPI * DirectSoundEnumerate)(LPDSENUMCALLBACK lpCB, LPVOID lpVoid);
  HRESULT (WINAPI * DirectSoundCaptureCreate)(LPGUID lpguid, 
			 LPDIRECTSOUNDCAPTURE *ppDS, IUnknown FAR * pUnkOuter);
  BOOL (WINAPI * DirectSoundCaptureEnumerate)(LPDSENUMCALLBACK lpCB,
					      LPVOID lpVoid);
} ds;

#define NSECS 3

static int useDSound = 0;
EXTERN HINSTANCE snackHInst;
static WNDCLASSA windowClass;
static HWND snackHwnd = NULL;
HINSTANCE hDSoundInstance = NULL;
static LPDIRECTSOUND lpDirectSound;
static LPDIRECTSOUNDCAPTURE lpDSCapture;

int
SnackAudioOpen(ADesc *A, Tcl_Interp *interp, char *device,
	       int mode, int freq, int nchannels, int encoding)
{
  int i, devIndex = -1;

  if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioOpen", mode);

  switch (mode) {
  case RECORD:
    if (device != NULL) {
      for (i = 0; i < numInDevs; i++) {
	if (strcmp(inDeviceList[i], device) == 0) {
	  useDSound = 0;
	  devIndex = i;
	  break;
	}
      }
      if (devIndex == -1) {
	for (i = 0; i < numDSInDevs; i++) {
	  if (strcmp(DSInDeviceList[i], device) == 0) {
	    useDSound = 1;
	    devIndex = i;
	    break;
	  }
	}
      }
    }
    break;
  case PLAY:
    if (device != NULL) {
      for (i = 0; i < numOutDevs; i++) {
	if (strcmp(outDeviceList[i], device) == 0) {
	  useDSound = 0;
	  devIndex = i;
	  break;
	}
      }
      if (devIndex == -1) {
	for (i = 0; i < numDSOutDevs; i++) {
	  if (strcmp(DSOutDeviceList[i], device) == 0) {
	    useDSound = 1;
	    devIndex = i;
	    break;
	  }
	}
      }
    }
    break; 
  }
  
  if (A->debug == 1) Snack_WriteLogInt("device", devIndex);

  if (useDSound) {
    int res = 0;
    HRESULT hr;
    DSCBCAPS dscbcaps;

    A->mode = mode;
    A->curr = 0;
    A->freq = freq;
    A->nChannels = nchannels;
    A->shortRead = 0;

    memset(&A->pcmwf, 0, sizeof(PCMWAVEFORMAT));
    switch (mode) {
    case RECORD:
      switch (encoding) {
      case LIN16:
	A->bytesPerSample = sizeof(short);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	break;
      case ALAW:
	A->bytesPerSample = sizeof(char);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_ALAW;
	break;
      case MULAW:
	A->bytesPerSample = sizeof(char);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_MULAW;
	break;
      case LIN8OFFSET:
	A->bytesPerSample = sizeof(char);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	break;
      }
      A->pcmwf.wf.nChannels       = nchannels;
      A->pcmwf.wf.nSamplesPerSec  = freq;
      A->pcmwf.wf.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
      A->pcmwf.wf.nBlockAlign     = A->bytesPerSample * nchannels;
      A->pcmwf.wBitsPerSample     = A->bytesPerSample * 8;

      memset(&A->dscbdesc, 0, sizeof(DSCBUFFERDESC)); 
      A->dscbdesc.dwSize = sizeof(DSCBUFFERDESC); 
      A->dscbdesc.dwFlags = 0;/*DSCBCAPS_WAVEMAPPED;*/ 
      A->dscbdesc.dwBufferBytes = NSECS * A->pcmwf.wf.nAvgBytesPerSec;
      A->dscbdesc.lpwfxFormat = (LPWAVEFORMATEX)&A->pcmwf;
      A->lplpDscb = NULL;
      Snack_WriteLog("**\n"); 
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_ALLOCATED); 
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_BADFORMAT );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_INVALIDPARAM );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_NOAGGREGATION );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_OUTOFMEMORY );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_UNINITIALIZED );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_UNSUPPORTED);
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_INVALIDCALL );
      Snack_WriteLogInt("CreateSoundBuffer", DSERR_PRIOLEVELNEEDED );
      Snack_WriteLogInt("CreateSoundBuffer", DS_OK);
      if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioOpen", 14);

      if (devIndex == -1) {
	hr = (*ds.DirectSoundCaptureCreate)(NULL, &lpDSCapture,
					    NULL);
      } else {
	hr = (*ds.DirectSoundCaptureCreate)(&guidIn[devIndex], &lpDSCapture,
					    NULL);
      }
      if (hr != DS_OK) {
	Tcl_AppendResult(interp, "Failed creating capture object.", NULL);
	return TCL_ERROR; 
      }
      hr = IDirectSoundCapture_CreateCaptureBuffer(lpDSCapture, 
				      &A->dscbdesc, &A->lplpDscb, NULL);

      if (hr != DS_OK) {
        A->lplpDscb = NULL;
	if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioOpen", hr); 
	Tcl_AppendResult(interp, "Failed creating capture buffer.", NULL);
	return TCL_ERROR;
      }
      dscbcaps.dwSize = sizeof(DSCBCAPS);
      hr = IDirectSoundCaptureBuffer_GetCaps(A->lplpDscb, &dscbcaps);
      A->BufLen = dscbcaps.dwBufferBytes;
      hr = IDirectSoundCaptureBuffer_Start(A->lplpDscb, DSCBSTART_LOOPING);
      A->BufPos = -1;
if (A->debug == 1) Snack_WriteLogInt("Buffer_Start", hr); 
      break;
    case PLAY:
      memset(&A->pcmwfPB, 0, sizeof(PCMWAVEFORMAT));
      switch (encoding) {
      case LIN16:
	A->bytesPerSample = sizeof(short);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	A->pcmwfPB.wf.wFormatTag = WAVE_FORMAT_PCM;
	break;
      case ALAW:
	A->bytesPerSample = sizeof(char);
	break;
      case MULAW:
	A->bytesPerSample = sizeof(char);
	break;
      case LIN8OFFSET:
	A->bytesPerSample = sizeof(char);
	A->pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	A->pcmwfPB.wf.wFormatTag = WAVE_FORMAT_PCM;
	break;
      }
      A->pcmwf.wf.nChannels       = nchannels;
      A->pcmwf.wf.nSamplesPerSec  = freq;
      A->pcmwf.wf.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
      A->pcmwf.wf.nBlockAlign     = A->bytesPerSample * nchannels;
      A->pcmwf.wBitsPerSample     = A->bytesPerSample * 8;

      A->pcmwfPB.wf.nChannels       = nchannels;
      A->pcmwfPB.wf.nSamplesPerSec  = freq;
      A->pcmwfPB.wf.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
      A->pcmwfPB.wf.nBlockAlign     = A->bytesPerSample * nchannels;
      A->pcmwfPB.wBitsPerSample     = A->bytesPerSample * 8;

      memset(&A->dsbdesc, 0, sizeof(DSBUFFERDESC)); 
      A->dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
      A->dsbdesc.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_STICKYFOCUS; 
      A->dsbdesc.dwBufferBytes = NSECS * A->pcmwf.wf.nAvgBytesPerSec;
      A->dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&A->pcmwf;

      if (devIndex == -1) {
	hr = (*ds.DirectSoundCreate)(NULL, &lpDirectSound, NULL);
      } else {
	hr = (*ds.DirectSoundCreate)(&guidOut[devIndex], &lpDirectSound, NULL);
      }
      if (hr != DS_OK) {
	Tcl_AppendResult(interp, "Failed creating DirectSound object.", NULL);
	return TCL_ERROR; 
      }
      hr = IDirectSound_SetCooperativeLevel(lpDirectSound, snackHwnd,
					    DSSCL_PRIORITY);
      if (hr != DS_OK) {
	Tcl_AppendResult(interp, "Failed setting cooperative level.", NULL);
	return TCL_ERROR; 
      }
      hr = IDirectSound_CreateSoundBuffer(lpDirectSound, &A->dsbdesc,
					  &A->lplpDsb, NULL);
      
      memset(&A->dsbdescPB, 0, sizeof(DSBUFFERDESC)); 
      A->dsbdescPB.dwSize = sizeof(DSBUFFERDESC); 
      A->dsbdescPB.dwFlags = DSBCAPS_PRIMARYBUFFER; 
      A->dsbdescPB.dwBufferBytes = 0;
      A->dsbdescPB.lpwfxFormat = NULL;  
      hr = IDirectSound_CreateSoundBuffer(lpDirectSound, &A->dsbdescPB,
					  &A->lplpDsPB, NULL);
      
      if (A->debug == 1) {
	Snack_WriteLogInt("CreateSoundBuffer", hr);
      }
      if (hr != DS_OK) {
        A->lplpDsb = NULL;
	Tcl_AppendResult(interp, "Failed creating sound buffer.", NULL);
	return TCL_ERROR; 
      }
      A->BufLen = NSECS * A->pcmwf.wf.nAvgBytesPerSec;
      A->BufPos = 0;
      A->written = 0;
      hr = IDirectSoundBuffer_SetFormat(A->lplpDsPB,
					(LPWAVEFORMATEX)&A->pcmwfPB);
      if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioOpen", hr);
    }
    return TCL_OK;
  } else { /* Windows multimedia library */
    int res = 0, i, size;

    if (A->debug == 1) Snack_WriteLog("Enter SnackAudioOpen\n");

    A->mode = mode;
    A->curr = 0;
    A->freq = freq;
    A->nChannels = nchannels;
    A->shortRead = 0;
    A->convert = 0;

    if (devIndex == -1) {
      devIndex = WAVE_MAPPER;
    }
    
    switch (mode) {
    case RECORD:
      memset(&wFormatIn, 0, sizeof(WAVEFORMATEX));
      switch (encoding) {
      case LIN16:
	A->bytesPerSample = sizeof(short);
	wFormatIn.wFormatTag = WAVE_FORMAT_PCM;
	break;
      case ALAW:
	A->bytesPerSample = sizeof(char);
	wFormatIn.wFormatTag = WAVE_FORMAT_ALAW;
	break;
      case MULAW:
	A->bytesPerSample = sizeof(char);
	wFormatIn.wFormatTag = WAVE_FORMAT_MULAW;
	break;
      case LIN8OFFSET:
	A->bytesPerSample = sizeof(char);
	wFormatIn.wFormatTag = WAVE_FORMAT_PCM;
	break;
      }
      wFormatIn.nChannels       = nchannels;
      wFormatIn.nSamplesPerSec  = freq;
      wFormatIn.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
      wFormatIn.nBlockAlign     = A->bytesPerSample * nchannels;
      wFormatIn.wBitsPerSample  = A->bytesPerSample * 8;
      wFormatIn.cbSize          = 0;
      
      size = A->bytesPerSample * freq / 16;
      
      res = waveInOpen(&hWaveIn, devIndex, 
		       (WAVEFORMATEX *)&wFormatIn, 0, 0L, CALLBACK_NULL);
      if (res) {
	Tcl_AppendResult(interp, "waveInOpen failed!", NULL);
	return TCL_ERROR;
      }
      for (i = 0; i < NBUFS; i++) {
	blockIn[i] = ckalloc(size);
	if (!blockIn[i]) {
	  Tcl_AppendResult(interp, "Failed allocating audio block.", NULL);
	  return TCL_ERROR;
	}
	if (A->debug == 1) Snack_WriteLogInt("blockaddr", (int) blockIn[i]);
	memset(&waveHdrIn[i], 0, sizeof(WAVEHDR));

	waveHdrIn[i].lpData = blockIn[i];
	waveHdrIn[i].dwBufferLength = size;
	waveHdrIn[i].dwFlags = 0L;
	waveHdrIn[i].dwLoops = 0L;
	res = waveInPrepareHeader(hWaveIn, &waveHdrIn[i], sizeof(WAVEHDR));
	res = waveInAddBuffer(hWaveIn, &waveHdrIn[i], sizeof(WAVEHDR));
      }
      res = waveInStart(hWaveIn);
      break;
    
    case PLAY:
      memset(&wFormatOut, 0, sizeof(WAVEFORMATEX));
      switch (encoding) {
      case LIN16:
	A->bytesPerSample = sizeof(short);
	wFormatOut.wFormatTag = WAVE_FORMAT_PCM;
	break;
      case ALAW:
	A->bytesPerSample = sizeof(char);
	wFormatOut.wFormatTag = WAVE_FORMAT_ALAW;
	break;
      case MULAW:
	A->bytesPerSample = sizeof(char);
	wFormatOut.wFormatTag = WAVE_FORMAT_MULAW;
	break;
      case LIN8OFFSET:
	A->bytesPerSample = sizeof(char);
	wFormatOut.wFormatTag = WAVE_FORMAT_PCM;
	break;
      }
      wFormatOut.nChannels       = nchannels;
      wFormatOut.nSamplesPerSec  = freq;
      wFormatOut.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
      wFormatOut.nBlockAlign     = A->bytesPerSample * nchannels;
      wFormatOut.wBitsPerSample  = A->bytesPerSample * 8;
      wFormatOut.cbSize          = 0;
      
      if (encoding == MULAW || encoding == ALAW) {
	res = waveOutOpen(NULL, devIndex, (WAVEFORMATEX *)&wFormatOut,
			  0, 0L, WAVE_FORMAT_QUERY);
	if (res != MMSYSERR_NOERROR) {
	  A->convert = encoding;
	  A->bytesPerSample = sizeof(short);
	  wFormatOut.wFormatTag = WAVE_FORMAT_PCM;
	  wFormatOut.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
	  wFormatOut.nBlockAlign     = A->bytesPerSample * nchannels;
	  wFormatOut.wBitsPerSample  = A->bytesPerSample * 8;
	  if (A->debug == 1) Snack_WriteLogInt("Converting", encoding);
	}
      }
      
      res = waveOutOpen(&hWaveOut, devIndex,
			(WAVEFORMATEX *)&wFormatOut, 0, 0L, CALLBACK_NULL);
      if (res) {
	Tcl_AppendResult(interp, "waveOutOpen failed!", NULL);
	return TCL_ERROR;
      }
      for (i = 0; i < NBUFS; i++) {
	blockOut[i] = NULL;
	blockSizeOut[i] = 0;
	waveHdrOut[i].dwFlags = WHDR_DONE;
      }
      break;
    }

    if (A->debug == 1) Snack_WriteLogInt("Correction", correction);
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioOpen", A->bytesPerSample);

    return TCL_OK;
  }
}

int
SnackAudioClose(ADesc *A)
{
  if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioClose", useDSound);

  if (useDSound) {
    HRESULT hr;

    switch (A->mode) {
    case RECORD:
      if (A->lplpDscb==0) {
	if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioClose",(int)A->lplpDscb);
	return(0);
      }
      hr = IDirectSoundCaptureBuffer_Stop(A->lplpDscb);
      IDirectSoundCaptureBuffer_Release(A->lplpDscb);
      IDirectSoundCapture_Release(lpDSCapture);
      break;
    case PLAY:
	if (A->debug == 1) Snack_WriteLogInt("Exit",A->written*(A->bytesPerSample *A->nChannels));
	if (A->debug == 1) Snack_WriteLogInt("Exit", SnackAudioPlayed(A));
      if (A->written*(A->bytesPerSample *A->nChannels) > SnackAudioPlayed(A)) {
	if (A->debug == 1) Snack_WriteLog("Exit failed SnackAudioClose\n");
	return(-1);
      } else {
	hr = IDirectSoundBuffer_Stop(A->lplpDsb);
	IDirectSoundBuffer_Release(A->lplpDsb);
	A->lplpDsb = NULL;
	IDirectSound_Release(lpDirectSound);
      }
      break;
    }
  } else { /* Windows multimedia library */
    int i = 0, res;

    if (A->debug == 1) Snack_WriteLogInt("mode", (int) A->mode);

    switch (A->mode) {
    case RECORD:
      waveInStop(hWaveIn);
      waveInReset(hWaveIn);
      waveInClose(hWaveIn);
      for (i = 0; i < NBUFS; i++) {
	waveInUnprepareHeader(hWaveIn, &waveHdrIn[i], sizeof(WAVEHDR));
	if (blockIn[i]) {
	  ckfree(blockIn[i]);
	  if (A->debug == 1) Snack_WriteLogInt("freeing", (int) blockIn[i]);
	  blockIn[i] = NULL;
	}
      }
      A->mode = 0;
      break;
    
    case PLAY:
      if (A->debug == 1) Snack_WriteLog("Attempting waveOutClose\n");
      res = waveOutClose(hWaveOut);
      if (A->debug == 1) Snack_WriteLogInt("waveOutClose", res);
      if (res == WAVERR_STILLPLAYING) return(-1);
      if (A->debug == 1) Snack_WriteLog("waveOutClose ok\n");
      for (i = 0; i < NBUFS; i++) {
	waveOutUnprepareHeader(hWaveOut, &waveHdrOut[i], sizeof(WAVEHDR));
	if (blockOut[i]) {
	  ckfree(blockOut[i]);
	  blockOut[i] = NULL;
	}
      }
      if (A->debug == 1) Snack_WriteLog("waveOutUnprepareHeader ok\n");
      A->mode = 0;
      break;

    default:
      if (A->debug == 1) Snack_WriteLog("nop\n");
      break;
    }  
  }
  if (A->debug == 1) Snack_WriteLog("Exit SnackAudioClose\n");

  return(0);
}

int
SnackAudioPause(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;
    
    switch (A->mode) {
    case RECORD:
      hr = IDirectSoundCaptureBuffer_Stop(A->lplpDscb);
      break;
    case PLAY:
      hr = IDirectSoundBuffer_Stop(A->lplpDsb);
      break;
    }
  } else { /* Windows multimedia library */
    int res;

    switch (A->mode) {
    case RECORD:
      break;
      
    case PLAY:
      res = waveOutPause(hWaveOut);
      break;
    }
  }
  return(-1);
}

void
SnackAudioResume(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;

    switch (A->mode) {
    case RECORD:
      break;      
    case PLAY:
      hr = IDirectSoundBuffer_Play(A->lplpDsb, 0, 0, DSBPLAY_LOOPING);
      break;
    }
  } else { /* Windows multimedia library */
    int res;
    switch (A->mode) {
    case RECORD:
      break;
    case PLAY:
      res = waveOutRestart(hWaveOut);
      break;
    }
  }
}

void
SnackAudioFlush(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;
    
    switch (A->mode) {
    case RECORD:
      break;
    case PLAY:
      hr = IDirectSoundBuffer_Stop(A->lplpDsb);
      A->written = 0;
      break;
    }
  } else { /* Windows multimedia library */
    int res = 0;
    switch (A->mode) {
    case RECORD:
      /*
	res = waveInReset(hWaveIn);
	*/
      break;
      
    case PLAY:
      res = waveOutReset(hWaveOut);
      if (A->debug == 1) Snack_WriteLogInt("waveOutReset", res);
      break;
    }
  }
}

void
SnackAudioPost(ADesc *A)
{
}

int
SnackAudioRead(ADesc *A, void *buf, int nFrames)
{
  if (useDSound) {
    LPVOID lpvPtr1; 
    DWORD dwBytes1; 
    LPVOID lpvPtr2; 
    DWORD dwBytes2; 
    HRESULT hr;
    int size = 0;
    DWORD pos = 0;

    if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioRead", nFrames);

    hr = IDirectSoundCaptureBuffer_GetCurrentPosition(A->lplpDscb, NULL,&pos);

    if (A->BufPos == -1) {
      return(0);
    } else if (pos > A->BufPos) {
      size = pos - A->BufPos;
    } else {
      size = A->BufLen - (A->BufPos - pos);
    }

    if (size > nFrames * (A->bytesPerSample * A->nChannels)) {
      size = nFrames * (A->bytesPerSample * A->nChannels);
    }

    hr = IDirectSoundCaptureBuffer_Lock(A->lplpDscb, A->BufPos, size, 
			      &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0);

    if (A->debug == 1) Snack_WriteLogInt("Lock", hr);
    if (A->debug == 1) Snack_WriteLogInt(" at ", A->BufPos);
    if (A->debug == 1) Snack_WriteLogInt(" -1 ", dwBytes1);
    if (A->debug == 1) Snack_WriteLogInt(" -2 ", dwBytes2);

    if (hr == DS_OK) {
      if (A->debug == 1) {
	Snack_WriteLogInt("memcpy to", (int) lpvPtr1);
	Snack_WriteLogInt("length", dwBytes1);
      }
      memcpy(buf, lpvPtr1, dwBytes1);
      if (NULL != lpvPtr2) { 
	if (A->debug == 1) {
	  Snack_WriteLogInt("memcpy2 to", (int) lpvPtr2);
	  Snack_WriteLogInt("length2", dwBytes2);
	}
	memcpy((char*)buf+dwBytes1, lpvPtr2, dwBytes2); 
      } 
      hr = IDirectSoundCaptureBuffer_Unlock(A->lplpDscb, lpvPtr1, dwBytes1,
				     lpvPtr2,  dwBytes2);

      A->BufPos = (A->BufPos + dwBytes1 + dwBytes2) % A->BufLen; 
      nFrames = size / (A->bytesPerSample * A->nChannels);

      if (hr == DS_OK) {
	if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioRead", A->BufPos);
	return(nFrames);
      }
    }
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioRead2", hr);
    return(0); 
  } else { /* Windows multimedia library */
    int res = 0, i, noread = 1, nsamps, n = 0;
    
    if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioRead", nFrames);

    if (A->mode == RECORD) {
      for (i = 0; i < NBUFS; i++) {
	if (waveHdrIn[i].dwFlags & WHDR_DONE) noread = 0;
      }
      if (A->debug == 1) Snack_WriteLogInt(" noread", noread);
      if (noread) {
	return(0);
      }
      if (A->debug == 1) Snack_WriteLogInt(" shortRead", A->shortRead);
      if (A->shortRead > 0) {
	int offset = A->shortRead * (A->bytesPerSample * A->nChannels);
	int rest = waveHdrIn[A->curr].dwBytesRecorded - offset;
	
	if (A->debug == 1) Snack_WriteLog("short read\n");
	
	memcpy((char *)buf, blockIn[A->curr] + offset, rest);
	waveInUnprepareHeader(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	waveHdrIn[A->curr].lpData = blockIn[A->curr];
	waveHdrIn[A->curr].dwBufferLength = A->bytesPerSample * A->freq / 16;
	waveHdrIn[A->curr].dwFlags = 0L;
	waveHdrIn[A->curr].dwLoops = 0L;
	res = waveInPrepareHeader(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	if (A->debug == 1) Snack_WriteLogInt("waveInPrepareHeader", res);
	res = waveInAddBuffer(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	if (A->debug == 1) Snack_WriteLogInt("waveInAddBuffer", res);
	A->curr = (A->curr + 1) % NBUFS;
	A->shortRead = 0;
	n = rest / (A->bytesPerSample * A->nChannels);
	if (A->debug == 1) Snack_WriteLogInt("short read rest", n);
      }
      
      while ((waveHdrIn[A->curr].dwFlags & WHDR_DONE) && (n < nFrames)) {
	if ((unsigned int)(nFrames - n) * (A->bytesPerSample * A->nChannels) < waveHdrIn[A->curr].dwBytesRecorded) {
	  nsamps = nFrames - n;
	  A->shortRead = nsamps;
	  memcpy(((char *)buf + n * A->bytesPerSample * A->nChannels),
		 blockIn[A->curr], nsamps * (A->bytesPerSample * A->nChannels));
	  if (A->debug == 1) Snack_WriteLogInt("short read", nsamps);
	} else {
	  if (A->debug == 1) Snack_WriteLogInt(" pre memcpy",A->curr);
	  if (A->debug == 1) Snack_WriteLogInt(" to", (int)buf + n * A->bytesPerSample * A->nChannels);
	  if (A->debug == 1) Snack_WriteLogInt(" from", (int)blockIn[A->curr]);
	  if (A->debug == 1) Snack_WriteLogInt(" size", waveHdrIn[A->curr].dwBytesRecorded);
	  memcpy(((char *)buf + n * A->bytesPerSample * A->nChannels),
		 blockIn[A->curr], waveHdrIn[A->curr].dwBytesRecorded);
	  if (A->debug == 1) Snack_WriteLogInt(" post memcpy",A->curr);
	  nsamps = waveHdrIn[A->curr].dwBytesRecorded / (A->bytesPerSample * A->nChannels);
	  waveInUnprepareHeader(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	  if (A->debug == 1) Snack_WriteLogInt("waveInUnprepareHeader", res);
	  waveHdrIn[A->curr].lpData = blockIn[A->curr];
	  waveHdrIn[A->curr].dwBufferLength = A->bytesPerSample * A->freq / 16;
	  waveHdrIn[A->curr].dwFlags = 0L;
	  waveHdrIn[A->curr].dwLoops = 0L;
	  res = waveInPrepareHeader(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	  if (A->debug == 1) Snack_WriteLogInt("waveInPrepareHeader", res);
	  res = waveInAddBuffer(hWaveIn, &waveHdrIn[A->curr], sizeof(WAVEHDR));
	  if (A->debug == 1) Snack_WriteLogInt("waveInAddBuffer", res);
	  A->curr = (A->curr + 1) % NBUFS;
	  A->shortRead = 0;
	}
	n += nsamps;
      }
    }
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioRead", res);
    
    return(n);
  }
}

int
SnackAudioWrite(ADesc *A, void *buf, int nFrames)
{
  if (useDSound) {
    LPVOID lpvPtr1; 
    DWORD dwBytes1; 
    LPVOID lpvPtr2; 
    DWORD dwBytes2; 
    HRESULT hr;
    int size = 0;

    if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioWrite", nFrames);

    if (nFrames > SnackAudioWriteable(A)) {
      nFrames = SnackAudioWriteable(A);
    }
    size = nFrames * A->bytesPerSample * A->nChannels;
	if (A->debug == 1) {
		Snack_WriteLogInt("Locking at", A->BufPos);
		Snack_WriteLogInt("size", size);
	}
    hr = IDirectSoundBuffer_Lock(A->lplpDsb, A->BufPos,
				 size, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0); 
    if (A->debug == 1) Snack_WriteLogInt("Lock", hr);
    if (hr == DSERR_BUFFERLOST) { 
      IDirectSoundBuffer_Restore(A->lplpDsb); 
      hr = IDirectSoundBuffer_Lock(A->lplpDsb, A->BufPos, size, 
				   &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0); 
      if (A->debug == 1) Snack_WriteLogInt("Lock2", hr);
    }
    if (hr == DS_OK) {
      if (A->debug == 1) {
	Snack_WriteLogInt("memcpy to", (int) lpvPtr1);
	Snack_WriteLogInt("length", dwBytes1);
      }
      memcpy(lpvPtr1, buf, dwBytes1);
      if (NULL != lpvPtr2) { 
	if (A->debug == 1) {
	  Snack_WriteLogInt("memcpy2 to", (int) lpvPtr2);
	  Snack_WriteLogInt("length2", dwBytes2);
	}
	memcpy(lpvPtr2, (char*)buf+dwBytes1, dwBytes2); 
      } 
      hr = IDirectSoundBuffer_Unlock(A->lplpDsb, &lpvPtr1, dwBytes1, &lpvPtr2, 
				     dwBytes2);
      hr = IDirectSoundBuffer_Play(A->lplpDsb, 0, 0,DSBPLAY_LOOPING);

      A->BufPos = (A->BufPos + size) % A->BufLen; 
      A->written += size;
      
	  if (hr == DS_OK) {
	if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWrite", nFrames);
	return(nFrames); 
      } 
    } 
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWrite", 0);
    return(0); 

  } else { /* Windows multimedia library */
    int res, i, nowrit = 1, size = nFrames * A->bytesPerSample * A->nChannels;

    if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioWrite", nFrames);

    if (nFrames == 0) return(0); 
    for (i = 0; i < NBUFS; i++) {
      if (waveHdrOut[i].dwFlags & WHDR_DONE) nowrit = 0;
    }
    if (nowrit) return(0); 
  
    for (i = 0; i < NBUFS; i++) {
      if (waveHdrOut[i].dwFlags & WHDR_DONE) {
	if (size > blockSizeOut[i]) {
	  if (blockSizeOut[i]) {
	    ckfree(blockOut[i]);
	  }
	  /*if (A->debug == 1) Snack_WriteLogInt("Allocing", size);*/
	  blockOut[i] = ckalloc(size);
	  if (!blockOut[i]) return(0);
	  blockSizeOut[i] = size;
	} else {
	  /*if (A->debug == 1) Snack_WriteLogInt("Reusing", size);*/
	  /*if (A->debug == 1) Snack_WriteLogInt("Block", i);*/
	}
	waveOutUnprepareHeader(hWaveOut, &waveHdrOut[i], sizeof(WAVEHDR));
	
	if (A->convert) {
	  int j;
	  short *s = blockOut[i];
	  
	  for (j = 0; j < nFrames * A->nChannels; j++) {
	    if (A->convert == ALAW) {
	      *s++ = Snack_Alaw2Lin(((unsigned char *)buf)[j]);
	    } else {
	      *s++ = Snack_Mulaw2Lin(((unsigned char *)buf)[j]);
	    }
	  }
	} else {
	  memcpy(blockOut[i], buf, size);
	}
	waveHdrOut[i].lpData = blockOut[i];
	waveHdrOut[i].dwBufferLength = size;
	waveHdrOut[i].dwFlags = 0L;
	waveHdrOut[i].dwLoops = 0L;
	res = waveOutPrepareHeader(hWaveOut, &waveHdrOut[i], sizeof(WAVEHDR));
	res = waveOutWrite(hWaveOut, &waveHdrOut[i], sizeof(WAVEHDR));
	break;
      }
    }
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWrite", res);
  }
  return(nFrames);
}

int
SnackAudioReadable(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;
    DWORD pos = 0;
    DWORD status = 0;

    if (A->debug == 1) Snack_WriteLog("Enter SnackAudioReadable\n");

    hr = IDirectSoundCaptureBuffer_GetStatus(A->lplpDscb, &status);
    if (!(status && DSCBSTATUS_CAPTURING)) return(0);

    hr = IDirectSoundCaptureBuffer_GetCurrentPosition(A->lplpDscb, NULL,&pos);

    if (A->BufPos == -1 && pos == 0) {
    } else if (A->BufPos == -1 && pos > 0) {
      A->BufPos = 0;
    } else if (pos > A->BufPos) {
      pos = pos - A->BufPos;
    } else {
      pos = A->BufLen - (A->BufPos - pos);
    }

    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioReadable",
					 pos/(A->bytesPerSample*A->nChannels));

    return(pos / (A->bytesPerSample * A->nChannels));
  } else { /* Windows multimedia library */
    int i, n = 0;

    if (A->debug == 1) Snack_WriteLog("Enter SnackAudioReadable\n");

    if (A->mode == RECORD) {
      for (i = 0; i < NBUFS; i++) {
	if (waveHdrIn[i].dwFlags & WHDR_DONE) {
	  n += (waveHdrIn[i].dwBufferLength / (A->bytesPerSample * A->nChannels));
	}
      }
      if (A->shortRead > 0) {
	n -= (waveHdrIn[A->curr].dwBufferLength / (A->bytesPerSample * A->nChannels));
	n += A->shortRead;
      }
    }
    if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioReadable", n);

    return(n);
  }
}

int
SnackAudioWriteable(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;
    DWORD ppos = 0;
    DWORD status = 0;

    if (A->debug == 1) Snack_WriteLog("Enter SnackAudioWriteable\n");

    hr = IDirectSoundBuffer_GetStatus(A->lplpDsb, &status);
    if (!(status && DSBSTATUS_PLAYING)) {
      if (A->debug == 1) Snack_WriteLogInt("x SnackAudioWriteable",A->BufLen);
      return(A->BufLen / (A->bytesPerSample * A->nChannels));
    }
    
    hr = IDirectSoundBuffer_GetCurrentPosition(A->lplpDsb, &ppos, NULL);

    if (A->debug == 1) Snack_WriteLogInt("ppos",ppos);
    if (ppos > A->BufPos) {
      if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWriteable1",(ppos - A->BufPos)/(A->bytesPerSample * A->nChannels));
      return ((ppos - A->BufPos)/(A->bytesPerSample * A->nChannels));
    } else {
      if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWriteable2",(A->BufLen - (A->BufPos - ppos))/(A->bytesPerSample * A->nChannels));
      return((A->BufLen - (A->BufPos - ppos))/(A->bytesPerSample * A->nChannels));
    }
  } else { /* Windows multimedia library */
    int res = 0, i;

    for (i = 0; i < NBUFS; i++) {
      if (waveHdrOut[i].dwFlags & WHDR_DONE) res = -1;
    }
    return(res);
  }
}

int
SnackAudioPlayed(ADesc *A)
{
  if (useDSound) {
    HRESULT hr;
    DWORD ppos = 0;
    DWORD status = 0;

    if (A->debug == 1) Snack_WriteLog("Enter SnackAudioPlayed\n");
    
    hr = IDirectSoundBuffer_GetStatus(A->lplpDsb, &status);
    if (!(status && DSBSTATUS_PLAYING)) return(0);
    if (A->debug == 1) Snack_WriteLog("bugg?\n");
    hr = IDirectSoundBuffer_GetCurrentPosition(A->lplpDsb, &ppos, NULL);
    if (A->debug == 1) Snack_WriteLog("nej\n");
    if (ppos > A->BufPos) {
      if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioPlayed1",
					   (A->written - A->BufLen + (ppos - A->BufPos))/(A->bytesPerSample * A->nChannels));
      return ((A->written - A->BufLen + (ppos - A->BufPos))/(A->bytesPerSample * A->nChannels));
    } else {
      if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioPlayed2",
					   (A->written - (A->BufPos - ppos))/(A->bytesPerSample * A->nChannels));
      return((A->written - (A->BufPos - ppos))/(A->bytesPerSample * A->nChannels));
    }
  } else { /* Windows multimedia library */
    MMTIME mmtime;
  
    mmtime.wType = TIME_SAMPLES;
    waveOutGetPosition(hWaveOut, &mmtime, sizeof(MMTIME));

    if ((wFormatOut.wFormatTag == WAVE_FORMAT_MULAW) ||
	(wFormatOut.wFormatTag == WAVE_FORMAT_ALAW)) {
      return(mmtime.u.sample / correction);
    }
    return(mmtime.u.sample);
  }
 
}

static LRESULT CALLBACK
SnackWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case MM_MIXM_LINE_CHANGE:
    return TRUE;
  case MM_MIXM_CONTROL_CHANGE:
    return TRUE;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

static BOOL CALLBACK
DSEnumProc(LPGUID lpGUID, LPSTR lpszDesc, LPSTR lpszDrvName, LPVOID lpContext)
{
  if (lpGUID != NULL) {
    memcpy(&guidOut[numDSOutDevs], lpGUID, sizeof(GUID));
  } 
  DSOutDeviceList[numDSOutDevs] = ckalloc(strlen(lpszDesc)+15);
  if (DSOutDeviceList[numDSOutDevs] != NULL) {
    strcpy(DSOutDeviceList[numDSOutDevs], lpszDesc);
    strcat(DSOutDeviceList[numDSOutDevs], " (DirectSound)");
  }
  numDSOutDevs++;
  
  return(TRUE);
}

static BOOL CALLBACK
DSCEnumProc(LPGUID lpGUID, LPSTR lpszDesc, LPSTR lpszDrvName, LPVOID lpContext)
{
  if (lpGUID != NULL) {
    memcpy(&guidIn[numDSInDevs], lpGUID, sizeof(GUID));
  } 
  DSInDeviceList[numDSInDevs] = ckalloc(strlen(lpszDesc)+15);
  if (DSInDeviceList[numDSInDevs] != NULL) {
    strcpy(DSInDeviceList[numDSInDevs], lpszDesc);
    strcat(DSInDeviceList[numDSInDevs], " (DirectSound)");
  }
  numDSInDevs++;
  
  return(TRUE);
}

void
SnackAudioInit()
{
  int i;
  HRESULT hr;

  /*  Snack_WriteLog("SnackAudioInit\n");*/

  for (i = 0; i < (int)waveInGetNumDevs(); i++) {
    if (waveInGetDevCaps(i, &wInCaps, sizeof(WAVEINCAPS)) == 0) {
      inDeviceList[numInDevs] = ckalloc(strlen(wInCaps.szPname)+17);
      if (inDeviceList[numInDevs] != NULL) {
	strcpy(inDeviceList[numInDevs], wInCaps.szPname);
	strcat(inDeviceList[numInDevs], " (Win multimedia)");
      }
      numInDevs++;
    }
  }

  for (i = 0; i < (int)waveOutGetNumDevs(); i++) {
    if (waveOutGetDevCaps(i, &wOutCaps, sizeof(WAVEOUTCAPS)) == 0) {
      outDeviceList[numOutDevs] = ckalloc(strlen(wOutCaps.szPname)+17);
      if (outDeviceList[numOutDevs] != NULL) {
	strcpy(outDeviceList[numOutDevs], wOutCaps.szPname);
	strcat(outDeviceList[numOutDevs], " (Win multimedia)");
      }
      numOutDevs++;
    }
  }

  for (i = 0; i < (int)mixerGetNumDevs(); i++) {
    if (mixerGetDevCaps(i, &wMixCaps, sizeof(MIXERCAPS)) == 0) {
      mixerDeviceList[numMixDevs] = ckalloc(strlen(wMixCaps.szPname)+17);
      if (mixerDeviceList[numMixDevs] != NULL) {
	strcpy(mixerDeviceList[numMixDevs], wMixCaps.szPname);
      }
      numMixDevs++;
    }
  }

  hDSoundInstance = LoadLibrary("dsound.dll");

  windowClass.style = 0;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = snackHInst;
  windowClass.hbrBackground = NULL;
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = "SnackDSound";
  windowClass.lpfnWndProc = SnackWindowProc;
  windowClass.hIcon = NULL;
  windowClass.hCursor = NULL;
  
  if (!RegisterClassA(&windowClass)) {
    return;
  }

  snackHwnd = CreateWindowA("SnackDSound", "SnackDSound", WS_TILED,
			    0, 0, 0, 0, NULL, NULL, snackHInst, NULL);

  /*
  if (snackHwnd != NULL) {
    mixerOpen(&hMixer, 0, SnackWindowProc, NULL, 0);
  }
  */
  if (hDSoundInstance != NULL) {
    ds.DirectSoundCreate = (HRESULT (WINAPI *)(LPGUID lpguid, 
		       LPDIRECTSOUND *lpDirectSound, IUnknown FAR * pUnkOuter))
      GetProcAddress(hDSoundInstance, "DirectSoundCreate");
    ds.DirectSoundEnumerate = (BOOL (WINAPI *)(LPDSENUMCALLBACK lpCB,
					       LPVOID lpVoid))
      GetProcAddress(hDSoundInstance, "DirectSoundEnumerateA");
    ds.DirectSoundCaptureCreate = (HRESULT (WINAPI *)(LPGUID lpguid, 
	      LPDIRECTSOUNDCAPTURE *lpDirectSound, IUnknown FAR * pUnkOuter))
      GetProcAddress(hDSoundInstance, "DirectSoundCreate");
    ds.DirectSoundCaptureEnumerate = (BOOL (WINAPI *)(LPDSENUMCALLBACK lpCB,
						    LPVOID lpVoid))
      GetProcAddress(hDSoundInstance, "DirectSoundCaptureEnumerateA");

    if (ds.DirectSoundCreate == NULL || 
       	ds.DirectSoundEnumerate == NULL) {
      return;
    }

    if (ds.DirectSoundCaptureCreate == NULL ||
	ds.DirectSoundCaptureEnumerate == NULL) {
      /* ingen direct capture!!! */
    }

    hr = (*ds.DirectSoundCreate)(NULL, &lpDirectSound, NULL);

    if (hr == DS_OK) {

      (*ds.DirectSoundEnumerate)((LPDSENUMCALLBACK) DSEnumProc, (LPVOID) NULL);
            
      if (ds.DirectSoundCaptureEnumerate != NULL) {
	(*ds.DirectSoundCaptureEnumerate)((LPDSENUMCALLBACK) DSCEnumProc,
					  (LPVOID) NULL);
      }
      
      IDirectSound_Release(lpDirectSound);
    }
  } else {
    /*
      unsigned char buf[2] = { 255, 255 };
      int res = 0;

      if (AInit(&winad, PLAY, 8000, 1, SPEAKER, MULAW) == 0) {
      AWrite(&winad, buf, 2);
      Tcl_Sleep(50);
      res = APlayed(&winad);
      if (res == 4) correction = 2;
      while (AClose(&winad) != 0);
      }*/
  }
}

void
SnackAudioFree()
{
  int i, j;

  if (snackHwnd != NULL) {
    DestroyWindow(snackHwnd);
    snackHwnd = NULL;
  }
  UnregisterClassA("SnackDSound", snackHInst);
  if (hDSoundInstance != NULL) {
    FreeLibrary(hDSoundInstance);
  }
  for (i = 0; i < numInDevs; i++) {
    ckfree(inDeviceList[i]);
  }
  for (i = 0; i < numOutDevs; i++) {
    ckfree(outDeviceList[i]);
  }
  for (i = 0; i < numMixDevs; i++) {
    ckfree(mixerDeviceList[i]);
  }
  for (i = 0; i < numDSInDevs; i++) {
    ckfree(DSInDeviceList[i]);
  }
  for (i = 0; i < numDSOutDevs; i++) {
    ckfree(DSOutDeviceList[i]);
  }

  for (i = 0; i < SNACK_NUMBER_MIXERS; i++) {
    for (j = 0; j < 2; j++) {
      if (mixerLinks[i][j].mixer != NULL) {
	ckfree(mixerLinks[i][j].mixer);
      }
      if (mixerLinks[i][j].mixerVar != NULL) {
	ckfree(mixerLinks[i][j].mixerVar);
      }
    }
    if (mixerLinks[i][0].jack != NULL) {
      ckfree(mixerLinks[i][0].jack);
    }
    if (mixerLinks[i][0].jackVar != NULL) {
      ckfree(mixerLinks[i][0].jackVar);
    }
  }
}

void
ASetRecGain(int gain)
{
  unsigned long g = min(max(gain, 0), 100);

}

void
ASetPlayGain(int gain)
{
  int g = min(max(gain, 0), 100);

  waveOutSetVolume((HWAVEOUT) WAVE_MAPPER, g * 655 + g * 65536 * 655);
}

int
AGetRecGain()
{
  int g = 0;

  return(g);
}

int
AGetPlayGain()
{
  int g = 0;
  
  waveOutGetVolume((HWAVEOUT) WAVE_MAPPER, &g);
  g = (g & 0xffff) / 655;

  return(g);
}

void
SnackAudioGetFormats(char *device, char *buf, int n)
{
  int res, pos = 0, i, devIndex = -1;

  if (device != NULL) {
    for (i = 0; i < numOutDevs; i++) {
      if (strcmp(outDeviceList[i], device) == 0) {
	useDSound = 0;
	devIndex = i;
      }
    }
    for (i = 0; i < numDSInDevs; i++) {
      if (strcmp(DSInDeviceList[i], device) == 0) {
	useDSound = 1;
	devIndex = i;
      }
    }
    for (i = 0; i < numInDevs; i++) {
      if (strcmp(inDeviceList[i], device) == 0) {
	useDSound = 0;
	devIndex = i;
      }
    }
    for (i = 0; i < numDSOutDevs; i++) {
      if (strcmp(DSOutDeviceList[i], device) == 0) {
	useDSound = 1;
	devIndex = i;
      }
    }
  }

  if (devIndex == -1) {
    devIndex = WAVE_MAPPER; /* Will not work for DSound */
  }
  
  if (useDSound) {
  } else {
    wFormatOut.wFormatTag = WAVE_FORMAT_PCM;
    wFormatOut.nChannels       = 1;
    wFormatOut.nSamplesPerSec  = 11025;
    wFormatOut.nAvgBytesPerSec = 22050;
    wFormatOut.nBlockAlign     = 2;
    wFormatOut.wBitsPerSample  = 16;
    wFormatOut.cbSize          = 0;
    res = waveOutOpen(NULL, devIndex, (WAVEFORMATEX *)&wFormatOut,
		      0, 0L, WAVE_FORMAT_QUERY);
    if (res == MMSYSERR_NOERROR) {
      pos += sprintf(&buf[pos], "%s", "Lin16");
    }
    
    wFormatOut.wFormatTag = WAVE_FORMAT_PCM;
    wFormatOut.nChannels       = 1;
    wFormatOut.nSamplesPerSec  = 11025;
    wFormatOut.nAvgBytesPerSec = 11025;
    wFormatOut.nBlockAlign     = 1;
    wFormatOut.wBitsPerSample  = 8;
    wFormatOut.cbSize          = 0;
    res = waveOutOpen(NULL, devIndex, (WAVEFORMATEX *)&wFormatOut,
		      0, 0L, WAVE_FORMAT_QUERY);
    if (res == MMSYSERR_NOERROR) {
      pos += sprintf(&buf[pos], "%s", " Lin8offset");
    }
    
    wFormatOut.wFormatTag = WAVE_FORMAT_MULAW;
    wFormatOut.nChannels       = 1;
    wFormatOut.nSamplesPerSec  = 11025;
    wFormatOut.nAvgBytesPerSec = 11025;
    wFormatOut.nBlockAlign     = 1;
    wFormatOut.wBitsPerSample  = 8;
    wFormatOut.cbSize          = 0;
    res = waveOutOpen(NULL, devIndex, (WAVEFORMATEX *)&wFormatOut,
		      0, 0L, WAVE_FORMAT_QUERY);
    if (res == MMSYSERR_NOERROR) {
      pos += sprintf(&buf[pos], "%s", " Mulaw");
    }
    
    wFormatOut.wFormatTag = WAVE_FORMAT_ALAW;
    wFormatOut.nChannels       = 1;
    wFormatOut.nSamplesPerSec  = 11025;
    wFormatOut.nAvgBytesPerSec = 11025;
    wFormatOut.nBlockAlign     = 1;
    wFormatOut.wBitsPerSample  = 8;
    wFormatOut.cbSize          = 0;
    res = waveOutOpen(NULL, devIndex, (WAVEFORMATEX *)&wFormatOut,
		      0, 0L, WAVE_FORMAT_QUERY);
    if (res == MMSYSERR_NOERROR) {
      pos += sprintf(&buf[pos], "%s", " Alaw");
    }
  }
}

void
SnackAudioGetFrequencies(char *device, char *buf, int n)
{
  strncpy(buf, "8000 11025 16000 22050 32000 44100 48000", n);
  buf[n-1] = '\0';
}

void
SnackMixerGetInputJackLabels(char *buf, int n)
{
  buf[0] = '\0';
}

void
SnackMixerGetOutputJackLabels(char *buf, int n)
{
  buf[0] = '\0';
}

void
SnackMixerGetInputJack(char *buf, int n)
{
  buf[0] = '\0';
}

int
SnackMixerSetInputJack(Tcl_Interp *interp, char *jack, char *status)
{
  return 1;
}

void
SnackMixerGetOutputJack(char *buf, int n)
{
  buf[0] = '\0';
}

void
SnackMixerSetOutputJack(char *jack, char *status)
{
}

void
SnackMixerGetChannelLabels(char *line, char *buf, int n)
{
  strncpy(buf, "Left Right", n);
}

void
SnackMixerGetVolume(char *line, int channel, char *buf, int n)
{
  unsigned long vol = 0;
  unsigned int left, right;
  
  waveOutGetVolume((HWAVEOUT) WAVE_MAPPER, &vol);

  left  = (unsigned int) ((vol & 0xffff) / 655.35 +.5);
  right = (unsigned int) (((vol & 0xffff0000) >> 16) / 655.35 +.5);
  if (channel == 0) {
    sprintf(buf, "%d", left);
  } else if (channel == 1) {
    sprintf(buf, "%d", right);
  } else if (channel == -1) {
    sprintf(buf, "%d", (left + right)/2); 
  }
}

void
SnackMixerSetVolume(char *line, int channel, int volume)
{
  int tmp = min(max(volume, 0), 100);
  unsigned int vol = (unsigned int) (((tmp << 16) + tmp) * 655.35);
  unsigned long oldVol = 0;

  if (channel == 0) {
    vol = (unsigned int) (tmp * 655.35 +.5);
  }
  if (channel == 1) {
    vol = (unsigned int) ((tmp << 16) * 655.35 +.5);
  }

  waveOutGetVolume((HWAVEOUT) WAVE_MAPPER, &oldVol);

  if (channel == 0) {
    vol = (oldVol & 0xffff0000) | (vol & 0x0000ffff);
  }
  if (channel == 1) {
    vol = (vol & 0xffff0000) | (oldVol & 0x0000ffff);
  }

  waveOutSetVolume((HWAVEOUT) WAVE_MAPPER, vol);
}

void
SnackMixerLinkJacks(Tcl_Interp *interp, char *jack, Tcl_Obj *var)
{
}

static char *
VolumeVarProc(ClientData clientData, Tcl_Interp *interp, char *name1,
	      char *name2, int flags)
{
  MixerLink *mixLink = (MixerLink *) clientData;
  char *stringValue;
  
  if (flags & TCL_TRACE_UNSETS) {
    if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
      Tcl_Obj *obj, *var;
      char tmp[VOLBUFSIZE];

      SnackMixerGetVolume(mixLink->mixer, mixLink->channel, tmp, VOLBUFSIZE);
      obj = Tcl_NewIntObj(atoi(tmp));
      var = Tcl_NewStringObj(mixLink->mixerVar, -1);
      Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY | TCL_PARSE_PART1);
      Tcl_TraceVar(interp, mixLink->mixerVar,
		   TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		   VolumeVarProc, (int *)mixLink);
    }
    return (char *) NULL;
  }
  stringValue = Tcl_GetVar(interp, mixLink->mixerVar, TCL_GLOBAL_ONLY);
  if (stringValue != NULL) {
    SnackMixerSetVolume(mixLink->mixer, mixLink->channel, atoi(stringValue));
  }

  return (char *) NULL;
}

void
SnackMixerLinkVolume(Tcl_Interp *interp, char *line, int n,
		     Tcl_Obj *CONST objv[])
{
  char *mixLabels[] = { "Play" };
  int i, j, channel;
  char *value, tmp[VOLBUFSIZE];

  for (i = 0; i < SNACK_NUMBER_MIXERS; i++) {
    if (strnicmp(line, mixLabels[i], strlen(line)) == 0) {
      for (j = 0; j < n; j++) {
	if (n == 1) {
	  channel = -1;
	} else {
	  channel = j;
	}
	mixerLinks[i][j].mixer = (char *)SnackStrDup(line);
	mixerLinks[i][j].mixerVar = (char *)SnackStrDup(Tcl_GetStringFromObj(objv[j+3], NULL));
	mixerLinks[i][j].channel = j;
	value = Tcl_GetVar(interp, mixerLinks[i][j].mixerVar, TCL_GLOBAL_ONLY);
	if (value != NULL) {
	  SnackMixerSetVolume(line, channel, atoi(value));
	} else {
	  Tcl_Obj *obj;
	  SnackMixerGetVolume(line, channel, tmp, VOLBUFSIZE);
	  obj = Tcl_NewIntObj(atoi(tmp));
	  Tcl_ObjSetVar2(interp, objv[j+3], NULL, obj, 
			 TCL_GLOBAL_ONLY | TCL_PARSE_PART1);
	}
	Tcl_TraceVar(interp, mixerLinks[i][j].mixerVar,
		     TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		     VolumeVarProc, (ClientData) &mixerLinks[i][j]);
      }
    }
  }
}

void
SnackMixerUpdateVars(Tcl_Interp *interp)
{
  int i, j;
  char tmp[VOLBUFSIZE];
  Tcl_Obj *obj, *var;

  for (i = 0; i < SNACK_NUMBER_MIXERS; i++) {
    for (j = 0; j < 2; j++) {
      if (mixerLinks[i][j].mixerVar != NULL) {
	SnackMixerGetVolume(mixerLinks[i][j].mixer, mixerLinks[i][j].channel,
			    tmp, VOLBUFSIZE);
	obj = Tcl_NewIntObj(atoi(tmp));
	var = Tcl_NewStringObj(mixerLinks[i][j].mixerVar, -1);
	Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY|TCL_PARSE_PART1);
      }
    }
  }
}

void
SnackMixerGetLineLabels(char *buf, int n)
{
  /*
  int i, j, pos = 0, connections;
  MIXERLINE mxl;
  MMRESULT mmr;

  if (mixerGetDevCaps((UINT)selectedMixer, &wMixCaps, sizeof(MIXERCAPS)) ==0) {
    for (i = 0; i < (int) wMixCaps.cDestinations; i++) {
      mxl.cbStruct = sizeof(mxl);
      mxl.dwDestination = i;
      mmr = mixerGetLineInfo((HMIXEROBJ)selectedMixer, &mxl,
			     MIXER_GETLINEINFOF_DESTINATION);
      connections = (int) mxl.cConnections;
      for (j = 0; j < connections; j++) {
	mxl.cbStruct = sizeof(mxl);
	mxl.dwDestination = i;
	mxl.dwSource = j;
	mmr = mixerGetLineInfo((HMIXEROBJ)selectedMixer, &mxl,
			       MIXER_GETLINEINFOF_SOURCE);
	pos += sprintf(&buf[pos], "%s ", (LPSTR) mxl.szShortName);
      }
    }
  }
*/
  strncpy(buf, "Play", n);
  buf[n-1] = '\0';
}

int
SnackGetInputDevices(char **arr, int n)
{
  int i, j = 0;

  for (i = 0; i < numInDevs; i++) {
    if (j < n) {
      arr[j++] = (char *) SnackStrDup(inDeviceList[i]);
    }
  }
  for (i = 0; i < numDSInDevs; i++) {
    if (j < n) {
      arr[j++] = (char *) SnackStrDup(DSInDeviceList[i]);
    }
  }

  return(j);
}

int
SnackGetOutputDevices(char **arr, int n)
{
  int i, j = 0;

  for (i = 0; i < numOutDevs; i++) {
    if (j < n) {
      arr[j++] = (char *) SnackStrDup(outDeviceList[i]);
    }
  }
  for (i = 0; i < numDSOutDevs; i++) {
    if (j < n) {
      arr[j++] = (char *) SnackStrDup(DSOutDeviceList[i]);
    }
  }

  return(j);
}

int
SnackGetMixerDevices(char **arr, int n)
{
  int i, j = 0;

  for (i = 0; i < numMixDevs; i++) {
    if (j < n) {
      arr[j++] = (char *) SnackStrDup(mixerDeviceList[i]);
    }
  }

  return(j);
}

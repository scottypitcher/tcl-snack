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
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

extern void Snack_WriteLog(char *s);
extern void Snack_WriteLogInt(char *s, int n);

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define NBUFS 64

HWAVEOUT      hWaveOut;
HWAVEIN       hWaveIn;
WAVEFORMATEX  wFormat;
WAVEHDR       waveHdr[NBUFS];
HANDLE        Block[NBUFS];
HPSTR	      lpBlock[NBUFS];
int           BlockSize[NBUFS];
/*static ADesc  winad;*/
static int    correction = 1;

#define SNACK_NUMBER_MIXERS 1

struct MixerLink mixerLinks[SNACK_NUMBER_MIXERS][2];

int
SnackAudioOpen(ADesc *A, Tcl_Interp *interp, int mode, int freq,
	       int nchannels, int encoding)
{
  int res = 0, i, size;

  if (A->debug == 1) Snack_WriteLog("Enter SnackAudioOpen\n");

  A->mode = mode;
  A->curr = 0;
  A->freq = freq;
  A->nChannels = nchannels;
  A->shortRead = 0;

  switch (encoding) {
  case LIN16:
    A->bytesPerSample = sizeof(short);
    wFormat.wFormatTag = WAVE_FORMAT_PCM;
    break;
  case ALAW:
    A->bytesPerSample = sizeof(char);
    wFormat.wFormatTag = WAVE_FORMAT_ALAW;
    break;
  case MULAW:
    A->bytesPerSample = sizeof(char);
    wFormat.wFormatTag = WAVE_FORMAT_MULAW;
    break;
  case LIN8OFFSET:
    A->bytesPerSample = sizeof(char);
    wFormat.wFormatTag = WAVE_FORMAT_PCM;
    break;
  }
  wFormat.nChannels       = nchannels;
  wFormat.nSamplesPerSec  = freq;
  wFormat.nAvgBytesPerSec = freq * A->bytesPerSample * nchannels;
  wFormat.nBlockAlign     = A->bytesPerSample * nchannels;
  wFormat.wBitsPerSample  = A->bytesPerSample * 8;
  wFormat.cbSize          = 0;
  
  size = A->bytesPerSample * freq / 16;

  switch (mode) {
  case RECORD:
    res = waveInOpen(&hWaveIn, WAVE_MAPPER, 
		     (WAVEFORMATEX *)&wFormat, 0, 0L, CALLBACK_NULL);
    if (res) {
      Tcl_AppendResult(interp, "waveInOpen failed!", NULL);
      return TCL_ERROR;
    }
    for (i = 0; i < NBUFS; i++) {
      Block[i] = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size);
      if (!Block[i]) {
	Tcl_AppendResult(interp, "Failed allocating audio block.", NULL);
	return TCL_ERROR;
      }
      lpBlock[i] = GlobalLock(Block[i]);

      waveHdr[i].lpData = lpBlock[i];
      waveHdr[i].dwBufferLength = size;
      waveHdr[i].dwFlags = 0L;
      waveHdr[i].dwLoops = 0L;
      res = waveInPrepareHeader(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
      res = waveInAddBuffer(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
    }
    res = waveInStart(hWaveIn);
    break;
    
  case PLAY:
    res = waveOutOpen(&hWaveOut, WAVE_MAPPER, 
		      (WAVEFORMATEX *)&wFormat, 0, 0L, CALLBACK_NULL);
    if (res) {
      Tcl_AppendResult(interp, "waveOutOpen failed!", NULL);
      return TCL_ERROR;
    }
    for (i = 0; i < NBUFS; i++) {
      Block[i] = NULL;
      BlockSize[i] = 0;
      waveHdr[i].dwFlags = WHDR_DONE;
    }
    break;
  }

  if (A->debug == 1) Snack_WriteLogInt("Correction", correction);
  if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioOpen", A->bytesPerSample);

  return TCL_OK;
}

int
SnackAudioClose(ADesc *A)
{
  int i = 0, res;

  if (A->debug == 1) Snack_WriteLog("Enter SnackAudioClose\n");

  switch (A->mode) {
  case RECORD:
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);
    waveInClose(hWaveIn);
    for (i = 0; i < NBUFS; i++) {
      waveInUnprepareHeader(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
      GlobalUnlock(lpBlock[i]);
      GlobalFree(Block[i]);
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
      waveOutUnprepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
      if (Block[i]) {
	GlobalUnlock(lpBlock[i]);
	GlobalFree(Block[i]);
      }
    }
    if (A->debug == 1) Snack_WriteLog("waveOutUnprepareHeader ok\n");
    A->mode = 0;
    break;
  }

  if (A->debug == 1) Snack_WriteLog("Exit SnackAudioClose\n");

  return(0);
}

int
SnackAudioPause(ADesc *A)
{
  int res;
  switch (A->mode) {
  case RECORD:
    break;
    
  case PLAY:
    res = waveOutPause(hWaveOut);
    break;
  }
  return(-1);
}

void
SnackAudioResume(ADesc *A)
{
  int res;
  switch (A->mode) {
  case RECORD:
    break;
    
  case PLAY:
    res = waveOutRestart(hWaveOut);
    break;
  }
}

void
SnackAudioFlush(ADesc *A)
{
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

void
SnackAudioPost(ADesc *A)
{
}

int
SnackAudioRead(ADesc *A, void *buf, int nSamples)
{
  int res = 0, i, noread = 1, nsamps, n = 0;

  if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioRead", nSamples);

  for (i = 0; i < NBUFS; i++) {
    if (waveHdr[i].dwFlags & WHDR_DONE) noread = 0;
  }
  if (noread) return(0);
  
  if (A->shortRead > 0) {
    int offset = A->shortRead * (A->bytesPerSample * A->nChannels);
    int rest = waveHdr[A->curr].dwBytesRecorded - offset;

    memcpy((char *)buf, lpBlock[A->curr] + offset, rest);
    waveInUnprepareHeader(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
    waveHdr[A->curr].lpData = lpBlock[A->curr];
    waveHdr[A->curr].dwBufferLength = A->bytesPerSample * A->freq / 16;
    waveHdr[A->curr].dwFlags = 0L;
    waveHdr[A->curr].dwLoops = 0L;
    res = waveInPrepareHeader(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
    res = waveInAddBuffer(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
    A->curr = (A->curr + 1) % NBUFS;
    A->shortRead = 0;
    n = rest / (A->bytesPerSample * A->nChannels);
    if (A->debug == 1) Snack_WriteLogInt("short read rest", n);
  }

  while ((waveHdr[A->curr].dwFlags & WHDR_DONE) && (n < nSamples)) {
    if ((unsigned int)(nSamples - n) * (A->bytesPerSample * A->nChannels) < waveHdr[A->curr].dwBytesRecorded) {
      nsamps = nSamples - n;
      A->shortRead = nsamps;
      memcpy(((char *)buf + n * A->bytesPerSample * A->nChannels),
	     lpBlock[A->curr], nsamps * (A->bytesPerSample * A->nChannels));
      if (A->debug == 1) Snack_WriteLogInt("short read", nsamps);
    } else {
      memcpy(((char *)buf + n * A->bytesPerSample * A->nChannels),
	     lpBlock[A->curr], waveHdr[A->curr].dwBytesRecorded);
      nsamps = waveHdr[A->curr].dwBytesRecorded / (A->bytesPerSample * A->nChannels);
      waveInUnprepareHeader(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
      waveHdr[A->curr].lpData = lpBlock[A->curr];
      waveHdr[A->curr].dwBufferLength = A->bytesPerSample * A->freq / 16;
      waveHdr[A->curr].dwFlags = 0L;
      waveHdr[A->curr].dwLoops = 0L;
      res = waveInPrepareHeader(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
      if (A->debug == 1) Snack_WriteLogInt("waveInPrepareHeader", res);
      res = waveInAddBuffer(hWaveIn, &waveHdr[A->curr], sizeof(WAVEHDR));
      if (A->debug == 1) Snack_WriteLogInt("waveInAddBuffer", res);
      A->curr = (A->curr + 1) % NBUFS;
      A->shortRead = 0;
    }
    n += nsamps;
  }
  if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioRead", res);

  return(n);
}

int
SnackAudioWrite(ADesc *A, void *buf, int nSamples)
{
  int res, i, nowrit = 1, size = nSamples * A->bytesPerSample * A->nChannels;

  if (A->debug == 1) Snack_WriteLogInt("Enter SnackAudioWrite", nSamples);

  if (nSamples == 0) return(0); 
  for (i = 0; i < NBUFS; i++) {
    if (waveHdr[i].dwFlags & WHDR_DONE) nowrit = 0;
  }
  if (nowrit) return(0); 
  
  for (i = 0; i < NBUFS; i++) {
    if (waveHdr[i].dwFlags & WHDR_DONE) {
      if (size > BlockSize[i]) {
	if (BlockSize[i]) {
	  GlobalUnlock(lpBlock[i]);
	  GlobalFree(Block[i]);
	}
if (A->debug == 1) Snack_WriteLogInt("Allocing", size);
	Block[i] = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size);
	if (!Block[i]) return(0);
	lpBlock[i] = GlobalLock(Block[i]);
	BlockSize[i] = size;
      } else {
if (A->debug == 1) Snack_WriteLogInt("Reusing", size);
if (A->debug == 1) Snack_WriteLogInt("Block", i);
      }
      waveOutUnprepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
      memcpy(lpBlock[i], buf, size);
      waveHdr[i].lpData = lpBlock[i];
      waveHdr[i].dwBufferLength = size;
      waveHdr[i].dwFlags = 0L;
      waveHdr[i].dwLoops = 0L;
      res = waveOutPrepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
      res = waveOutWrite(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
      break;
    }
  }

  if (A->debug == 1) Snack_WriteLogInt("Exit SnackAudioWrite", res);

  return(nSamples);
}

int
SnackAudioReadable(ADesc *A)
{
  int i, n = 0;
  
  for (i = 0; i < NBUFS; i++) {
    if (waveHdr[i].dwFlags & WHDR_DONE) {
      n += (waveHdr[i].dwBufferLength / (A->bytesPerSample * A->nChannels));
    }
  }
  if (A->shortRead > 0) {
    n -= (waveHdr[A->curr].dwBufferLength / (A->bytesPerSample * A->nChannels));
    n += A->shortRead;
  }
  return(n);
}

int
SnackAudioWriteable(ADesc *A)
{
  int res = 0, i;

  for (i = 0; i < NBUFS; i++) {
    if (waveHdr[i].dwFlags & WHDR_DONE) res = -1;
  }
  
  return(res);
}

int
SnackAudioPlayed(ADesc *A)
{
  MMTIME mmtime;
  
  mmtime.wType = TIME_SAMPLES;
  waveOutGetPosition(hWaveOut, &mmtime, sizeof(MMTIME));

  if ((wFormat.wFormatTag == WAVE_FORMAT_MULAW) ||
      (wFormat.wFormatTag == WAVE_FORMAT_ALAW)) {
    return(mmtime.u.sample / correction);
  }

  return(mmtime.u.sample);
}

void
SnackMixerOpen()
{
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

void
ASetRecGain(int gain)
{
  int g = min(max(gain, 0), 100);

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
SnackAudioGetFormats(char *buf, int n)
{
  strncpy(buf, "Lin16 Lin8offset Mulaw Alaw", n);
  buf[n-1] = '\0';
}

void
SnackAudioGetFrequencies(char *buf, int n)
{
  strncpy(buf, "8000 11025 16000 22050 32000 44100 48000", n);
  buf[n-1] = '\0';
}

void
SnackMixerGetInputJacks(char *buf, int n)
{
  buf[0] = '\0';
}

void
SnackMixerGetOutputJacks(char *buf, int n)
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
SnackMixerGetNumChannels(char *mixer, char *buf, int n)
{
  strncpy(buf, "Left Right", n);
  buf[n-1] = '\0';
}

void
SnackMixerGetVolume(char *mixer, int channel, char *buf, int n)
{
  unsigned int vol = 0, left, right;
  
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
SnackMixerSetVolume(char *mixer, int channel, int volume)
{
  int tmp = min(max(volume, 0), 100);
  unsigned int vol = (unsigned int) (((tmp << 16) + tmp) * 655.35), oldVol = 0;

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
SnackMixerLinkVolume(Tcl_Interp *interp, char *mixer, int n,
		     Tcl_Obj *CONST objv[])
{
  char *mixLabels[] = { "Play" };
  int i, j, channel;
  char *value, tmp[VOLBUFSIZE];

  for (i = 0; i < SNACK_NUMBER_MIXERS; i++) {
    if (strnicmp(mixer, mixLabels[i], strlen(mixer)) == 0) {
      for (j = 0; j < n; j++) {
	if (n == 1) {
	  channel = -1;
	} else {
	  channel = j;
	}
	mixerLinks[i][j].mixer = (char *)strdup(mixer);
	mixerLinks[i][j].mixerVar = (char *)strdup(Tcl_GetStringFromObj(objv[j+3], NULL));
	mixerLinks[i][j].channel = j;
	value = Tcl_GetVar(interp, mixerLinks[i][j].mixerVar, TCL_GLOBAL_ONLY);
	if (value != NULL) {
	  SnackMixerSetVolume(mixer, channel, atoi(value));
	} else {
	  Tcl_Obj *obj;
	  SnackMixerGetVolume(mixer, channel, tmp, VOLBUFSIZE);
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
SnackMixerGetMixers(char *buf, int n)
{
  strncpy(buf, "Play", n);
  buf[n-1] = '\0';
}

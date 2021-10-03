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
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#define DEVICE_NAME "/dev/dsp"
#define MIXER_NAME  "/dev/mixer"

extern void Snack_WriteLog(char *s);
extern void Snack_WriteLogInt(char *s, int n);

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#endif

int mfd = 0;

struct MixerLink mixerLinks[SOUND_MIXER_NRDEVICES][2];

int
SnackAudioOpen(ADesc *A, Tcl_Interp *interp, int mode, int freq,
	       int nchannels, int encoding)
{
  int format;
  int nformat;
  int channels;
  int speed;
  int mask;
  /*  int frag = 0x7fff0010;*/

  A->mode = mode;
  switch (mode) {
  case RECORD:
    if ((A->afd = open(DEVICE_NAME, O_RDONLY, 0)) == -1) {
      Tcl_AppendResult(interp, "Couldn't open ", DEVICE_NAME, " for read.",
		       NULL);
      return TCL_ERROR;
    }
    break;

  case PLAY:
    if ((A->afd = open(DEVICE_NAME, O_WRONLY, 0)) == -1) {
      Tcl_AppendResult(interp, "Couldn't open ", DEVICE_NAME, " for write.",
		       NULL);
      return TCL_ERROR;
    }
    break;
  }

  fcntl(A->afd, F_SETFD, FD_CLOEXEC);

  /*  if (ioctl(A->afd, SNDCTL_DSP_SETFRAGMENT, &frag)) return(-1);*/

  ioctl(A->afd, SNDCTL_DSP_GETFMTS, &mask);

  A->convert = 0;

  switch (encoding) {
  case LIN16:
    format = AFMT_S16_LE;
    A->bytesPerSample = sizeof(short);
    break;
  case ALAW:
    if (mask & AFMT_A_LAW) {
      format = AFMT_A_LAW;
      A->bytesPerSample = sizeof(char);
    } else {
      format = AFMT_S16_LE;
      A->bytesPerSample = sizeof(short);
      A->convert = ALAW;
    }
    break;
  case MULAW:
    if (mask & AFMT_MU_LAW) {
      format = AFMT_MU_LAW;
      A->bytesPerSample = sizeof(char);
    } else {
      format = AFMT_S16_LE;
      A->bytesPerSample = sizeof(short);
      A->convert = MULAW;
    }
    break;
  case LIN8OFFSET:
    format = AFMT_U8;
    A->bytesPerSample = sizeof(char);
    break;
  case LIN8:
    format = AFMT_S8;
    A->bytesPerSample = sizeof(char);
    break;
  }

  nformat = format;
  if (ioctl(A->afd, SNDCTL_DSP_SETFMT, &format) == -1
      || format != nformat) {
    close(A->afd);
    Tcl_AppendResult(interp, "Failed setting format.", NULL);
    return TCL_ERROR;
  }

  A->nChannels = nchannels;
  channels = nchannels;
  if (ioctl(A->afd, SNDCTL_DSP_CHANNELS, &channels) == -1
      || channels != nchannels) {
    close(A->afd);
    Tcl_AppendResult(interp, "Failed setting number of channels.", NULL);
    return TCL_ERROR;
  }

  speed = freq;
  if (ioctl(A->afd, SNDCTL_DSP_SPEED, &speed) == -1
      || abs(speed - freq) > freq / 100) {
    close(A->afd);
    Tcl_AppendResult(interp, "Failed setting sample frequency.", NULL);
    return TCL_ERROR;
  }

  A->count = 0;
  /*
  A->frag_size = 0;
  ioctl(A->afd, SNDCTL_DSP_GETBLKSIZE, &A->frag_size);
  printf("Frag size: %d\n",  A->frag_size);
  if (A->frag_size < 16384) {
    A->frag_size = 0;
  }*/
  A->time = SnackCurrentTime();
  A->timep = 0.0;
  A->freq = freq;

  return TCL_OK;
}

int
SnackAudioClose(ADesc *A)
{
  A->count = 0;
  close(A->afd);

  return(0);
}

int
SnackAudioPause(ADesc *A)
{
  /*
  A->count = APlayed(A) + A->frag_size;
  ioctl(A->afd, SNDCTL_DSP_RESET, 0);
  return(A->count);
  */
  int res = SnackAudioPlayed(A);

  A->timep = SnackCurrentTime();
  ioctl(A->afd, SNDCTL_DSP_RESET, 0);

  return(res);
}

void
SnackAudioResume(ADesc *A)
{
  A->time = A->time + SnackCurrentTime() - A->timep;
}

void
SnackAudioFlush(ADesc *A)
{
  if (A->mode == RECORD) {
  } else {
    ioctl(A->afd, SNDCTL_DSP_RESET, 0);
  }
}

void
SnackAudioPost(ADesc *A)
{
  ioctl(A->afd, SNDCTL_DSP_POST, 0);
}

int
SnackAudioRead(ADesc *A, void *buf, int nSamples)
{
  if (A->convert) {
    int n = 0, i, res;
    short s[2];

    for (i = 0; i < nSamples * A->nChannels; i += A->nChannels) {
      res = read(A->afd, &s, A->nChannels * sizeof(short));
      if (res <= 0) return(n / (A->bytesPerSample * A->nChannels));
      if (A->convert == ALAW) {
	((unsigned char *)buf)[i] = Snack_Lin2Alaw(s[0]);
	if (A->nChannels == 2) {
	  ((unsigned char *)buf)[i+1] = Snack_Lin2Alaw(s[1]);
	}
      } else {
	((unsigned char *)buf)[i] = Snack_Lin2Mulaw(s[0]);
	if (A->nChannels == 2) {
	  ((unsigned char *)buf)[i+1] = Snack_Lin2Mulaw(s[1]);
	}
      }
      n += res;
    }

    return(n / (A->bytesPerSample * A->nChannels));
  } else {
    int n = read(A->afd, buf, nSamples * A->bytesPerSample * A->nChannels);
    if (n > 0) n /= (A->bytesPerSample * A->nChannels);

    return(n);
  }
}

int
SnackAudioWrite(ADesc *A, void *buf, int nSamples)
{
  if (A->convert) {
    int n = 0, i, res;
    short s;

    for (i = 0; i < nSamples * A->nChannels; i++) {
      if (A->convert == ALAW) {
	s = Snack_Alaw2Lin(((unsigned char *)buf)[i]);
      } else {
	s = Snack_Mulaw2Lin(((unsigned char *)buf)[i]);
      }
      res = write(A->afd, &s, sizeof(short));
      if (res <= 0) return(n / (A->bytesPerSample * A->nChannels));
      n += res;
    }

    return(n / (A->bytesPerSample * A->nChannels));
  } else {
    int n = write(A->afd, buf, nSamples * A->bytesPerSample * A->nChannels); 
    if (n > 0) n /= (A->bytesPerSample * A->nChannels);
    
    return(n);
  }
}

int
SnackAudioReadable(ADesc *A)
{
  audio_buf_info info;

  ioctl(A->afd, SNDCTL_DSP_GETISPACE, &info);
  return (info.bytes / (A->bytesPerSample * A->nChannels));
}

int
SnackAudioWriteable(ADesc *A)
{
  audio_buf_info info;

  ioctl(A->afd, SNDCTL_DSP_GETOSPACE, &info);

  return (info.bytes / (A->bytesPerSample * A->nChannels));
}

int
SnackAudioPlayed(ADesc *A)
{
  /*
    count_info info;
    
    ioctl(A->afd, SNDCTL_DSP_GETOPTR, &info);
    
    return(A->count + (info.bytes / (A->bytesPerSample * A->nChannels)) - 
    A->frag_size);
    */
  int res;
  
  res = (A->freq * (SnackCurrentTime() - A->time) +.5);
  return(res);
}

void
SnackMixerOpen()
{
  if ((mfd = open(MIXER_NAME, O_RDWR, 0)) == -1) {
    printf("Unable to open mixer\n");
  }
}

void
ASetRecGain(int gain)
{
  int g = min(max(gain, 0), 100);
  int recsrc = 0;

  g = g * 256 + g;
  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recsrc);
  if (recsrc & SOUND_MASK_LINE) {
    ioctl(mfd, SOUND_MIXER_WRITE_LINE, &g);
  } else {
    ioctl(mfd, SOUND_MIXER_WRITE_MIC, &g);
  }
}

void
ASetPlayGain(int gain)
{
  int g = min(max(gain, 0), 100);
  int pcm_gain = 25700;

  g = g * 256 + g;
  ioctl(mfd, SOUND_MIXER_WRITE_VOLUME, &g);
  ioctl(mfd, SOUND_MIXER_WRITE_PCM, &pcm_gain);
}

int
AGetRecGain()
{
  int g = 0, left, right, recsrc = 0;

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recsrc);
  if (recsrc & SOUND_MASK_LINE) {
    ioctl(mfd, SOUND_MIXER_READ_LINE, &g);
  } else {
    ioctl(mfd, SOUND_MIXER_READ_MIC, &g);
  }
  left  =  g & 0xff;
  right = (g & 0xff00) / 256;
  g = (left + right) / 2;

  return(g);
}

int
AGetPlayGain()
{
  int g = 0, left, right;
  
  ioctl(mfd, SOUND_MIXER_READ_VOLUME, &g);
  left  =  g & 0xff;
  right = (g & 0xff00) / 256;
  g = (left + right) / 2;

  return(g);
}

void
SnackAudioGetFormats(char *buf, int n)
{
  int afd, mask, pos = 0;

  if ((afd = open(DEVICE_NAME, O_WRONLY, 0)) == -1) {
    buf[0] = '\0';
    return;
  }
  if (ioctl(afd, SNDCTL_DSP_GETFMTS, &mask) == -1) {
    buf[0] = '\0';
    return;
  }
  close(afd);

  if (mask & AFMT_S16_LE) {
    pos += sprintf(&buf[pos], "%s", "Lin16");
  }
  if (mask & AFMT_U8) {
    pos += sprintf(&buf[pos], "%s", " Lin8offset");
  }
  if (mask & AFMT_S8) {
    pos += sprintf(&buf[pos], "%s", " Lin8");
  }
  pos += sprintf(&buf[pos], "%s", " Mulaw Alaw");
}

void
SnackAudioGetFrequencies(char *buf, int n)
{
  int afd, freq, pos = 0, i;
  int f[] = { 8000, 11025, 16000, 22050, 32000, 44100, 48000 };

  if ((afd = open(DEVICE_NAME, O_WRONLY, 0)) == -1) {
    buf[0] = '\0';
    return;
  }
  for (i = 0; i < 7; i++) {
    freq = f[i];
    if (ioctl(afd, SNDCTL_DSP_SPEED, &freq) == -1) break;
    if (abs(f[i] - freq) > freq / 100) continue;
    pos += sprintf(&buf[pos], "%d ", freq);
  }
  close(afd);
}

void
SnackMixerGetInputJacks(char *buf, int n)
{
  char *jackLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, recMask, pos = 0;

  if (mfd != -1) {
    ioctl(mfd, SOUND_MIXER_READ_RECMASK, &recMask);
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
      if ((1 << i) & recMask) {
	pos += sprintf(&buf[pos], "%s", jackLabels[i]);
	pos += sprintf(&buf[pos], " ");
      }
    }
  } else {
    buf[0] = '\0';
  }
  buf[n-1] = '\0';
}

void
SnackMixerGetOutputJacks(char *buf, int n)
{
  buf[0] = '\0';
}

void
SnackMixerGetInputJack(char *buf, int n)
{
  char *jackLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, recSrc = 0, pos = 0;

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recSrc);
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if ((1 << i) & recSrc) {
      pos += sprintf(&buf[pos], "%s", jackLabels[i]);
      while (isspace(buf[pos-1])) pos--;
      pos += sprintf(&buf[pos], " ");
    }
  }
  if(isspace(buf[pos-1])) pos--;
  buf[pos] = '\0';
}

int
SnackMixerSetInputJack(Tcl_Interp *interp, char *jack, char *status)
{
  char *jackLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, recSrc = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(jack, jackLabels[i], strlen(jack)) == 0) {
      if (strcmp(status, "0")) {
	recSrc = 1 << i;

	if (ioctl(mfd, SOUND_MIXER_WRITE_RECSRC, &recSrc) == -1) {
	  return 1;
	} else {
	  return 0;
	};
	break;
      }
    }
  }
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

static int bar = 0;

static char *
JackVarProc(ClientData clientData, Tcl_Interp *interp, char *name1,
	    char *name2, int flags)
{
  MixerLink *mixLink = (MixerLink *) clientData;
  char *jackLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, recSrc = 0, status = 0;
  char *stringValue;
  Tcl_Obj *obj, *var;

  if (bar) return (char *) NULL;

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recSrc);

  if (flags & TCL_TRACE_UNSETS) {
    if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
      for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
	if (strncasecmp(mixLink->jack, jackLabels[i], strlen(mixLink->jack))
	    == 0) {
	  if ((1 << i) & recSrc) {
	    status = 1;
	  } else {
	    status = 0;
	  }
	  break;
	}
      }
      obj = Tcl_NewIntObj(status);
      var = Tcl_NewStringObj(mixLink->jackVar, -1);
      Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY | TCL_PARSE_PART1);
      Tcl_TraceVar(interp, mixLink->jackVar,
		   TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		   JackVarProc, mixLink);
    }
    return (char *) NULL;
  }

  stringValue = Tcl_GetVar(interp, mixLink->jackVar, TCL_GLOBAL_ONLY);
  if (stringValue != NULL) {
    SnackMixerSetInputJack(interp, mixLink->jack, stringValue);
  }

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recSrc);

  bar = 1;
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (mixerLinks[i][0].jackVar != NULL) {
      if ((1 << i) & recSrc) {
	status = 1;
      } else {
	status = 0;
      }
      obj = Tcl_NewIntObj(status);
      var = Tcl_NewStringObj(mixerLinks[i][0].jackVar, -1);
      Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY |TCL_PARSE_PART1);
    }
  }
  bar = 0;

  return (char *) NULL;
}

void
SnackMixerLinkJacks(Tcl_Interp *interp, char *jack, Tcl_Obj *var)
{
  char *jackLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, recSrc = 0, status;
  char *value;

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recSrc);

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(jack, jackLabels[i], strlen(jack)) == 0) {
      if ((1 << i) & recSrc) {
	status = 1;
      } else {
	status = 0;
      }
      mixerLinks[i][0].jack = strdup(jack);
      mixerLinks[i][0].jackVar = strdup(Tcl_GetStringFromObj(var, NULL));
      value = Tcl_GetVar(interp, mixerLinks[i][0].jackVar, TCL_GLOBAL_ONLY);
      if (value != NULL) {
	SnackMixerSetInputJack(interp, mixerLinks[i][0].jack, value);
      } else {
	Tcl_Obj *obj = Tcl_NewIntObj(status);
	Tcl_ObjSetVar2(interp, var, NULL, obj, 
		       TCL_GLOBAL_ONLY | TCL_PARSE_PART1);

      }
      Tcl_TraceVar(interp, mixerLinks[i][0].jackVar,
		   TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		   JackVarProc, (ClientData) &mixerLinks[i][0]);
      break;
    }
  }
}

void
SnackMixerGetNumChannels(char *mixer, char *buf, int n)
{
  char *mixLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, devMask;

  ioctl(mfd, SOUND_MIXER_READ_STEREODEVS, &devMask);
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(mixer, mixLabels[i], strlen(mixer)) == 0) {
      if (devMask & (1 << i)) {
	sprintf(buf, "Left Right");
      } else {
	sprintf(buf, "Mono");
      }
      break;
    }
  }
}

void
SnackMixerGetVolume(char *mixer, int channel, char *buf, int n)
{
  char *mixLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, vol = 0, devMask, isStereo = 0, left, right;

  buf[0] = '\0';

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(mixer, mixLabels[i], strlen(mixer)) == 0) {
      ioctl(mfd, MIXER_READ(i), &vol);
      ioctl(mfd, SOUND_MIXER_READ_STEREODEVS, &devMask);
      if (devMask & (1 << i)) {
	isStereo = 1;
      }
      break;
    }
  }
  left  =  vol & 0xff;
  right = (vol & 0xff00) >> 8;
  if (isStereo) {
    if (channel == 0) {
      sprintf(buf, "%d", left);
    } else if (channel == 1) {
      sprintf(buf, "%d", right);
    } else if (channel == -1) {
      sprintf(buf, "%d", (left + right)/2);
    }
  } else {
    sprintf(buf, "%d", left);
  }
}

void
SnackMixerSetVolume(char *mixer, int channel, int volume)
{
  char *mixLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int tmp = min(max(volume, 0), 100), i, oldVol = 0;
  int vol = (tmp << 8) + tmp;

  if (channel == 0) {
    vol = tmp;
  }
  if (channel == 1) {
    vol = tmp << 8;
  }

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(mixer, mixLabels[i], strlen(mixer)) == 0) {
      ioctl(mfd, MIXER_READ(i), &oldVol);
      if (channel == 0) {
	vol = (oldVol & 0xff00) | (vol & 0x00ff);
      }
      if (channel == 1) {
	vol = (vol & 0xff00) | (oldVol & 0x00ff);
      }
      ioctl(mfd, MIXER_WRITE(i), &vol);
      break;
    }
  }
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
		   VolumeVarProc, mixLink);
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
  char *mixLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, j, channel;
  char *value, tmp[VOLBUFSIZE];

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    if (strncasecmp(mixer, mixLabels[i], strlen(mixer)) == 0) {
      for (j = 0; j < n; j++) {
	if (n == 1) {
	  channel = -1;
	} else {
	  channel = j;
	}
	mixerLinks[i][j].mixer = strdup(mixer);
	mixerLinks[i][j].mixerVar = strdup(Tcl_GetStringFromObj(objv[j+3],NULL));
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
  int i, j, recSrc, status;
  char tmp[VOLBUFSIZE];
  Tcl_Obj *obj, *var;

  ioctl(mfd, SOUND_MIXER_READ_RECSRC, &recSrc);
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
    for (j = 0; j < 2; j++) {
      if (mixerLinks[i][j].mixerVar != NULL) {
	SnackMixerGetVolume(mixerLinks[i][j].mixer, mixerLinks[i][j].channel,
			    tmp, VOLBUFSIZE);
	obj = Tcl_NewIntObj(atoi(tmp));
	var = Tcl_NewStringObj(mixerLinks[i][j].mixerVar, -1);
	Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY|TCL_PARSE_PART1);
      }
    }
    if (mixerLinks[i][0].jackVar != NULL) {
      if ((1 << i) & recSrc) {
	status = 1;
      } else {
	status = 0;
      }
      obj = Tcl_NewIntObj(status);
      var = Tcl_NewStringObj(mixerLinks[i][0].jackVar, -1);
      Tcl_ObjSetVar2(interp, var, NULL, obj, TCL_GLOBAL_ONLY | TCL_PARSE_PART1);
    }
  }
}

void
SnackMixerGetMixers(char *buf, int n)
{
  char *mixLabels[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
  int i, devMask, pos = 0;

  if (mfd != -1) {
    ioctl(mfd, SOUND_MIXER_READ_DEVMASK, &devMask);
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
      if ((1 << i) & devMask && pos < n-8) {
	pos += sprintf(&buf[pos], "%s", mixLabels[i]);
	pos += sprintf(&buf[pos], " ");
      }
    }
  } else {
    buf[0] = '\0';
  }
  buf[n-1] = '\0';
}

//
// FhGAACencoder.h
//
// Copyright 2011 tmkk. All rights reserved.
//

#pragma once

#include "common.h"
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#include <sndfile.h>
#include <mmsystem.h>

class FhGAACEncoder
{
  public:
	AudioCoder *encoder;
	FILE *fp;
	SNDFILE *sff;
	__int64 totalFrames;
	PCMType type;
	int samplerate;
	int bitPerSample;
	int channels;

	AudioCoder* (*finishAudio3)(_TCHAR *filename, AudioCoder *coder);
	void (*prepareToFinish)(_TCHAR *filename, AudioCoder *coder);
	AudioCoder* (*createAudio3)(int nch, int srate, int bps, unsigned int srct, unsigned int *outt, char *configfile);

	FhGAACEncoder();
	~FhGAACEncoder();
	bool openFile(_TCHAR *inFile);
	bool openStream(FILE *stream);
	__int64 beginEncode(_TCHAR *outFile, encodingParameters *params);
};

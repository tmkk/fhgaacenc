//
// common.h
//
// Copyright 2011 tmkk. All rights reserved.
//

#pragma once

#define VERSION 20120624

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

#define SWAP32(n) ((((n)>>24)&0xff) | (((n)>>8)&0xff00) | (((n)<<8)&0xff0000) | (((n)<<24)&0xff000000))
#define SWAP16(n) ((((n)>>8)&0xff) | (((n)<<8)&0xff00))

typedef enum
{
	kModeVBR = 0,
	kModeCBR
} codecMode;

typedef enum
{
	kProfileAuto = 0,
	kProfileLC,
	kProfileHE,
	kProfileHEv2
} codecProfile;

typedef struct
{
	_TCHAR *inFile;
	_TCHAR *outFile;
	codecMode mode;
	int modeQuality;
	codecProfile profile;
	bool quiet;
	bool readFromStdin;
	bool ignoreLength;
	bool adtsMode;
} encodingParameters;

typedef enum
{
	kPCMTypeSignedInt = 0,
	kPCMTypeUnsignedInt,
	kPCMTypeFloat
} PCMType;

class AudioCoder
{
  public:
    AudioCoder() { }
    virtual int Encode(int framepos, void *in, int in_avail, int *in_used, void *out, int out_avail)=0; //returns bytes in out
    virtual int hoge(void);
    virtual ~AudioCoder() { };
};

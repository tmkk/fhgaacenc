//
// FhGAACencoder.cpp
//
// Copyright 2011 tmkk. All rights reserved.
//

#include "stdafx.h"
#include "FhGAACencoder.h"

#define SAMPLES_PER_LOOP 1024

static const unsigned char INTEGER_PCM_GUID[16] = {0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};
static const unsigned char FLOAT_PCM_GUID[16]   = {0x03,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};

static int fseek_stdin(FILE *fp, __int64 offset, char **seekBuf, int *seekBufSize)
{
	if(!offset) return 0;
	if(fp!=stdin) return fseeko(fp,offset,SEEK_CUR);
	if(offset > *seekBufSize) {
		*seekBuf = (char *)realloc(*seekBuf, (size_t)offset);
		*seekBufSize = (size_t)offset;
	}
	if(fread(*seekBuf,1,(size_t)offset,fp) < (size_t)offset) return -1;
	return 0;
}

static void optimizeAtoms(FILE *fp, __int64 origSize)
{
	unsigned int tmp;
	unsigned int moovSize;
	char atom[4];
	
	if(!fp) return;
	
	unsigned int bufferSize = 1024*1024;
	char *tmpbuf = (char *)malloc(bufferSize);
	char *tmpbuf2 = (char *)malloc(bufferSize);
	char *read = tmpbuf;
	char *write = tmpbuf2;
	char *swap;
	char *moovbuf = NULL;
	int i;
	bool moov_after_mdat = false;
	
	while(1) { //skip until moov;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"moov",4)) break;
		if(!memcmp(atom,"mdat",4)) moov_after_mdat = true;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	if(!moov_after_mdat) goto end;
	
	__int64 pos_moov = ftello(fp) - 8;
	moovSize = tmp;

	unsigned int bytesRead = 0;
	while(bytesRead < moovSize-8) {
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		bytesRead += tmp;
		if(memcmp(atom,"trak",4)) {
			if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
			continue;
		}

		__int64 pos_next = ftello(fp) + tmp - 8;
		
		while(1) { //skip until mdia;
			if(fread(&tmp,4,1,fp) < 1) goto end;
			if(fread(atom,1,4,fp) < 4) goto end;
			tmp = SWAP32(tmp);
			if(!memcmp(atom,"mdia",4)) break;
			if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
		}
		
		while(1) { //skip until minf;
			if(fread(&tmp,4,1,fp) < 1) goto end;
			if(fread(atom,1,4,fp) < 4) goto end;
			tmp = SWAP32(tmp);
			if(!memcmp(atom,"minf",4)) break;
			if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
		}
		
		while(1) { //skip until stbl;
			if(fread(&tmp,4,1,fp) < 1) goto end;
			if(fread(atom,1,4,fp) < 4) goto end;
			tmp = SWAP32(tmp);
			if(!memcmp(atom,"stbl",4)) break;
			if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
		}
		
		while(1) { //skip until stco;
			if(fread(&tmp,4,1,fp) < 1) goto end;
			if(fread(atom,1,4,fp) < 4) goto end;
			tmp = SWAP32(tmp);
			if(!memcmp(atom,"stco",4)) break;
			if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
		}
		
		int *stco = (int *)malloc(tmp-8);
		if(fread(stco,1,tmp-8,fp) < tmp-8) goto end;
		int nElement = SWAP32(stco[1]);
		
		/* update stco atom */
		for(i=0;i<nElement;i++) {
			stco[2+i] = SWAP32(SWAP32(stco[2+i])+moovSize);
		}
		if(fseeko(fp,8-(int)tmp,SEEK_CUR) != 0) goto end;
		if(fwrite(stco,1,tmp-8,fp) < tmp-8) goto end;
		
		free(stco);

		if(fseeko(fp,pos_next,SEEK_SET) != 0) goto end;
	}
	
	rewind(fp);
	
	/* save moov atom */
	
	moovbuf = (char *)malloc(moovSize);
	if(fseeko(fp,pos_moov,SEEK_SET) != 0) goto end;
	if(fread(moovbuf,1,moovSize,fp) < moovSize) goto end;
	rewind(fp);
	
	while(1) { //skip until ftyp;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"ftyp",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	/* position after ftyp atom is the inserting point */
	if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	pos_moov = ftello(fp);
	
	/* optimize */
	
	__int64 bytesToMove = origSize-pos_moov-moovSize;
	
	if(bytesToMove < moovSize) {
		if(bufferSize < bytesToMove) {
			tmpbuf = (char *)realloc(tmpbuf,(size_t)bytesToMove);
			read = tmpbuf;
		}
		if(fread(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
		if(fseeko(fp,(__int64)moovSize-bytesToMove,SEEK_CUR) != 0) goto end;
		if(fwrite(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
	}
	else if(bytesToMove > bufferSize) {
		if(bufferSize < moovSize) {
			tmpbuf = (char *)realloc(tmpbuf,moovSize);
			tmpbuf2 = (char *)realloc(tmpbuf2,moovSize);
			read = tmpbuf;
			write = tmpbuf2;
			bufferSize = moovSize;
			if(bytesToMove <= bufferSize) goto moveBlock_is_smaller_than_buffer;
		}
		if(fread(write,1,bufferSize,fp) < bufferSize) goto end;
		bytesToMove -= bufferSize;
		while(bytesToMove > bufferSize) {
			if(fread(read,1,bufferSize,fp) < bufferSize) goto end;
			if(fseeko(fp,(__int64)moovSize-2*(__int64)bufferSize,SEEK_CUR) != 0) goto end;
			if(fwrite(write,1,bufferSize,fp) < bufferSize) goto end;
			if(fseeko(fp,(__int64)bufferSize-(__int64)moovSize,SEEK_CUR) != 0) goto end;
			swap = read;
			read = write;
			write = swap;
			bytesToMove -= bufferSize;
			//NSLog(@"DEBUG: %d bytes left",bytesToMove);
		}
		if(fread(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
		if(fseeko(fp,(__int64)moovSize-(__int64)bufferSize-bytesToMove,SEEK_CUR) != 0) goto end;
		if(fwrite(write,1,bufferSize,fp) < bufferSize) goto end;
		if(fwrite(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
	}
	else {
moveBlock_is_smaller_than_buffer:
		if(fread(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
		if(moovSize < bytesToMove) {
			if(fseeko(fp,(__int64)moovSize-bytesToMove,SEEK_CUR) != 0) goto end;
		}
		else {
			if(fseeko(fp,0-bytesToMove,SEEK_CUR) != 0) goto end;
			if(fwrite(moovbuf,1,moovSize,fp) < moovSize) goto end;
		}
		if(fwrite(read,1,(size_t)bytesToMove,fp) < bytesToMove) goto end;
	}
	
	if(fseeko(fp,pos_moov,SEEK_SET) != 0) goto end;
	if(fwrite(moovbuf,1,moovSize,fp) < moovSize) goto end;
	
end:
	if(moovbuf) free(moovbuf);
	free(tmpbuf);
	free(tmpbuf2);
}

static unsigned int getFrequencyAndChannelFromM4a(FILE *fp)
{
	char atom[4];
	int tmp,i;
	__int64 initPos = ftello(fp);
	unsigned int value = 0xffffffff;
	
	if(fseeko(fp,0,SEEK_SET) != 0) goto end;
	
	while(1) { //skip until moov;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"moov",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	while(1) { //skip until trak;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"trak",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	while(1) { //skip until mdia;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"mdia",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	while(1) { //skip until minf;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"minf",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	while(1) { //skip until stbl;
		if(fread(&tmp,4,1,fp) < 1) goto end;
		if(fread(atom,1,4,fp) < 4) goto end;
		tmp = SWAP32(tmp);
		if(!memcmp(atom,"stbl",4)) break;
		if(fseeko(fp,tmp-8,SEEK_CUR) != 0) goto end;
	}
	
	while(1) { //skip until esds;
		if(fread(atom,1,4,fp) < 4) goto end;
		if(!memcmp(atom,"esds",4)) break;
		if(fseeko(fp,-3,SEEK_CUR) != 0) goto end;
	}
	
	if(fseeko(fp,5,SEEK_CUR) != 0) goto end;
	for(i=0;i<3;i++) {
		if(fread(atom,1,1,fp) < 1) goto end;
		if((unsigned char)atom[0] != 0x80) {
			if(fseeko(fp,-1,SEEK_CUR) != 0) goto end;
			break;
		}
	}
	if(fseeko(fp,5,SEEK_CUR) != 0) goto end;
	for(i=0;i<3;i++) {
		if(fread(atom,1,1,fp) < 1) goto end;
		if((unsigned char)atom[0] != 0x80) {
			if(fseeko(fp,-1,SEEK_CUR) != 0) goto end;
			break;
		}
	}
	if(fseeko(fp,15,SEEK_CUR) != 0) goto end;
	for(i=0;i<3;i++) {
		if(fread(atom,1,1,fp) < 1) goto end;
		if((unsigned char)atom[0] != 0x80) {
			if(fseeko(fp,-1,SEEK_CUR) != 0) goto end;
			break;
		}
	}
	if(fseeko(fp,1,SEEK_CUR) != 0) goto end;
	if(fread(atom,1,2,fp) < 2) goto end;
	value = (atom[0]<<1)&0xe;
	value |= (atom[1]>>7)&0x1;
	value <<= 4;
	value |= (atom[1]>>3)&0xf;
	
end:
	fseeko(fp,initPos,SEEK_SET);
	return value;
}

static
int getSamplingRateIndex(int rate)
{
	static const int rtab[] = {
		96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
		16000, 12000, 11025, 8000, 7350, 0
	};
	for (const int *p = rtab; *p; ++p)
		if (*p == rate)
			return p - rtab;
	return 0xf;
}

static
int getChannelConfig(int nchannels)
{
	return nchannels < 7 ? nchannels : 7;
}

FhGAACEncoder::FhGAACEncoder()
{
	encoder = NULL;
	fp = NULL;
	sff = NULL;
	totalFrames = 0;
}

FhGAACEncoder::~FhGAACEncoder()
{
	
}

bool FhGAACEncoder::openStream(FILE *stream)
{
	char chunk[4];
	char *seekBuf = (char *)malloc(256);
	int seekBufSize = 256;
	unsigned int tmp, chunkSize;
	unsigned short tmp2;
	unsigned int channelBitmap;
	bool extensible = false;
	bool ret = false;

	fp = stream;

	if(fread(chunk,1,4,fp) < 4) goto last;
	if(memcmp(chunk,"RIFF",4)) {
		fprintf(stderr,"Error: input stream is not a RIFF WAVE file\n");
		goto last;
	}
	if(fseek_stdin(fp,4,&seekBuf,&seekBufSize)) goto last;
	if(fread(chunk,1,4,fp) < 4) goto last;
	if(memcmp(chunk,"WAVE",4)) {
		fprintf(stderr,"Error: input stream is not a RIFF WAVE file\n");
		goto last;
	}
	while(1) { // find fmt chunk
		if(fread(chunk,1,4,fp) < 4) goto last;
		if(fread(&chunkSize,4,1,fp) < 1) goto last;
		if(!memcmp(chunk,"fmt ",4)) break;
		if(fseek_stdin(fp,chunkSize,&seekBuf,&seekBufSize)) goto last;
	}

	if(fread(&tmp2,2,1,fp) < 1) goto last;
	if(tmp2 == 0x1) {
		type = kPCMTypeSignedInt;
	}
	/*else if(tmp2 == 0x3) {
		PCMType = kPCMTypeFloat;
	}*/
	else if(tmp2 == 0xFFFE) {
		extensible = true;
	}
	else {
		fprintf(stderr,"Error: unsupported format\n");
		goto last;
	}
	if(fread(&tmp2,2,1,fp) < 1) goto last;
	channels = tmp2;
	if(fread(&tmp,4,1,fp) < 1) goto last;
	samplerate = tmp;
	if(fseek_stdin(fp,4,&seekBuf,&seekBufSize)) goto last;
	if(fread(&tmp2,2,1,stream) < 1) goto last;
	if(fread(&tmp2,2,1,fp) < 1) goto last;
	bitPerSample = tmp2;
	if(bitPerSample == 8) type = kPCMTypeUnsignedInt;

	if(extensible && (chunkSize-16) == 24) {
		if(fread(&tmp2,2,1,fp) < 1) goto last;
		chunkSize = tmp2;
		if(fseek_stdin(fp,2,&seekBuf,&seekBufSize)) goto last;
		if(fread(&channelBitmap,4,1,fp) < 1) goto last;
		unsigned char guid[16];
		if(fread(guid,1,16,fp) < 16) goto last;
		if(!memcmp(guid,INTEGER_PCM_GUID,16)) {
			if(bitPerSample == 8) type = kPCMTypeUnsignedInt;
			else type = kPCMTypeSignedInt;
		}
		/*else if(!memcmp(guid,FLOAT_PCM_GUID,16)) {
			PCMType = kPCMTypeFloat;
		}*/
		else {
			fprintf(stderr,"Error: unsupported format\n");
			goto last;
		}
	}
	else if(extensible) {
		fprintf(stderr,"Error: unsupported format\n");
		goto last;
	}
	else if(fseek_stdin(fp,chunkSize-16,&seekBuf,&seekBufSize)) goto last;

	while(1) { // find data chunk
		if(fread(chunk,1,4,fp) < 4) goto last;
		if(fread(&chunkSize,4,1,fp) < 1) goto last;
		if(!memcmp(chunk,"data",4)) break;
		if(fseek_stdin(fp,chunkSize,&seekBuf,&seekBufSize)) goto last;
	}

	totalFrames = chunkSize / (channels*bitPerSample/8);
	ret = true;
last:
	free(seekBuf);
	return ret;
}

bool FhGAACEncoder::openFile(_TCHAR *inFile)
{
	SF_INFO sfinfo = { 0 };
#ifdef UNICODE
	sff = sf_wchar_open(inFile, SFM_READ, &sfinfo);
#else
	sff = sf_open(inFile, SFM_READ, &sfinfo);
#endif
	if(!sff || sf_error(sff)) {
		fprintf(stderr,"Error: unsupported format\n");
		if(sff) sf_close(sff);
		return false;
	}
	samplerate = sfinfo.samplerate;
	channels = sfinfo.channels;
	switch((sfinfo.format)&SF_FORMAT_SUBMASK) {
	  case 1:
		bitPerSample = 8;
		type = kPCMTypeSignedInt;
		break;
	  case 2:
		bitPerSample = 16;
		type = kPCMTypeSignedInt;
		break;
	  case 3:
		bitPerSample = 24;
		type = kPCMTypeSignedInt;
		break;
	  case 4:
		bitPerSample = 32;
		type = kPCMTypeSignedInt;
		break;
	  case 5:
		bitPerSample = 8;
		type = kPCMTypeUnsignedInt;
		break;
	  /*case 6:
		bitPerSample = 32;
		type = kPCMTypeFloat;
		break;*/
	  default:
		fprintf(stderr,"Error: unsupported format\n");
		sf_close(sff);
		sff = NULL;
		return false;
	}
	
	totalFrames = sfinfo.frames;
	//fprintf(stderr,"%d,%d,%d,%lld\n",bitPerSample,channels,samplerate,totalFrames);
	return true;
}


__int64 FhGAACEncoder::beginEncode(_TCHAR *outFile, encodingParameters *params)
{
	__int64 total = 0;
	int used,i;
	_TCHAR tempDir[MAX_PATH+1];
	_TCHAR tempFile[MAX_PATH+1];
#ifdef UNICODE
	char tempFileMB[1024];
#endif
	int *readbuf = (int *)malloc(channels*4*SAMPLES_PER_LOOP);
	unsigned char *inbuf = (unsigned char *)malloc(channels*4*SAMPLES_PER_LOOP);
	unsigned char *outbuf = (unsigned char *)malloc(32768);
	
	if(!fp && !sff) goto last;
	
	GetTempPath(MAX_PATH+1, tempDir);
	GetTempFileName(tempDir,_T("fhg"),0,tempFile);
	GetLongPathName(tempFile,tempFile,MAX_PATH+1);
#ifdef UNICODE
	wcstombs_s(NULL,tempFileMB,1024,tempFile,(MAX_PATH+1)*sizeof(_TCHAR));
#endif
	
	FILE * tmp;
	unsigned int outt;
	_tfopen_s(&tmp,tempFile,_T("wt"));
	fprintf(tmp, "[audio_fhgaac]\nmode=%d\nprofile=%d\nbitrate=%d\npreset=%d\nsurround=0\n",params->mode,params->profile,params->modeQuality,params->modeQuality);
	outt = mmioFOURCC('A','A','C','f');
	fclose(tmp);

#ifdef UNICODE
	encoder=createAudio3(channels,samplerate,bitPerSample,mmioFOURCC('P','C','M',' '),&outt,tempFileMB);
#else
	encoder=createAudio3(channels,samplerate,bitPerSample,mmioFOURCC('P','C','M',' '),&outt,tempFile);
#endif
	DeleteFile(tempFile);
	if(!encoder) {
		fprintf(stderr,"error: createAudio3 failure (input PCM format is unsupported or invalid encoding parameters)\n");
		goto last;
	}

	FILE *fpw;
	if (params->adtsMode && !_tcscmp(outFile, _T("-"))) {
		fpw = stdout;
		_setmode(_fileno(stdout), _O_BINARY);
	}
	else if(_tfopen_s(&fpw, outFile,_T("wb"))) {
		if(fpw) fclose(fpw);
		fprintf(stderr,"error: cannot create output file\n");
		goto last;
	}

	unsigned char adts[8] = "\xff\xf1\x00\x00\x00\x1f\xfc";
	unsigned srindex = getSamplingRateIndex(samplerate);
	unsigned chconfig = getChannelConfig(channels);
	adts[2] = 0x40 | (srindex << 2) | ((chconfig & 4) >> 2);
	adts[3] = (chconfig & 3) << 6;

	if (params->adtsMode) {
		FILE *fpTemp = NULL;
		finishAudio3(tempFile,encoder);
		if(!_tfopen_s(&fpTemp,tempFile,_T("rb"))) {
			unsigned int value = getFrequencyAndChannelFromM4a(fpTemp);
				if(value != 0xffffffff) {
				srindex = (value >> 4) & 0xf;
				chconfig = value & 0xf;
				adts[2] = 0x40 | (srindex << 2) | ((chconfig & 4) >> 2);
				adts[3] = (chconfig & 3) << 6;
			}
		}
		if(fpTemp) fclose(fpTemp);
		DeleteFile(tempFile);
	}
	
	if(fp && params->ignoreLength) totalFrames = 0;
	int framepos = 0;
	while(1) {
		int ret;
		int readSize = SAMPLES_PER_LOOP;
		if(totalFrames && total + SAMPLES_PER_LOOP > totalFrames) readSize = (int)(totalFrames - total);
		
		if(sff) {
			if(bitPerSample == 32) ret = (int)sf_readf_int(sff,(int *)inbuf,readSize);
			else {
				ret = (int)sf_readf_int(sff,(int *)readbuf,readSize);
				for(i=0;i<ret*channels;i++) {
					switch(bitPerSample) {
					  case 8:
						*(inbuf+i) = ((readbuf[i] >> 24) &0xff)^0x80;
						break;
					  case 16:
						*((short *)inbuf+i) = readbuf[i] >> 16;
						break;
					  case 24:
						*(inbuf+3*i) = (readbuf[i] >> 8)&0xff;
						*(inbuf+3*i+1) = (readbuf[i] >> 16)&0xff;
						*(inbuf+3*i+2) = (readbuf[i] >> 24)&0xff;
						break;
					}
				}
			}
		}
		else ret = fread(inbuf,channels*bitPerSample/8,SAMPLES_PER_LOOP,fp);
		
		unsigned char *ptr = inbuf;
		int insize = ret*channels*bitPerSample/8;
		total += ret;
		//fprintf(stderr,"read %d samples (total %lld frames), encoding...\n",ret,total);
		while(insize > 0) {
			used = 0;
			int outsize = encoder->Encode(framepos++,ptr,insize,&used,outbuf,32768);
			//fprintf(stderr,"get %d bytes from encoder (remaining %d bytes)\n",outsize,insize);
			if(outsize>0 && params->adtsMode) {
				unsigned len = outsize + 7;
				adts[3] &= 0xfc;
				adts[3] |= len >> 11;
				adts[4] = (len >> 3) & 0xff;
				adts[5] &= 0x1f;
				adts[5] |= (len & 7) << 5;
				fwrite(adts,1,7,fpw);
				fwrite(outbuf,1,outsize,fpw);
			}
			if(used>0) {
				insize -= used;
				ptr += used;
			}
		}

		if(totalFrames && !params->quiet) {
			static double previousPercent = -1.0;
			double percent = floor(100.0*(double)total/totalFrames+0.5);
			if(percent > previousPercent) {
				fprintf(stderr,"\rProgress:% 4d%%",(int)percent);
				fflush(stderr);
				previousPercent = percent;
			}
		}
		if(ret < readSize || (totalFrames && total >= totalFrames)) break;
	}
	putc('\n', stderr);
	prepareToFinish(0,encoder);
	while(1) {
		used = 0;
		int outsize = encoder->Encode(framepos++,inbuf,0,&used,outbuf,32768);
		//fprintf(stderr,"get %d bytes from encoder, %d bytes used\n",outsize,used);
		if(outsize>0 && params->adtsMode) {
			unsigned len = outsize + 7;
			adts[3] &= 0xfc;
			adts[3] |= len >> 11;
			adts[4] = (len >> 3) & 0xff;
			adts[5] &= 0x1f;
			adts[5] |= (len & 7) << 5;
			fwrite(adts,1,7,fpw);
			fwrite(outbuf,1,outsize,fpw);
		}
		else if(outsize <= 0) break;
	}
	fclose(fpw);
	
	if(!params->adtsMode) {
		finishAudio3(outFile,encoder);
		struct __stat64 statbuf;
		if(!_tstat64(outFile,&statbuf)) {
			if(!_tfopen_s(&fpw,outFile,_T("r+b"))) {
				optimizeAtoms(fpw,statbuf.st_size);
			}
			if(fpw) fclose(fpw);
		}
	}
	
  last:
	free(readbuf);
	free(inbuf);
	free(outbuf);
	return total;
}

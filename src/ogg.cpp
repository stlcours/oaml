#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "oamlCommon.h"


static size_t oggFile_read(void *ptr, size_t size, size_t nmemb, void *datasource) {
	oggFile *ogg = (oggFile*)datasource;
	return ogg->GetFileCallbacks()->read(ptr, size, nmemb, ogg->GetFD());
}

static int oggFile_seek(void *datasource, ogg_int64_t offset, int whence) {
	oggFile *ogg = (oggFile*)datasource;
	return ogg->GetFileCallbacks()->seek(ogg->GetFD(), offset, whence);
}

int oggFile_close(void *datasource) {
	oggFile *ogg = (oggFile*)datasource;
	return ogg->GetFileCallbacks()->close(ogg->GetFD());
}

long oggFile_tell(void *datasource) {
	oggFile *ogg = (oggFile*)datasource;
	return ogg->GetFileCallbacks()->tell(ogg->GetFD());
}

static const ov_callbacks ogg_callbacks = {
	oggFile_read,
	oggFile_seek,
	oggFile_close,
	oggFile_tell
};

oggFile::oggFile(oamlFileCallbacks *cbs) : audioFile(cbs) {
	fcbs = cbs;
	fd = NULL;

	format = 0;
	channels = 0;
	samplesPerSec = 0;
	bitsPerSample = 0;
	totalSamples = 0;
}

oggFile::~oggFile() {
	if (fd != NULL) {
		Close();
	}
}

int oggFile::Open(const char *filename) {
	ASSERT(filename != NULL);

	if (fd != NULL) {
		Close();
	}

	fd = fcbs->open(filename);
	if (fd == NULL) {
		return -1;
	}

	OggVorbis_File *vf = new OggVorbis_File;
	if (ov_open_callbacks((void*)this, vf, NULL, 0, ogg_callbacks) < 0) {
		return -1;
	}

	vorbis_info *vi = ov_info(vf, -1);
	if (vi == NULL) {
		return -1;
	}

	channels = vi->channels;
	samplesPerSec = vi->rate;
	bitsPerSample = 16;
	totalSamples = (int)ov_pcm_total(vf, -1) * channels;

	fd = (void*)vf;

	return 0;
}

int oggFile::Read(ByteBuffer *buffer, int size) {
	unsigned char buf[4096];

	if (fd == NULL)
		return -1;

	OggVorbis_File *vf = (OggVorbis_File *)fd;

	int bytesRead = 0;
	while (size > 0) {
		// Let's keep reading data!
		int bytes = size < 4096 ? size : 4096;
		int ret = (int)ov_read(vf, (char*)buf, bytes, 0, 2, 1, &currentSection);
		if (ret == 0) {
			break;
		} else {
			buffer->putBytes(buf, ret);
			bytesRead+= ret;
			size-= ret;
		}
	}

	return bytesRead;
}

void oggFile::WriteToFile(const char *filename, ByteBuffer *buffer, int channels, unsigned int sampleRate, int bytesPerSample) {
}

void oggFile::Close() {
	if (fd != NULL) {
		OggVorbis_File *vf = (OggVorbis_File *)fd;
		delete vf;
		fd = NULL;
	}
}

#include "exiftransplant.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string>
using namespace std;

static void swap16(uint16_t &us) {
	us = ((us >> 8) | (us << 8));
}


bool ExifTransplant::transplant(const char *src_path, const char *dst_path) {
	FILE *src = fopen(src_path, "rb");
	if(!src) {
		error = "Could not open source file ";
		return false;
	}

	unsigned int exif_size;
	unsigned char *exif = getExif(src, exif_size);
	fclose(src);
	if(!exif) {
		return false;
	}
	FILE *dst = fopen(dst_path, "r+b");
	if(!dst) {
		error = "Could not open destination file";
		return false;
	}
	unsigned int size = insertExif(dst, exif_size, exif);
	if(size == 0)
		return false;
	fclose(dst);
	truncate(dst_path, size);
	return true;
}

unsigned char *ExifTransplant::getExif(FILE *fp, unsigned int &exif_size) {
	unsigned char header[2];
	fread(header, 1, 2, fp);
	if(header[0] != 0xFF || header[1] != 0xD8) {
		error =  "Source is not a JPEG file";
		return NULL;
	}
	unsigned int exif_end = 2;
	while(1) {

		unsigned char marker[2];
		int readed = fread(marker, 1, 2, fp);
		if(readed != 2) {
			error = "Source file truncated";
			return NULL;
		}

		if(marker[1] == 0xD9) {
			error  = "Exif not found";
			return NULL;
		}
		int pos = ftell(fp);

		unsigned short marker_size;
		readed = fread(&marker_size, 2, 1, fp);
		if(readed != 1) {
			error = "Source file truncated";
			return NULL;
		}
		swap16(marker_size);

		if(marker[1] == 0xE0 || marker[1] == 0xE1) {
			exif_end = pos + marker_size;
		} else
			break;

		pos += marker_size;
		fseek(fp, pos, SEEK_SET);
	}
	if(exif_end == 2)
		return NULL;
	exif_size = exif_end -2;
	//found exif
	unsigned char *exif = new unsigned char[exif_size];
	fseek(fp, 2, SEEK_SET);
	unsigned int readed = fread(exif, 1, exif_size, fp);
	if(readed != exif_size) {
		error = "Could not read source file";
		delete []exif;
		return NULL;
	}
	return exif;
}

//return file size
unsigned int ExifTransplant::insertExif(FILE *fp, unsigned int exif_size, unsigned char *exif) {
	unsigned char header[2];
	fread(header, 1, 2, fp);
	if(header[0] != 0xFF || header[1] != 0xD8) {
		error = "Destination is not a JPEG file";
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	unsigned int file_size = ftell(fp);
	fseek(fp, 2, SEEK_SET);

	//we can assume E0 and E1 are just after the header.
	//remove E0 and E1 (in case), replace with what we got.
	unsigned int remainder_start = 2;

	while(1) {
		//read marker
		unsigned char marker[2];
		int readed = fread(marker, 1, 2, fp);
		if(readed != 2) {
			error = "Destination file truncated";
			return 0;
		}

		if(marker[1] == 0xD9) //jpeg finished, might have some additional non standard bytes.
			break;

		int pos = ftell(fp); //marker size includes size bytes

		unsigned short marker_size;
		readed = fread(&marker_size, 2, 1, fp);
		if(readed != 1) {
			error = "Destination file truncated";
			return 0;
		}
		swap16(marker_size);

		pos += marker_size;
		fseek(fp, pos, SEEK_SET);

		if(marker[1] == 0xE0 || marker[1] == 0xE1)
			remainder_start = pos;
		else
			break;
	}

	unsigned int remainder_size = file_size -remainder_start;
	fseek(fp, remainder_start, SEEK_SET);
	unsigned char *remainder = new unsigned char[remainder_size];
	int readed = fread(remainder, 1, remainder_size, fp);
	if(readed != remainder_size) {
		delete []exif;
		delete []remainder;
		return false;
	}
	fseek(fp, 2, SEEK_SET);
	int written = fwrite(exif, 1, exif_size, fp);
	delete []exif;
	written = fwrite(remainder, 1, remainder_size, fp);
	delete []remainder;
	return ftell(fp);
}



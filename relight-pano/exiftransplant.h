#ifndef EXIFTRANSPLANT_H
#define EXIFTRANSPLANT_H

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string>
using namespace std;

class ExifTransplant {
public:
	std::string error;

	bool transplant(const char *src_path, const char *dst_path);
	unsigned char *getExif(FILE *fp, unsigned int &exif_size);
	//returns file size
	unsigned int insertExif(FILE *fp, unsigned int exif_size, unsigned char *exif);
};



#endif // EXIFTRANSPLANT_H

#ifndef RRTI_H
#define RRTI_H

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>

class LRti {
public:
	enum Type { UNKNOWN = 0, PTM_LRGB = 1, PTM_RGB = 2, HSH  = 3};
	enum PTMFormat { RAW = 0, JPEG = 1, JPEGLS = 2 };

	Type type;

	int width;
	int height;
	std::vector<std::vector<unsigned char>> data;
	std::vector<float> scale;
	std::vector<int> bias;
	bool chromasubsampled = false;

	LRti():  type(UNKNOWN), width(0), height(0) {}
	bool load(const char *filename);

	void clip(int left, int bottom, int right, int top);
	LRti clipped(int left, int bottom, int right, int top);

	bool encode(PTMFormat format, int &size, uint8_t *&buffer, int quality = 90);
	bool encode(PTMFormat format, const char *filename, int quality = 90);

	bool encodeJPEG(int startplane, int quality, const char *filename);
	

protected:
	bool loadPTM(FILE *file);
	bool loadHSH(FILE *file);

	bool decodeRAW(const std::string &version, FILE *file);
	bool decodeJPEG(FILE *file);


	bool encodeJPEG(std::vector<int> &sizes, std::vector<uint8_t *> &buffers, int quality = 90);
};

#endif // RRTI_H

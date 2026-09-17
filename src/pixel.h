#ifndef PIXEL_H
#define PIXEL_H

#include "color.h"
#include <vector>

/*A Pixel is the collection of N lights intensity.
  pixel array is organized by pixel:
  pixel0: light1, light2 ... light n;
  then pixel1: etc etc.
*/

class Pixel: public std::vector<Color3f> {
public:
	int x, y;
};

class PixelArray: public std::vector<Pixel> {
public:
	uint32_t nlights;

	PixelArray(size_t n = 0, size_t k = 0): nlights(k) {
		resize(n, k);
	}
	void resize(size_t n, size_t k) {
		nlights = k;
		std::vector<Pixel>::resize(n);
		for(auto &pixel: *this)
			pixel.resize(nlights);
	}
	uint32_t components() { return nlights; }
	uint32_t npixels() const { return size(); }
	std::vector<Color3f> &pixel(size_t i) {
		return this->at(i);
	}
};

#endif // PIXEL_H

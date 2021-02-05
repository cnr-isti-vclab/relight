#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <vector>
#include <algorithm>

class Material {
public:
	struct Plane {
		float range = 0.0; //to rename as mscale
		float min = 0.0;
		float max = 0.0;
		float scale = 0.0;
		float bias = 0.0;

		uint8_t quantize(double value) { //expects value [0,255] return [0,255]
			value /= 255.0;
			int v = (int)255*(value/scale + bias);
			return std::max(0, std::min(255, v));
		}
		
		float dequantize(uint8_t value) {
			return 255.0f*((value/255.0f) - bias)*scale;
		}


/*
		uint8_t quantize(double value) {
			int v = 127 + (int)255*(value - bias)/scale;
			return std::max(0, std::min(255, v));
		}
		float dequantize(uint8_t value) {
			return ((value - (int)127)/255.0)*scale + bias;
		}
*/
	};

	std::vector<Plane> planes;

//#define WHOLERANGE
#ifdef WHOLERANGE
	uint8_t quantize(int plane, double value) {
		Plane &p = planes[plane];
		int v = (int)255*(value - p.min)/(p.max - p.min);
		return std::max(0, std::min(255, v));
	}
	float dequantize(int plane, uint8_t value) {
		Plane &p = planes[plane];
		return ((value - (int)127)/255.0f)*(p.max - p.min) + p.min;
	}
#endif

};


class MaterialBuilder {
public:
	std::vector<float> proj;
	std::vector<float> mean;
};


#endif // MATERIAL_H


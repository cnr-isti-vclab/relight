#include "fast_gaussian_blur.h"

#include <stdlib.h>
#include <math.h>

void fast_gaussian_blur(std::vector<float> &data, unsigned int width, unsigned int height, float sigma) {

	// Allocate buffers
	std::vector<double> buffer(width*height, 0);

	double q;
	if (sigma >= 2.5f)
		q = 0.98711f * sigma - 0.96330f;
	else if (sigma >= 0.5f)
		q = 3.97156f - 4.14554f * sqrt(1.0f - 0.26891f * sigma);
	else
		return;


	double b0 = 1.57825f + 2.44413f*q + 1.4281f*q*q + 0.422205f*q*q*q;
	double b1 = 2.44413f*q + 2.85619f*q*q + 1.26661f*q*q*q;
	double b2 = -( 1.4281f*q*q + 1.26661f*q*q*q );
	double b3 = 0.422205f*q*q*q;

	double B = 1.0 - (b1 + b2 + b3) / b0;

	double prev1, prev2, prev3;

	// Horizontal forward pass
	for(unsigned int y = 0; y < height; y++) {
		prev1 = prev2 = prev3 = data[y*width];

		for(unsigned int x = 0; x < width; x++) {
			double val = B * double(data[x + y*width]) + (b1 * prev1 + b2 * prev2 + b3 * prev3) / b0;
			buffer[x + y*width] = val;
			prev3 = prev2;
			prev2 = prev1;
			prev1 = val;
		}
	}

	// Backward pass
	for(unsigned int y = height-1; y < height; y--) {
		prev1 = prev2 = prev3 = buffer[width-1 +  y*width];

		for(int x = width-1; x >= 0; x--) {
			double val = B * buffer[x + y*width] + (b1 * prev1 + b2 * prev2 + b3 * prev3) / b0;
			buffer[x + y*width] = val;
			prev3 = prev2;
			prev2 = prev1;
			prev1 = val;

		}
	}

	// Vertical  pass
	for(unsigned int x = 0; x < width; x++) {
		prev1 = prev2 = prev3 = buffer[x];

		for(unsigned int y = 0; y < height; y++) {
			double val = B * buffer[x + y*width] + (b1 * prev1 + b2 * prev2 + b3 * prev3) / b0;
			buffer[x + y*width] = val;
			prev3 = prev2;
			prev2 = prev1;
			prev1 = val;

		}
	}


	for(unsigned int x = width-1; x < width; x--) {
		prev1 = prev2 = prev3 = buffer[x +  (height-1)*width];

		for(int y = height-1; y >= 0; y--) {
			double val = B * buffer[x + y*width] + (b1 * prev1 + b2 * prev2 + b3 * prev3) / b0;
			data[x + y*width] = float(val);
			prev3 = prev2;
			prev2 = prev1;
			prev1 = val;

		}
	}
}


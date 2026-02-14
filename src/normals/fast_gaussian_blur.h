#ifndef FAST_GAUSSIAN_BLUR_H
#define FAST_GAUSSIAN_BLUR_H

#include <vector>

void fast_gaussian_blur(std::vector<float> &data, unsigned int width, unsigned int height, float sigma);

#endif

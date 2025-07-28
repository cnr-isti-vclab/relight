#ifndef GRID_H
#define GRID_H
#include <vector>
#include <algorithm>
#include <eigen3/Eigen/Dense>
#include <assert.h>

template <class T> class Grid: public std::vector<T> {

public:
	Grid() {}
	Grid(size_t cols, size_t rows, const T &zero) {
		_rows = rows;
		_cols = cols;
		_zero = zero;
		this->resize(rows*cols, _zero);
	}
	size_t rows() const { return _rows; }
	size_t cols() const { return _cols; }
	T &at(size_t y, size_t x) { return (*this)[x + y*_cols]; }
	const T &at(size_t y, size_t x) const { return (*this)[x + y*_cols]; }
	void fill(T t) {
		for(T &v: *this)
			v = t;
	}

	T bilinearInterpolate(float x, float y) {
		int x1 = static_cast<int>(floor(x));
		int y1 = static_cast<int>(floor(y));
		int x2 = std::min(x1 + 1, static_cast<int>(_cols-1));
		int y2 = std::min(y1 + 1, static_cast<int>(_rows -1));

		float dx = x - x1;
		float dy = y - y1;

		T p1 = at(y1, x1);
		T p2 = at(y1, x2);
		T p3 = at(y2, x1);
		T p4 = at(y2 ,x2);

		return p1 * (1 - dx) * (1 - dy) + p2 * dx * (1 - dy) + p3 * (1 - dx) * dy + p4 * dx * dy;
	}

	// Downscale using area sampling (box filter)
	Grid downscale(int newWidth, int newHeight) {
		int oldWidth = _cols;
		int oldHeight = _rows;

		Grid output(newWidth, newHeight, _zero);
		for (int y = 0; y < newHeight; ++y) {
			for (int x = 0; x < newWidth; ++x) {
				// Calculate the corresponding pixel range in the original image
				float srcX = (static_cast<float>(x) / newWidth) * oldWidth;
				float srcY = (static_cast<float>(y) / newHeight) * oldHeight;
				int srcX1 = std::floor(srcX);
				int srcY1 = std::floor(srcY);
				int srcX2 = std::min(srcX1 + 1, oldWidth - 1);
				int srcY2 = std::min(srcY1 + 1, oldHeight - 1);

				// Perform area-based averaging (simple box filter)
				T avgPixel = _zero;
				int count = 0;
				for (int sy = srcY1; sy <= srcY2; ++sy) {
					for (int sx = srcX1; sx <= srcX2; ++sx) {
						avgPixel += at(sy, sx);
						count++;
					}
				}
				avgPixel /= count;

				output.at(y, x) = avgPixel;
			}
		}
		return output;
	}

	// Upscale using bilinear interpolation
	Grid upscale(int newWidth, int newHeight) {
		int oldWidth = _cols;
		int oldHeight = _rows;
		Grid output(newWidth, newHeight, _zero);

		for (int y = 0; y < newHeight; ++y) {
			for (int x = 0; x < newWidth; ++x) {
				// Calculate corresponding location in the original image
				float srcX = (static_cast<float>(x) / newWidth) * oldWidth;
				float srcY = (static_cast<float>(y) / newHeight) * oldHeight;

				// Perform bilinear interpolation
				output.at(y, x) = bilinearInterpolate(srcX, srcY);
			}
		}

		return output;
	}

	std::vector<float> gaussianKernel(int size, float sigma) const {

		assert(size % 2 != 0);

		int radius = size / 2;
		std::vector<float> kernel(size);

		float sum = 0.0f;
		for (int i = -radius; i <= radius; ++i) {
			float value = exp(-(i * i) / (2 * sigma * sigma));
			kernel[i + radius] = value;
			sum += value;
		}

		for (float &value : kernel) {
			value /= sum;
		}

		return kernel;
	}

	Grid convolve1D(const std::vector<float> &kernel, bool alongRows) const {
		int radius = kernel.size() / 2;
		Grid result(_cols, _rows, _zero);

		if (alongRows) {
			for (int y = 0; y < int(_rows); ++y) {
				for (int x = 0; x < int(_cols); ++x) {
					T sum = _zero;
					float w = 0;
					for (int k = -radius; k <= radius; ++k) {
						int idx = x + k;
						if (idx >= 0 && idx < int(_cols)) {
							sum += at(y, idx) * kernel[k + radius];
							w += kernel[k + radius];
						}
					}
					result.at(y, x) = sum/w;
				}
			}
		} else {
			for (int x = 0; x < int(_cols); ++x) {
				for (int y = 0; y < int(_rows); ++y) {
					T sum = _zero;
					float w = 0;
					for (int k = -radius; k <= radius; ++k) {
						int idy = y + k;
						if (idy >= 0 && idy < int(_rows)) {
							sum += at(idy, x) * kernel[k + radius];
							w += kernel[k + radius];
						}
					}
					result.at(y, x) = sum/w;
				}
			}
		}

		return result;
	}

	Grid gaussianBlur(int kernelSize, float sigma) const {
		std::vector<float> kernel = gaussianKernel(kernelSize, sigma);

		Grid blurred_rows = convolve1D(kernel, true);
		return blurred_rows.convolve1D(kernel, false);
	}

private:
	T _zero;
	size_t _rows;
	size_t _cols;
};


#endif // GRID_H

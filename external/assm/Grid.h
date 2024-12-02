#ifndef GRID_H
#define GRID_H
#include <vector>
#include <Eigen/Dense>
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
	void gaussianBlur(Grid<T> &blurred, int size, float sigma) {
		//create kernel
		//convolve
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
					for (int k = -radius; k <= radius; ++k) {
						int idx = x + k;
						if (idx >= 0 && idx < _cols) {
							sum += at(y, idx) * kernel[k + radius];
						}
					}
					result.at(y, x) = sum;
				}
			}
		} else {
			for (int x = 0; x < int(_cols); ++x) {
				for (int y = 0; y < int(_rows); ++y) {
					T sum = _zero;
					for (int k = -radius; k <= radius; ++k) {
						int idx = y + k;
						if (idx >= 0 && idx < int(_rows)) {
							sum += at(idx, x) * kernel[k + radius];
						}
					}
					result.at(y, x) = sum;
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

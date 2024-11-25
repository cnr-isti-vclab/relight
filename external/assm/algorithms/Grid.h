#ifndef GRID_H
#define GRID_H
#include <vector>
#include <Eigen/Dense>
#include <assert.h>



template <class T> class Grid: public std::vector<T> {

public:
	Grid() {}
	Grid(size_t rows, size_t cols, const T &zero) {
		_rows = rows;
		_cols = cols;
		_zero = zero;
		this->resize(rows*cols, _zero);
	}
	size_t rows() const { return _rows; }
	size_t cols() const { return _cols; }
	T &at(size_t row, size_t col) { return (*this)[row + col*_rows]; }
	const T &at(size_t row, size_t col) const { return (*this)[row + col*_rows]; }
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
		Grid result(_rows, _cols, _zero);

		if (alongRows) {
			for (int i = 0; i < _rows; ++i) {
				for (int j = 0; j < _cols; ++j) {
					T sum = _zero;
					for (int k = -radius; k <= radius; ++k) {
						int idx = j + k;
						if (idx >= 0 && idx < _cols) {
							sum += at(i, idx) * kernel[k + radius];
						}
					}
					result.at(i, j) = sum;
				}
			}
		} else {
			for (int j = 0; j < _cols; ++j) {
				for (int i = 0; i < _rows; ++i) {
					T sum = _zero;
					for (int k = -radius; k <= radius; ++k) {
						int idx = i + k;
						if (idx >= 0 && idx < _rows) {
							sum += at(idx, j) * kernel[k + radius];
						}
					}
					result.at(i, j) = sum;
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

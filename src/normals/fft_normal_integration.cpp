#include "fft_normal_integration.h"

#include "pocketfft.h"
#include <Eigen/Dense>

#include <vector>
#include <complex>
#include <cmath>


using namespace std;
using namespace Eigen;

typedef Matrix<std::complex<double>, Dynamic, Dynamic> ComplexMatrix;

// Helper function to generate meshgrid-like matrices
void meshgrid(MatrixXd& wx, MatrixXd& wy, int cols, int rows) {
	wx.resize(rows, cols);
	wy.resize(rows, cols);

	double colMid = (cols / 2) + 1;
	double rowMid = (rows / 2) + 1;
	double colDiv = cols - (cols % 2);
	double rowDiv = rows - (rows % 2);

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			wx(i, j) = (j + 1 - colMid) / colDiv;
			wy(i, j) = (i + 1 - rowMid) / rowDiv;
		}
	}
}

MatrixXd ifftshift(const MatrixXd& input) {
	int rows = input.rows();
	int cols = input.cols();
	MatrixXd shifted(rows, cols);

	int rowShift = rows / 2;
	int colShift = cols / 2;

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			int newRow = (i + rowShift) % rows;
			int newCol = (j + colShift) % cols;
			shifted(newRow, newCol) = input(i, j);
		}
	}
	return shifted;
}
void fft2(const MatrixXd& input, ComplexMatrix& output) {
	int rows = input.rows();
	int cols = input.cols();

	// Prepare data
	std::vector<std::complex<double>> data(rows * cols);
	for (int y = 0; y < rows; ++y) {
		for (int x = 0; x < cols; ++x) {
			data[y * cols + x] = input(y, x);
		}
	}

	// Perform FFT
	ptrdiff_t element_size = sizeof(std::complex<double>);
	pocketfft::shape_t shape = {size_t(cols), size_t(rows)};
	pocketfft::stride_t stride = { element_size, ptrdiff_t(cols)*element_size };
	pocketfft::shape_t axes{0, 1};

	pocketfft::c2c(shape, stride, stride, axes, pocketfft::FORWARD, data.data(), data.data(), 1.0);

	// Fill output
	output.resize(rows, cols);
	for (int y = 0; y < rows; ++y)
		for (int x = 0; x < cols; ++x) {
			output(y, x) = data[y * cols + x];
		}
}

// Function to compute 2D IFFT using PocketFFT
void ifft2(const ComplexMatrix& input, MatrixXd& output) {
	int rows = input.rows();
	int cols = input.cols();

	// Prepare data
	std::vector<std::complex<double>> data(rows * cols);
	for (int i = 0; i < rows; ++i)
		for (int j = 0; j < cols; ++j) {
			data[i * cols + j] = input(i, j);
		}

	// Perform IFFT
	ptrdiff_t element_size = sizeof(std::complex<double>);
	pocketfft::shape_t shape = {size_t(cols), size_t(rows)};
	pocketfft::stride_t stride = { element_size, ptrdiff_t(cols)*element_size };
	pocketfft::shape_t axes{0, 1};
	pocketfft::c2c(shape, stride, stride, axes, pocketfft::BACKWARD, data.data(), data.data(), 1.0/(4*sqrt(2)* rows * cols));

	// Fill output and normalize
	output.resize(rows, cols);
	for (int i = 0; i < rows; ++i)
		for (int j = 0; j < cols; ++j) {
			output(i, j) = data[i * cols + j].real();
		}
}

void pad(int &w, int &h, std::vector<Eigen::Vector3f> &normals, int padding) {
	int W = w + 2*padding;
	int H = h + 2*padding;
	std::vector<Eigen::Vector3f> n(W*H);
	for(int y = 0; y < H;  y++) {
		for(int x = 0; x < W; x++) {
			int X = x - padding;
			int Y = y - padding;
			int flipx = 1;
			if(x < padding) {
				X = padding -x;
				flipx = -1;
			}

			if(x >= w + padding) {
				X = 2*w + padding - 1 - x;
				flipx = -1;
			}

			int flipy = 1;
			if(y < padding) {
				Y = padding - y;
				flipy = -1;
			}

			if(y >= h + padding) {
				Y = 2*h + padding - 1 - y;
				Y = H - padding + h - 1 -y;
				flipy = -1;
			}
			n[x + y*W][0] = flipx*normals[X + Y*w][0];
			n[x + y*W][1] = flipy*normals[X + Y*w][1];
			n[x + y*W][2] = normals[X + Y*w][2];
			assert(!isnan(n[x + y*W][0]));
		}
	}
	w = W;
	h = H;
	swap(normals, n);
}

void depad(int &w, int &h, std::vector<float> &heights, int padding) {

	int W = w;
	int H = h;
	w -= 2*padding;
	h -= 2*padding;
	std::vector<float> elev(w*h);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			elev[x + w*y] = heights[x + padding + (y + padding)*W];
		}
	}
	swap(elev, heights);
}


bool savePly(const QString &filename, size_t w, size_t h, std::vector<float> &z);

void fft_integrate(std::function<bool(QString s, int n)> progressed,
				   int cols, int rows, std::vector<Eigen::Vector3f> &normals, std::vector<float> &heights) {


	int minsize = std::min(cols, rows);
	int padding = minsize/2;
	pad(cols, rows, normals, padding);


	MatrixXd dzdx(rows, cols);
	MatrixXd dzdy(rows, cols);
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			auto &normal = normals[i * cols + j];
			dzdx(i, j) = normal[0] / normal[2]; // dz/dx = -nx/nz
			dzdy(i, j) = -normal[1] / normal[2]; // dz/dy = -ny/nz			assert(!isnan(dzdx(i, j)));
			assert(!isnan(dzdy(i, j)));
		}
	}

	MatrixXd wx, wy;
	meshgrid(wx, wy, cols, rows);

	wx = ifftshift(wx);
	wy = ifftshift(wy);

	// Fourier Transforms of gradients
	ComplexMatrix DZDX, DZDY;
	fft2(dzdx, DZDX);
	fft2(dzdy, DZDY);

	// Frequency domain integration
	ComplexMatrix Z(rows, cols);
	std::complex<double> j(0, 1); // Imaginary unit

	for (int y = 0; y < rows; ++y) {
		for (int x = 0; x < cols; ++x) {
			double wx2_wy2 = wx(y, x) * wx(y, x) + wy(y, x) * wy(y, x) + 1e-12; // Avoid division by zero
			Z(y, x) = (-j * wx(y, x) * DZDX(y, x) - j * wy(y, x) * DZDY(y, x)) / wx2_wy2;
		}
	}

	// Inverse FFT to reconstruct z
	MatrixXd z;
	ifft2(Z, z);

	heights.resize(rows* cols);
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			heights[i * cols + j] = static_cast<float>(z(i, j));
		}
	}
	depad(cols, rows, heights, padding);

	/*
	[wx, wy] = meshgrid(([1:cols]-(fix(cols/2)+1))/(cols-mod(cols,2)), ...
			([1:rows]-(fix(rows/2)+1))/(rows-mod(rows,2)));

	% Quadrant shift to put zero frequency at the appropriate edge
	wx = ifftshift(wx); wy = ifftshift(wy);

	DZDX = fft2(dzdx);   % Fourier transforms of gradients
	DZDY = fft2(dzdy);

	% Integrate in the frequency domain by phase shifting by pi/2 and
	% weighting the Fourier coefficients by their frequencies in x and y and
	% then dividing by the squared frequency.  eps is added to the
	% denominator to avoid division by 0.

	Z = (-j*wx.*DZDX -j*wy.*DZDY)./(wx.^2 + wy.^2 + eps);  % Equation 21

	z = real(ifft2(Z));  % Reconstruction
	*/
}

#include "fft_normal_integration.h"

#include "pocketfft.h"
#include <Eigen/Dense>

#include <vector>
#include <complex>
#include <cmath>


using namespace std;
using namespace Eigen;

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


//frequency of FFT bin i, as computed before by meshgrid() + ifftshift().
static double fftFrequency(int i, int n) {
	int j = (i + n - n/2) % n;
	return (j - n/2) / double(n - (n % 2));
}

bool savePly(const QString &filename, size_t w, size_t h, std::vector<float> &z);

void fft_integrate(std::function<bool(QString s, int n)> progressed,
				   int cols, int rows, std::vector<Eigen::Vector3f> &normals, std::vector<float> &heights) {

	int minsize = std::min(cols, rows);
	int padding = minsize/2;
	pad(cols, rows, normals, padding);

	std::vector<std::complex<double>> dzdx(size_t(rows) * cols);
	std::vector<std::complex<double>> dzdy(size_t(rows) * cols);
	for (size_t i = 0; i < dzdx.size(); ++i) {
		auto &normal = normals[i];
		dzdx[i] = normal[0] / normal[2]; // dz/dx = -nx/nz
		dzdy[i] = -normal[1] / normal[2]; // dz/dy = -ny/nz
		assert(!isnan(dzdx[i].real()));
		assert(!isnan(dzdy[i].real()));
	}

	// Fourier Transforms of gradients, nthreads = 0 uses all cores.
	ptrdiff_t element_size = sizeof(std::complex<double>);
	pocketfft::shape_t shape = {size_t(cols), size_t(rows)};
	pocketfft::stride_t stride = { element_size, ptrdiff_t(cols)*element_size };
	pocketfft::shape_t axes{0, 1};
	pocketfft::c2c(shape, stride, stride, axes, pocketfft::FORWARD, dzdx.data(), dzdx.data(), 1.0, 0);
	pocketfft::c2c(shape, stride, stride, axes, pocketfft::FORWARD, dzdy.data(), dzdy.data(), 1.0, 0);

	// Frequency domain integration, result goes in dzdx.
	std::complex<double> j(0, 1); // Imaginary unit
	for (int y = 0; y < rows; ++y) {
		double wy = fftFrequency(y, rows);
		for (int x = 0; x < cols; ++x) {
			double wx = fftFrequency(x, cols);
			double wx2_wy2 = wx * wx + wy * wy + 1e-12; // Avoid division by zero
			size_t i = size_t(y) * cols + x;
			dzdx[i] = (-j * wx * dzdx[i] - j * wy * dzdy[i]) / wx2_wy2;
		}
	}
	dzdy = std::vector<std::complex<double>>();

	// Inverse FFT to reconstruct z
	pocketfft::c2c(shape, stride, stride, axes, pocketfft::BACKWARD, dzdx.data(), dzdx.data(), 1.0/(4*sqrt(2)* rows * cols), 0);

	heights.resize(size_t(rows) * cols);
	for (size_t i = 0; i < heights.size(); ++i)
		heights[i] = static_cast<float>(dzdx[i].real());
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

#include "flatnormals.h"
#include "fast_gaussian_blur.h"
#include <assm/Grid.h>

#include <QFile>
#include <QTextStream>


#include <complex>
#include <math.h>

#include <Eigen/Dense>
using namespace Eigen;

#include "pocketfft.h"
using namespace pocketfft;

#include <vector>
#include <iostream>
using namespace std;

NormalsImage::~NormalsImage() {

}


void NormalsImage::load(std::vector<double> &_normals, int _w, int _h) {
	normals = _normals;
	w = _w;
	h = _h;
}

void NormalsImage::load(QString filename) {
	img.load(filename);
	w = img.width();
	h = img.height();

	normals.clear();
	normals.reserve(w*h);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			QRgb pixel = img.pixel(x, y);
			Vector3d n;
			n[0] = 2*qRed(pixel)/255.0 - 1.0;
			n[1] = 2*qGreen(pixel)/255.0 - 1.0;
			n[2] = 2*qBlue(pixel)/255.0 - 1.0;
			n.normalize();
			normals.push_back(n[0]);
			normals.push_back(n[1]);
			normals.push_back(n[2]);
		}
	}
}

void NormalsImage::save(QString filename) {
	flat = QImage(w, h, QImage::Format_ARGB32);
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			QRgb pixel = img.pixel(x, y);
			Vector3d n;
			n[0] = normals[3*(x + y*w)];
			n[1] = normals[3*(x + y*w)+1];
			n[2] = normals[3*(x + y*w)+2];

			pixel = qRgb((n[0] + 1.0)*255.0/2.0, (n[1] + 1.0)*255.0/2.0, n[2]*255.0);
			flat.setPixel(x, y, pixel);
		}
	}

	flat.save(filename);
}

void flattenRadialNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double binSize) {

	//don't use normals not flat enough
	double z_threshold = 0.7171;
	vector<double> binCount;
	vector<double> derivatives;
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int index = 3*(x + y*w);

			Vector3d n;
			n[0] = double(normals[index/3][0]);
			n[1] = double(normals[index/3][1]);
			n[2] = double(normals[index/3][2]);

			assert(!isnan(n[0]));
			assert(!isnan(n[1]));
			assert(!isnan(n[2]));

			if(n[2] < z_threshold) continue;

			Vector3d radial;
			radial[0] = x - w/2.0;
			radial[1] = h/2.0 - y;
			radial[2] = 1.0; //TODO Was zero!
			double distance = radial.norm();
			assert(distance > 0);
			radial.normalize();

			double outward = radial.dot(n);
			int bin = (int) floor(distance/binSize);
			if(derivatives.size() < size_t(bin+1)) {
				derivatives.resize(bin+1, 0.0);
				binCount.resize(bin+1, 0);
			}
			derivatives[bin] += outward;
			binCount[bin]++;
		}
	}
	for(size_t i = 0; i < derivatives.size(); i++)
		derivatives[i] /= binCount[i];

#if 0
	QFile csv("bins.csv");
	csv.open(QFile::WriteOnly | QFile::Truncate);
	QTextStream stream(&csv);
	for(int i = 0; i < derivatives.size(); i++)
		stream << i << "," << derivatives[i] << endl;
#endif

	double sum_x = 0;
	double sum_y = 0;
	double sum_xy = 0;
	double sum_x2 = 0;

	for(size_t i = 0; i < derivatives.size(); i++) {
		double x = i * binSize + binSize/2.0;
		sum_x += x;
		sum_y += derivatives[i];
		sum_xy += x * derivatives[i];
		sum_x2 += x*x;
	}

	// means
	double mean_x = sum_x / derivatives.size();
	double mean_y = sum_y / derivatives.size();

	float varx = sum_x2 - sum_x * mean_x;
	float cov = sum_xy - sum_x * mean_y;

	// check for zero varx
	double M = cov / varx;
	double Q = mean_y - M * mean_x;

	//overwrite normals;

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			Vector3d n;
			int idx = 3*(x + y*w);
			n[0] = double(normals[idx/3][0]);
			n[1] = double(normals[idx/3][1]);
			n[2] = double(normals[idx/3][2]);
			Vector3d radial;
			radial[0] = x - w/2.0;
			radial[1] = h/2.0 - y;
			radial[2] = 0;
			double distance = radial.norm();
			double inward = M*distance + Q;
			radial.normalize();
			radial = radial * inward;
			n -= radial;
			n.normalize();
			int pos = x + y*w;
			normals[pos][0] = float(n[0]);
			normals[pos][1] = float(n[1]);
			normals[pos][2] = float(n[2]);

		}
	}
}


void flattenRadialHeights(int w, int h, std::vector<float> &heights, double binSize) {

	vector<double> z(heights.size());

	//subsampling?
	Eigen::MatrixXd A(w*h, 6);
	for(int y = 0; y < h; y++) {
		for(int x= 0; x < w; x++) {
			int i = x + y*w;
			A(i, 0) = 1;
			A(i, 1) = x;
			A(i, 2) = y;
			A(i, 3) = x * x;
			A(i, 4) = y * y;
			A(i, 5) = x * y;
			z[i] = double(heights[i]);
		}
	}

	Eigen::VectorXd b = Eigen::Map<const Eigen::VectorXd>(z.data(), w*h);

	// Solve  A^T * A * coeffs = A^T * b
	Eigen::VectorXd coeffs = (A.transpose() * A).ldlt().solve(A.transpose() * b);
	cout << coeffs << endl;

	for(int y = 0; y < h; y++) {
		for(int x= 0; x < w; x++) {
			int i = x + y*w;
			double h = coeffs(0) + coeffs(1) * x + coeffs(2) * y
					+ coeffs(3) * x * x + coeffs(4) * y * y
					+ coeffs(5) * x * y;
			heights[i]  = float(z[i] - h);
		}
	}
}


void flattenBlurNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double sigma) {
	std::vector<float> b[2];
	for(int k = 0; k < 2; k++) {
		std::vector<float> &v = b[k];
		v.reserve(normals.size());
		for(Eigen::Vector3f n: normals) {
			n[k] = n[k]/n[2];
			v.push_back(n[k]);
		}
		fast_gaussian_blur(v, w, h, sigma);
	}
	for(size_t i = 0; i < normals.size(); i++) {
		auto &n = normals[i];
		n[0] -= b[0][i];
		n[1] -= b[1][i];

		float d = 1/sqrt(n[0]*n[0] + n[1]*n[1] + 1);
		n[0] *= d;
		n[1] *= d;
		n[2] = d;
	}
}

void flattenFourierNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, float padding, double sigma, bool exponential) {

	int padding_amount = round(padding*std::min(w, h));
	unsigned int W = w + padding_amount*2;
	unsigned int H = h + padding_amount*2;

	shape_t shape{ W, H };
	stride_t stride { 16, W*16 };
	shape_t axes{0, 1};

	vector<complex<double>> datax(W*H);
	vector<complex<double>> datay(W*H);

	vector<double> dataz(w*h);

	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			Vector3d n;
			int idx = 3*(x + y*w);
			n[0] = double(normals[idx/3][0]);
			n[1] = double(normals[idx/3][1]);
			n[2] = double(normals[idx/3][2]);


			if(exponential) {
				double t = acos(n[2]);
				if(t > 0) {
					n[0] = n[0]*t/sin(t);
					n[1] = n[1]*t/sin(t);
				}
			}

			assert(!isnan(n[0]));
			assert(!isnan(n[1]));
			assert(!isnan(n[2]));


			int X = x + padding_amount;
			int Y = y + padding_amount;
			datax[X + Y*W].real(n[0]);
			datax[X + Y*W].imag(0);
			datay[X + Y*W].real(n[1]);
			datay[X + Y*W].imag(0);
			if(x < padding_amount) {
				X = padding_amount -x;
			}

			if(x >= w - padding_amount) {
				X = W - padding_amount + w -1 - x;
			}

			if(y < padding_amount) {
				Y = padding_amount - y;
			}

			if(y >= h - padding_amount) {
				Y = H - padding_amount + h - 1 -y;
			}

			datax[X + Y*W].real(n[0]);
			datax[X + Y*W].imag(0);
			datay[X + Y*W].real(n[1]);
			datay[X + Y*W].imag(0);

			dataz[x + y*w] = n[2];
		}
	}

	c2c(shape, stride, stride, axes, FORWARD, datax.data(), datax.data(), 1.0);
	c2c(shape, stride, stride, axes, FORWARD, datay.data(), datay.data(), 1.0);


	for(int y = 0; y < H; y++) {
		for(int x = 0; x < W; x++) {
			int X = x;
			if(X > W/2) X -= W;
			int Y = y;
			if(Y > H/2) Y -= H/2;
			double r2 = X*X + Y*Y;
			double g = 1.0 - exp(-0.5*r2/(sigma*sigma));
			/* Using Hann instead?
			 * double g = 1.0;
			if(r2 < sigma*sigma/4)
				g = 1.0 - pow(cos(M_PI*r2/sigma), 2); */
			assert(!isnan(datax[x + y*W].real()));
			assert(!isnan(datay[x + y*W].imag()));

			datax[x + y*W] *= g;
			datay[x + y*W] *= g;
		}
	}

	c2c(shape, stride, stride, axes, BACKWARD, datax.data(), datax.data(), 1./(W*H));
	c2c(shape, stride, stride, axes, BACKWARD, datay.data(), datay.data(), 1./(W*H));


	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			int X  = x + padding_amount;
			int Y = y + padding_amount;
			double r = datax[X + Y*W].real();
			double g = datay[X + Y*W].real();
			double d = sqrt(r*r + g*g);
			if(exponential) {
				if(d > 0) {
					r = r*sin(d)/d;
					g = g*sin(d)/d;
				}
			}

			double b = sqrt(1 - r*r - g*g);

			int pos = x + y*w;
			normals[pos][0] = float(r);
			normals[pos][1] = float(g);
			normals[pos][2] = float(b);

			/*
			 //differences might be nice to export.
			double dr = inx[X + Y*W][0] - r;
			double dg = iny[X + Y*W][0] - g;
			double db = dataz[x + y*w] - b;


			QRgb dpixel = qRgb((dr + 1.0)*255.0/2.0, (dg + 1.0)*255.0/2.0, (db + 1.0)*255.0/2.0);
			residual.setPixel(x, y, dpixel);
			*/
		}
	}
}

void filterLowFrequencies(int w, int h, std::vector<std::complex<double>>& freq_data, double cutoff) {
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			double fx = (x <= w / 2) ? x : x - w;
			double fy = (y <= h / 2) ? y : y - h;
			double freq_mag = std::sqrt(fx * fx + fy * fy);

			if (freq_mag < cutoff) {
				freq_data[y * w +x] = 0.0;
			}
		}
	}
}

//TODO do the padding!
void flattenFourierHeights(int w, int h, std::vector<float> &heights, float padding, double sigma) {

	vector<double> heightmap(w*h);
	for(size_t i = 0; i < heights.size(); i++)
		heightmap[i] = heights[i];

	pocketfft::shape_t shape = { size_t(w), size_t(h) };
	pocketfft::stride_t stride = { ptrdiff_t(sizeof(double)), ptrdiff_t(sizeof(double)*w) };
	pocketfft::stride_t stride_freq = { ptrdiff_t(sizeof(std::complex<double>)), ptrdiff_t(sizeof(std::complex<double>)*w) };
	pocketfft::shape_t axes{0, 1};

	size_t n_elements = w*h;

	std::vector<std::complex<double>> freq_data(n_elements);

	pocketfft::r2c<double>(shape, stride, stride_freq, axes, FORWARD, heightmap.data(), freq_data.data(), 1.0);

	filterLowFrequencies(w, h, freq_data, sigma);

	pocketfft::c2r<double>(shape, stride_freq, stride, axes, BACKWARD, freq_data.data(), heightmap.data(), 1.0 / n_elements);

	for(size_t i = 0; i < heights.size(); i++)
		heights[i] = heightmap[i];
}

// ─────────────────────────── 4-point plane flattening ────────────────────────
// Define USE_PARABOLOID_FIT to use a paraboloid z = a*wx² + b*wy² + d*wx*wy + c
// centred on the image origin instead of sphere fitting.
#define USE_PARABOLOID_FIT

static void fitSphereN(const std::vector<Eigen::Vector3d> &pts,
                       double &cx, double &cy, double &cz, double &R2)
{
	// Constrained least-squares sphere fit: cx=cy=0 (axis through the image
	// centre), only cz and R are free.
	//   2*cz*zi + k = xi²+yi²+zi²   (k = R²-cz²)
	int n = (int)pts.size();
	Eigen::MatrixXd A(n, 2);
	Eigen::VectorXd b(n);
	for(int i = 0; i < n; i++) {
		A(i,0) = 2*pts[i].z();
		A(i,1) = 1.0;
		b[i] = pts[i].squaredNorm();
	}
	Eigen::Vector2d p = A.colPivHouseholderQr().solve(b);
	cx = 0; cy = 0; cz = p[0];
	R2 = std::max(0.0, p[1] + cz*cz);
}

void flattenPlaneNormals(int w, int h, std::vector<Eigen::Vector3f> &normals,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height)
{
	assert(points.size() >= 4);

	std::vector<QPoint> pts;
	for(const QPointF &fp : points) {
		int nx = (int)std::round((fp.x() - crop.left()) * w / (float)crop.width());
		int ny = (int)std::round((fp.y() - crop.top())  * h / (float)crop.height());
		pts.push_back(QPoint(nx, ny));
	}
	QPointF center(
		(image_width  / 2.0 - crop.left()) * w / (double)crop.width(),
		(image_height / 2.0 - crop.top())  * h / (double)crop.height()
	);
	// Fit surface slope field from reference normals.
	// slope_x = -nx/nz,  slope_y = -ny/nz.
	//
	// np < 7: radially symmetric  z = A*r⁴ + B*r²  (constant omitted; no effect on slope)
	//   slope_x = (4A r² + 2B) wx,   slope_y = (4A r² + 2B) wy
	// np >= 7: general 6-param   z = a*x⁴+b*x²y²+c*y⁴+d*x²+e*xy+f*y²
	//   slope_x = 4ax³ + 2bxy² + 2dx + ey
	//   slope_y = 2bx²y + 4cy³ + ex  + 2fy
	int np = (int)pts.size();
	const bool general  = (np >= 7);
	const bool linear   = (np > 9);
	const int ncols = linear ? 8 : (general ? 6 : 3);
	Eigen::MatrixXd Ag(2*np, ncols);
	Eigen::VectorXd sg(2*np);
	Ag.setZero();

	for(int i = 0; i < np; i++) {
		int px = pts[i].x(), py = pts[i].y();
		if(px < 0 || px >= w || py < 0 || py >= h)
			return;
		const Eigen::Vector3f &ni = normals[px + py*w];
		if(std::abs(ni[2]) < 1e-6f)
			return;
		double wx = px - center.x();
		double wy = center.y() - py;
		double r2 = wx*wx + wy*wy;
		double sx = -ni[0] / ni[2];
		double sy = -ni[1] / ni[2];
		if(linear) {
			// cols: a               b                c             d       e        f         h  k
			Ag(2*i,   0) = 4*wx*wx*wx;  Ag(2*i,   1) = 2*wx*wy*wy;                              Ag(2*i,   3) = 2*wx;  Ag(2*i,   4) = wy;  Ag(2*i,   6) = 1;
			                            Ag(2*i+1, 1) = 2*wx*wx*wy;  Ag(2*i+1, 2) = 4*wy*wy*wy;  Ag(2*i+1, 4) = wx;    Ag(2*i+1, 5) = 2*wy; Ag(2*i+1, 7) = 1;
		} else if(general) {
			// cols: a              b                             c             d       e       f
			Ag(2*i,   0) = 4*wx*wx*wx;  Ag(2*i,   1) = 2*wx*wy*wy;                              Ag(2*i,   3) = 2*wx;  Ag(2*i,   4) = wy;
			                            Ag(2*i+1, 1) = 2*wx*wx*wy;  Ag(2*i+1, 2) = 4*wy*wy*wy;  Ag(2*i+1, 4) = wx;    Ag(2*i+1, 5) = 2*wy;
		} else {
			// cols: a        b        c   (z = a*wx² + b*wx*wy + c*wy²)
			Ag(2*i,   0) = 2*wx;  Ag(2*i,   1) = wy;
			Ag(2*i+1, 1) = wx;    Ag(2*i+1, 2) = 2*wy;
		}
		sg[2*i]   = sx;
		sg[2*i+1] = sy;
	}

	Eigen::VectorXd cf = Ag.colPivHouseholderQr().solve(sg);

	cout << "flattenPlaneNormals: fitted slope coefficients (" << np << " points)\n";
	if(linear)
		cout << "  GENERAL4L: a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2]
		     << " d=" << cf[3] << " e=" << cf[4] << " f=" << cf[5]
		     << " h=" << cf[6] << " k=" << cf[7] << "\n";
	else if(general)
		cout << "  GENERAL4:  a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2]
		     << " d=" << cf[3] << " e=" << cf[4] << " f=" << cf[5] << "\n";
	else
		cout << "  QUADRATIC2: a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2] << "\n";

	SurfaceCoeffs sc;
	sc.center = center;
	if(linear) {
		sc.nterms = 9;
		sc.c[0] = cf[0]; sc.c[1] = cf[1]; sc.c[2] = cf[2];
		sc.c[3] = cf[3]; sc.c[4] = cf[4]; sc.c[5] = cf[5];
		sc.c[6] = cf[6]; sc.c[7] = cf[7]; sc.c[8] = 0;
	} else if(general) {
		sc.nterms = 7;
		sc.c[0] = cf[0]; sc.c[1] = cf[1]; sc.c[2] = cf[2];
		sc.c[3] = cf[3]; sc.c[4] = cf[4]; sc.c[5] = cf[5]; sc.c[6] = 0;
	} else {
		sc.nterms = 4;
		sc.c[0] = cf[0]; sc.c[1] = cf[1]; sc.c[2] = cf[2]; sc.c[3] = 0;
	}
	applyNormalCorrection(w, h, normals, sc);
}

void applyNormalCorrection(int w, int h, std::vector<Eigen::Vector3f> &normals,
                           const SurfaceCoeffs &sc)
{
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			double wx = x - sc.center.x();
			double wy = sc.center.y() - y;
			double gx, gy;
			sc.gradient(wx, wy, gx, gy);
			Eigen::Vector3f n_fit(-(float)gx, -(float)gy, 1.0f);
			n_fit.normalize();
			Eigen::Vector3f axis = n_fit.cross(Eigen::Vector3f(0, 0, 1));
			float sin_a = axis.norm();
			if(sin_a < 1e-6f) continue;
			axis /= sin_a;
			Eigen::Matrix3f R = Eigen::AngleAxisf(std::atan2(sin_a, n_fit[2]), axis).toRotationMatrix();
			normals[x + y*w] = (R * normals[x + y*w]).normalized();
		}
	}
}

void flattenPlaneHeights(int w, int h, std::vector<float> &heights,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height,
                         std::vector<Eigen::Vector3f> *correct_normals,
                         SurfaceCoeffs *out_sc)
{
	assert(points.size() >= 4);
	assert((int)heights.size() == w*h);

	std::vector<QPoint> pts;
	for(const QPointF &fp : points) {
		int nx = (int)std::round((fp.x() - crop.left()) * w / (float)crop.width());
		int ny = (int)std::round((fp.y() - crop.top())  * h / (float)crop.height());
		pts.push_back(QPoint(nx, ny));
	}
	QPointF center(
		(image_width  / 2.0 - crop.left()) * w / (double)crop.width(),
		(image_height / 2.0 - crop.top())  * h / (double)crop.height()
	);

	// Build 3D positions of the N reference points in pixel-unit space.
	std::vector<Eigen::Vector3d> xyz;
	xyz.reserve(pts.size());
	double mean_z = 0;
	for(size_t i = 0; i < pts.size(); i++) {
		int px = pts[i].x();
		int py = pts[i].y();
		if(px < 0 || px >= w || py < 0 || py >= h)
			return;
		double x = px - center.x();
		double y = center.y() - py;
		double z = heights[px + py*w];
		xyz.push_back({x, y, z});
		mean_z += z;
	}
	mean_z /= (double)xyz.size();

#ifdef USE_PARABOLOID_FIT
	// np < 7:  QUADRATIC2 z = a*x²+b*xy+c*y²+d              (4 params)
	// np 7..9: GENERAL4   z = a*x⁴+b*x²y²+c*y⁴+d*x²+e*xy+f*y²+g  (7 params)
	// np > 9:  GENERAL4L  same + h*x + k*y                  (9 params)
	const int np = (int)xyz.size();
	const bool general = (np >= 7);
	const bool linear  = (np > 9);
	{
		Eigen::MatrixXd Ap;
		Eigen::VectorXd bp(np);
		for(int i = 0; i < np; i++)
			bp[i] = xyz[i].z();

		if(!general) {
			// Basis: [x², xy, y², 1]
			Ap.resize(np, 4);
			for(int i = 0; i < np; i++) {
				double wx = xyz[i].x(), wy = xyz[i].y();
				Ap(i, 0) = wx*wx;
				Ap(i, 1) = wx*wy;
				Ap(i, 2) = wy*wy;
				Ap(i, 3) = 1.0;
			}
		} else if(!linear) {
			// Basis: [x⁴, x²y², y⁴, x², xy, y², 1]
			Ap.resize(np, 7);
			for(int i = 0; i < np; i++) {
				double wx = xyz[i].x(), wy = xyz[i].y();
				Ap(i, 0) = wx*wx*wx*wx;
				Ap(i, 1) = wx*wx*wy*wy;
				Ap(i, 2) = wy*wy*wy*wy;
				Ap(i, 3) = wx*wx;
				Ap(i, 4) = wx*wy;
				Ap(i, 5) = wy*wy;
				Ap(i, 6) = 1.0;
			}
		} else {
			// Basis: [x⁴, x²y², y⁴, x², xy, y², x, y, 1]
			Ap.resize(np, 9);
			for(int i = 0; i < np; i++) {
				double wx = xyz[i].x(), wy = xyz[i].y();
				Ap(i, 0) = wx*wx*wx*wx;
				Ap(i, 1) = wx*wx*wy*wy;
				Ap(i, 2) = wy*wy*wy*wy;
				Ap(i, 3) = wx*wx;
				Ap(i, 4) = wx*wy;
				Ap(i, 5) = wy*wy;
				Ap(i, 6) = wx;
				Ap(i, 7) = wy;
				Ap(i, 8) = 1.0;
			}
		}

		Eigen::VectorXd cf = Ap.colPivHouseholderQr().solve(bp);

		cout << "flattenPlaneHeights: fitted surface coefficients (" << np << " points)\n";
		if(!general)
			cout << "  QUADRATIC2: a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2] << " d=" << cf[3] << "\n";
		else if(!linear)
			cout << "  GENERAL4:  a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2]
			     << " d=" << cf[3] << " e=" << cf[4] << " f=" << cf[5] << " g=" << cf[6] << "\n";
		else
			cout << "  GENERAL4L: a=" << cf[0] << " b=" << cf[1] << " c=" << cf[2]
			     << " d=" << cf[3] << " e=" << cf[4] << " f=" << cf[5]
			     << " g=" << cf[6] << " h=" << cf[7] << " k=" << cf[8] << "\n";

		SurfaceCoeffs sc;
		sc.center = center;
		if(!general) {
			sc.nterms = 4;
			sc.c[0] = cf[0]; sc.c[1] = cf[1]; sc.c[2] = cf[2]; sc.c[3] = cf[3];
			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					double wx = x - center.x();
					double wy = center.y() - y;
					heights[x + y*w] -= (float)(cf[0]*wx*wx + cf[1]*wx*wy + cf[2]*wy*wy + cf[3]);
				}
			}
		} else if(!linear) {
			sc.nterms = 7;
			for(int k = 0; k < 7; k++) sc.c[k] = cf[k];
			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					double wx = x - center.x();
					double wy = center.y() - y;
					heights[x + y*w] -= (float)(
						cf[0]*wx*wx*wx*wx + cf[1]*wx*wx*wy*wy + cf[2]*wy*wy*wy*wy +
						cf[3]*wx*wx       + cf[4]*wx*wy       + cf[5]*wy*wy + cf[6]);
				}
			}
		} else {
			sc.nterms = 9;
			for(int k = 0; k < 9; k++) sc.c[k] = cf[k];
			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					double wx = x - center.x();
					double wy = center.y() - y;
					heights[x + y*w] -= (float)(
						cf[0]*wx*wx*wx*wx + cf[1]*wx*wx*wy*wy + cf[2]*wy*wy*wy*wy +
						cf[3]*wx*wx       + cf[4]*wx*wy       + cf[5]*wy*wy +
						cf[6]*wx          + cf[7]*wy          + cf[8]);
				}
			}
		}
		if(out_sc) *out_sc = sc;

		if(correct_normals)
			applyNormalCorrection(w, h, *correct_normals, sc);
	}
#else
	double cx, cy, cz, R2;
	fitSphereN(xyz, cx, cy, cz, R2);

	double sign_val = (cz < mean_z) ? 1.0 : -1.0;

	// Subtract the sphere surface from every height.
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			double wx = x - center.x();
			double wy = center.y() - y;
			double val_c = R2 - (wx-cx)*(wx-cx) - (wy-cy)*(wy-cy);
			double z_sphere = (val_c >= 0.0) ? (cz + sign_val*std::sqrt(val_c)) : cz;
			heights[x + y*w] -= (float)z_sphere;
		}
	}
	if(correct_normals) {
		for(int y = 0; y < h; y++) {
			for(int x = 0; x < w; x++) {
				double wx = x - center.x();
				double wy = center.y() - y;
				double val_c = R2 - wx*wx - wy*wy; // cx=cy=0
				if(val_c < 1e-10) continue;
				double sq = std::sqrt(val_c);
				// outward normal of top/bottom hemisphere at (wx, wy)
				Eigen::Vector3f n_fit((float)(sign_val*wx/sq), (float)(sign_val*wy/sq), 1.0f);
				n_fit.normalize();
				Eigen::Vector3f axis = n_fit.cross(Eigen::Vector3f(0, 0, 1));
				float sin_a = axis.norm();
				if(sin_a < 1e-6f) continue;
				axis /= sin_a;
				Eigen::Matrix3f R = Eigen::AngleAxisf(std::atan2(sin_a, n_fit[2]), axis).toRotationMatrix();
				(*correct_normals)[x + y*w] = (R * (*correct_normals)[x + y*w]).normalized();
			}
		}
	}
#endif

	// Re-centre so mean height is 0.
	double sum = 0;
	for(float v : heights) sum += v;
	float mean_val = (float)(sum / heights.size());
	for(float &v : heights) v -= mean_val;
}


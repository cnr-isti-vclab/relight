#ifndef NORMALSIMAGE_H
#define NORMALSIMAGE_H

#include <QString>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <vector>
#include <Eigen/Core>


void flattenBlurNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double sigma = 10.0);
void flattenRadialNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, double binSize = 20.0);
void flattenFourierNormals(int w, int h, std::vector<Eigen::Vector3f> &normals, float padding = 0.2, double sigma = 20, bool exponential = true);

// Polynomial surface model with no linear (x, y) terms so the extremum is
// always at the image centre (wx=wy=0), unless np > 9 where linear terms are added.
//
// QUADRATIC2 (nterms=4):  z = c[0]*x²  + c[1]*x*y + c[2]*y²  + c[3]
// GENERAL4   (nterms=7):  z = c[0]*x⁴  + c[1]*x²y² + c[2]*y⁴
//                           + c[3]*x²  + c[4]*x*y  + c[5]*y² + c[6]
// GENERAL4L  (nterms=9):  z = c[0]*x⁴  + c[1]*x²y² + c[2]*y⁴
//                           + c[3]*x²  + c[4]*x*y  + c[5]*y²
//                           + c[6]*x   + c[7]*y    + c[8]
//
// QUADRATIC2 when np < 7; GENERAL4 when 7 ≤ np ≤ 9; GENERAL4L when np > 9.
struct SurfaceCoeffs {
	int      nterms;  // 4, 7, or 9
	double   c[9];
	QPointF  center;  // image-centre in pixel coords of the image this was fitted to

	// Gradient of z at (wx, wy)  [wx = x - center.x(), wy = center.y() - y]
	void gradient(double wx, double wy, double &gx, double &gy) const {
		if(nterms == 4) {
			gx = 2*c[0]*wx + c[1]*wy;
			gy = c[1]*wx  + 2*c[2]*wy;
		} else {
			gx = 4*c[0]*wx*wx*wx + 2*c[1]*wx*wy*wy + 2*c[3]*wx + c[4]*wy;
			gy = 2*c[1]*wx*wx*wy + 4*c[2]*wy*wy*wy + c[4]*wx  + 2*c[5]*wy;
			if(nterms == 9) {
				gx += c[6];
				gy += c[7];
			}
		}
	}

	// Return coefficients re-expressed for a resolution scaled by s (new = s * old).
	// 4th-order terms scale by 1/s⁴, 2nd-order by 1/s², 1st-order by 1/s, constant unchanged.
	// Center scales by s.
	SurfaceCoeffs rescaled(double s) const {
		SurfaceCoeffs r = *this;
		double s2 = s*s, s4 = s2*s2;
		if(nterms == 4) {
			r.c[0] = c[0] / s2;
			r.c[1] = c[1] / s2;
			r.c[2] = c[2] / s2;
			// c[3] constant unchanged
		} else {
			r.c[0] = c[0] / s4;
			r.c[1] = c[1] / s4;
			r.c[2] = c[2] / s4;
			r.c[3] = c[3] / s2;
			r.c[4] = c[4] / s2;
			r.c[5] = c[5] / s2;
			if(nterms == 9) {
				r.c[6] = c[6] / s;
				r.c[7] = c[7] / s;
				// c[8] constant unchanged
			}
		}
		r.center = QPointF(center.x() * s, center.y() * s);
		return r;
	}
};

// Apply the surface gradient rotation to every normal in-place.
void applyNormalCorrection(int w, int h, std::vector<Eigen::Vector3f> &normals,
                           const SurfaceCoeffs &sc);

// - flattenPlaneNormals: fit surface slope field from reference-point normals,
//   then apply per-pixel correction. Points are in full-image pixel coordinates.
// - flattenPlaneHeights: fit surface through 3D reference points, subtract from
//   heights.  out_sc receives the fitted model for re-application at other resolutions.
void flattenPlaneNormals(int w, int h, std::vector<Eigen::Vector3f> &normals,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height);
void flattenPlaneHeights(int w, int h, std::vector<float> &heights,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height,
                         std::vector<Eigen::Vector3f> *normals = nullptr,
                         SurfaceCoeffs *out_sc = nullptr);

void flattenRadialHeights(int w, int h, std::vector<float> &heights, double binSize = 20.0);
void flattenFourierHeights(int w, int h, std::vector<float> &heights, float padding = 0.2, double sigma = 20);

class NormalsImage {
public:
	//radial
	bool exponential = true;
	//fourier
	double sigma = 20;
	int padding_amount = 20;

	int w, h;
	QImage img, flat;
	std::vector<double> normals;


	~NormalsImage();

	void load(std::vector<double> &normals, int w, int h);
	void load(QString filename);
	void save(QString filename);

	//fit a line radial normal divergence, bin size 20px

};

#endif // NORMALSIMAGE_H

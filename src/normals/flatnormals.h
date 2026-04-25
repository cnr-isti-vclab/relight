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

// - flattenPlaneNormals: fit a paraboloid slope field to the gradients measured
//   at the reference points, then apply a per-pixel correction rotation so the
//   fitted surface maps to (0,0,1).  Points are in full-image pixel coordinates.
// - flattenPlaneHeights: fit a paraboloid/sphere through the corresponding 3D
//   points and subtract the surface from the height field.
void flattenPlaneNormals(int w, int h, std::vector<Eigen::Vector3f> &normals,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height);
// normals: if non-null, also correct the normals using the fitted surface gradient.
// out_pa, out_pb, out_pd: if non-null, receive the fitted paraboloid coefficients
//   (z = pa*wx² + pb*wy² + pd*wx*wy) so the caller can re-apply to other resolutions.
void flattenPlaneHeights(int w, int h, std::vector<float> &heights,
                         const std::vector<QPointF> &points,
                         const QRect &crop, int image_width, int image_height,
                         std::vector<Eigen::Vector3f> *normals = nullptr,
                         double *out_pa = nullptr, double *out_pb = nullptr,
                         double *out_pd = nullptr);

// Apply the paraboloid gradient rotation to every normal in-place.
// pa, pb, pd are the coefficients returned by flattenPlaneHeights.
void applyParaboloidNormalCorrection(int w, int h, std::vector<Eigen::Vector3f> &normals,
                                     double pa, double pb, double pd, QPointF center);

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

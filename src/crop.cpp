#include "crop.h"
#include <QImage>
#include <Eigen/Core>
#include <cmath>
#include <QRect>

QPointF Crop::imgToCrop(QPointF p, QSize img_size) {
	QTransform rot;
	rot.rotate(angle);
	QRectF rotatedSize = rot.mapRect(QRectF(QPointF(0, 0), img_size));

	QSize center = img_size/2;
	QSizeF new_center = rotatedSize.size()/2;

	QTransform t;
	t.translate(new_center.width(), new_center.height());
	t.rotate(angle);
	t.translate(-center.width(), -center.height());
	QPointF q = t.map(p);
	return q;
}

QPointF Crop::cropToImg(QPointF p, QSize img_size) {
	QTransform rot;
	rot.rotate(angle);
	QRectF rotatedSize = rot.mapRect(QRectF(QPointF(0, 0), img_size));

	QSize center = img_size/2;
	QSizeF new_center = rotatedSize.size()/2;

	QTransform t;
	t.translate(new_center.width(), new_center.height());
	t.rotate(angle);
	t.translate(-center.width(), -center.height());

	QPointF q = t.inverted().map(p);
	return q;
}

QRect Crop::boundingRect(QSize img_size) {
	QSize center = img_size/2;

	QTransform rot;
	rot.rotate(angle);
	QRectF rotatedSize = rot.mapRect(QRectF(QPointF(0, 0), img_size));

	QSizeF new_center = rotatedSize.size()/2;

	//This is the transform applied to the image.
	QTransform t;
	t.translate(new_center.width(), new_center.height());
	t.rotate(angle);
	t.translate(-center.width(), -center.height());

	QTransform invRot = t.inverted();
	QPolygonF cropPolygon = invRot.map(QRectF(rect()));

	return cropPolygon.boundingRect().toAlignedRect();
}

QImage Crop::cropBoundingImage(QImage src) {
	QTransform rotToAlignCrop;
	rotToAlignCrop.rotate(angle);

	QImage rotated = src.transformed(rotToAlignCrop, Qt::SmoothTransformation);

	QSize targetSize = rect().size();
	QPoint center = rotated.rect().center();
	QRect finalCrop(center.x() - targetSize.width() / 2,
					center.y() - targetSize.height() / 2,
					targetSize.width(), targetSize.height());

	finalCrop = finalCrop.intersected(rotated.rect()); // just in case

	return rotated.copy(finalCrop);
}


void bilinearSample(float *n, const std::vector<float>& img, int width, int height, float x, float y) {
	int x0 = int(std::floor(x));
	int y0 = int(std::floor(y));
	for(int k = 0; k < 3; k++) {
		n[k] = img[(y0*width + x0)*3 + k];
	}
	return;
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	if (x0 < 0 || x1 >= width || y0 < 0 || y1 >= height) {
		n[0] = n[1] = n[2] = 0.0f;
		return;
	}

	float fx = x - x0;
	float fy = y - y0;

	for(int k = 0; k < 3; k++) {
		float c00 = img[(y0 * width + x0)*3 + k];
		float c10 = img[(y0 * width + x1)*3 + k];
		float c01 = img[(y1 * width + x0)*3 + k];
		float c11 = img[(y1 * width + x1)*3 + k];

		float c0 = c00 * (1 - fx) + c10 * fx;
		float c1 = c01 * (1 - fx) + c11 * fx;
		n[k] = c0 * (1 - fy) + c1 * fy;
	}
}

std::vector<float> Crop::cropBoundingNormals(
		const std::vector<float>& input,
		int &width, int &height) {

	QTransform rotToAlignCrop;
	rotToAlignCrop.rotate(angle);

	QRect rotatedSize = rotToAlignCrop.mapRect(QRectF(QPointF(0, 0), QPointF(width, height))).toRect();

	QSize targetSize = rect().size();
	QPoint center = QRect(QPoint(0, 0), rotatedSize.size()).center();
	QRect cropRect(center.x() - targetSize.width() / 2,
				   center.y() - targetSize.height() / 2,
				   targetSize.width(), targetSize.height());
	int old_w = width;
	int old_h = height;

	width = cropRect.width();
	height = cropRect.height();

	std::vector<float> cropped(3*cropRect.width() * cropRect.height());

	float cx = old_w / 2.0f;
	float cy = old_h / 2.0f;

	float angleRad = angle * float(M_PI) / 180.0f;
	float cosA = std::cos(angleRad);
	float sinA = std::sin(angleRad);

	for (int y = 0; y < cropRect.height(); ++y) {
		for (int x = 0; x < cropRect.width(); ++x) {
			//xy are the position on the final imagee.
			//they come from a crop of the rotated normals.
			//add crop topleft to get the coords in the rotated normals
			int dstX = cropRect.x() + x;
			int dstY = cropRect.y() + y;

			//We will rotate around the center of the rotated normals
			//first place the zero in the center of the
			//rotated thing
			float dx = dstX - rotatedSize.width()/2.0f;
			float dy = dstY - rotatedSize.height()/2.0f;
			//nor rotate return to coordinates top left of the initial image
			float srcX = cx + dx * cosA + dy * sinA;
			float srcY = cy - dx * sinA + dy * cosA;


			float *n = &cropped[(y * cropRect.width() + x)*3];
			bilinearSample(n, input, old_w, old_h, srcX, srcY);
		}
	}

	return cropped;
}

std::vector<Eigen::Vector3f> Crop::cropBoundingNormals(
		const std::vector<Eigen::Vector3f> &input,
		int &width, int &height) {
	// Convert Eigen vectors to flat float buffer, call existing implementation and convert back
	std::vector<float> flat(input.size()*3);
	for(size_t i = 0; i < input.size(); i++) {
		flat[3*i + 0] = input[i][0];
		flat[3*i + 1] = input[i][1];
		flat[3*i + 2] = input[i][2];
	}
	int w = width;
	int h = height;
	std::vector<float> cropped = cropBoundingNormals(flat, w, h);
	// convert back
	std::vector<Eigen::Vector3f> out(w*h);
	for(int i = 0; i < w*h; i++) {
		out[i][0] = cropped[3*i + 0];
		out[i][1] = cropped[3*i + 1];
		out[i][2] = cropped[3*i + 2];
	}
	width = w;
	height = h;
	return out;
}

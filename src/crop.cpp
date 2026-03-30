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

QRect Crop::boundingRect(QSize img_size) const {
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


Eigen::Vector3f bilinearSample(const std::vector<Eigen::Vector3f>& img, int width, int height, float x, float y) {
	Eigen::Vector3f n(0.0f, 0.0f, 0.0f);
	int x0 = std::min(width-1, std::max(0, int(std::floor(x))));
	int y0 = std::min(height-1, std::max(0, int(std::floor(y))));
	/*
	for(int k = 0; k < 3; k++) {
		n[k] = img[(y0*width + x0)*3 + k];
	}
	return n; */

	int x1 = std::min(width  - 1, x0 + 1);
	int y1 = std::min(height - 1, y0 + 1);

	float fx = x - x0;
	float fy = y - y0;

	for(int k = 0; k < 3; k++) {
		float c00 = img[(y0 * width + x0)][k];
		float c10 = img[(y0 * width + x1)][k];
		float c01 = img[(y1 * width + x0)][k];
		float c11 = img[(y1 * width + x1)][k];

		float c0 = c00 * (1 - fx) + c10 * fx;
		float c1 = c01 * (1 - fx) + c11 * fx;
		n[k] = c0 * (1 - fy) + c1 * fy;
	}
	return n;
}

std::vector<Eigen::Vector3f> Crop::cropBoundingNormals(
	const std::vector<Eigen::Vector3f>& input,
		int &width, int &height) {

	QTransform rotToAlignCrop;
	rotToAlignCrop.rotate(angle);

	QRect rotatedSize = rotToAlignCrop.mapRect(QRectF(QPointF(0, 0), QPointF(width, height))).toRect();

	QSize targetSize = rect().size();
	QPoint center = QRect(QPoint(0, 0), rotatedSize.size()).center();
	int rx = std::max(0, center.x() - targetSize.width()  / 2);
	int ry = std::max(0, center.y() - targetSize.height() / 2);

	int old_w = width;
	int old_h = height;

	width = targetSize.width();
	height = targetSize.height();

	std::vector<Eigen::Vector3f> cropped(width * height);

	float cx = old_w / 2.0f;
	float cy = old_h / 2.0f;

	float angleRad = angle * float(M_PI) / 180.0f;
	float cosA = std::cos(angleRad);
	float sinA = std::sin(angleRad);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			//xy are the position on the final imagee.
			//they come from a crop of the rotated normals.
			//add crop topleft to get the coords in the rotated normals
			int dstX = rx + x;
			int dstY = ry + y;

			//We will rotate around the center of the rotated normals
			//first place the zero in the center of the
			//rotated thing
			float dx = dstX - rotatedSize.width()/2.0f;
			float dy = dstY - rotatedSize.height()/2.0f;
			//nor rotate return to coordinates top left of the initial image
			float srcX = cx + dx * cosA + dy * sinA;
			float srcY = cy - dx * sinA + dy * cosA;
			//assert(srcX < old_w && srcY < old_h);
			cropped[y * width + x] = bilinearSample(input, old_w, old_h, srcX, srcY);
		}
	}

	return cropped;
}

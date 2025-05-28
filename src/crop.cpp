#include "crop.h"
#include <QImage>
#include <Eigen/Core>
#include <cmath>
#include <QRect>

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
	QPolygonF cropPolygon = invRot.map(QRectF(*this));

	return cropPolygon.boundingRect().toAlignedRect();
}

QImage Crop::cropBoundingImage(QImage src, QSize img_size) {
	QTransform rotToAlignCrop;
	rotToAlignCrop.rotate(angle);

	QImage rotated = src.transformed(rotToAlignCrop, Qt::SmoothTransformation);
	rotated.save("/home/ponchio/rotated.jpg");


	QSize targetSize = QRect(*this).size();
	QPoint center = rotated.rect().center();
	QRect finalCrop(center.x() - targetSize.width() / 2,
					center.y() - targetSize.height() / 2,
					targetSize.width(), targetSize.height());

	finalCrop = finalCrop.intersected(rotated.rect()); // just in case

	return rotated.copy(finalCrop);
}



using Vector3f = Eigen::Vector3f;

Vector3f bilinearSample(const std::vector<Vector3f>& img, int width, int height, float x, float y) {
	int x0 = int(std::floor(x));
	int y0 = int(std::floor(y));
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	if (x0 < 0 || x1 >= width || y0 < 0 || y1 >= height)
		return Vector3f(0, 0, 0); // or border handling

	float fx = x - x0;
	float fy = y - y0;

	Vector3f c00 = img[y0 * width + x0];
	Vector3f c10 = img[y0 * width + x1];
	Vector3f c01 = img[y1 * width + x0];
	Vector3f c11 = img[y1 * width + x1];

	Vector3f c0 = c00 * (1 - fx) + c10 * fx;
	Vector3f c1 = c01 * (1 - fx) + c11 * fx;
	return c0 * (1 - fy) + c1 * fy;
}

std::vector<Eigen::Vector3f> Crop::cropBoundingNormals(std::vector<Eigen::Vector3f> input, int w, int h, QSize img_size) {
	QTransform rotToAlignCrop;
	rotToAlignCrop.rotate(angle);

	//QImage rotated = src.transformed(rotToAlignCrop, Qt::SmoothTransformation);
	//rotated.save("/home/ponchio/rotated.jpg");
	/*QRectF rotatedSize = rotToAlignCrop.mapRect(QRectF(QPointF(0, 0), w, h));


	QSize targetSize = QRect(*this).size();
	QPoint center = rotatedSize.center();
	QRect finalCrop(center.x() - targetSize.width() / 2,
					center.y() - targetSize.height() / 2,
					targetSize.width(), targetSize.height());

	finalCrop = finalCrop.intersected(rotatedSize); // just in case
	return rotatedAndCropImage(input, w, h, angle, finalCrop); */
}

std::vector<Vector3f> rotateAndCropImage(
	const std::vector<Vector3f>& input,
	int width, int height,
	float angleDeg,
	QRect cropRect // crop rect in rotated image space
) {
	int rotWidth = width;
	int rotHeight = height;

	std::vector<Vector3f> cropped(cropRect.width() * cropRect.height());

	float cx = width / 2.0f;
	float cy = height / 2.0f;

	float angleRad = angleDeg * float(M_PI) / 180.0f;
	float cosA = std::cos(angleRad);
	float sinA = std::sin(angleRad);

	for (int y = 0; y < cropRect.height(); ++y) {
		for (int x = 0; x < cropRect.width(); ++x) {
			int dstX = cropRect.x() + x;
			int dstY = cropRect.y() + y;

			// Compute source position in original image via inverse rotation
			float dx = dstX - cx;
			float dy = dstY - cy;

			float srcX = cx + dx * cosA + dy * sinA;
			float srcY = cy - dx * sinA + dy * cosA;

			cropped[y * cropRect.width() + x] = bilinearSample(input, width, height, srcX, srcY);
		}
	}

	return cropped;
}

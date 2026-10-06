#ifndef AUTOALIGN_H
#define AUTOALIGN_H

#include <vector>
#include <functional>

#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>

class QImage;

/* Finds the translation of each image relative to the first one (experimental).
   The images are downscaled and split into overlapping tiles. Each tile is matched with phase correlation of the
   gradient magnitude, which depends little on the light direction. The tiles whose shifts agree best with the
   other tiles (flat, textured areas, where the shading does not move the edges) give the result. */

class AutoAlign {
public:
	const int max_side = 1536; //images are downscaled to this size.

	//returns image i downscaled to size.
	typedef std::function<QImage(int i, QSize size)> Loader;

	AutoAlign(QSize image_size);
	//pixel (x, y) of the first image is at (x, y) + offset in image i. Throws QString on failure or cancel.
	std::vector<QPointF> run(int n, Loader load, std::function<bool(QString stage, int percent)> *callback = nullptr);

	QRect region; //full resolution area of the tile that matched best, after run().

private:
	QSize image_size;
	int w = 0, h = 0; //downscaled size
	float scale = 1.0f; //full size / downscaled size
	int side = 256; //tile size, in downscaled pixels
};

#endif // AUTOALIGN_H

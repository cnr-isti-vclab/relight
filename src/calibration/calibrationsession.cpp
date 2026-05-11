#include "calibrationsession.h"

#include "../image_decoder.h"
#include "../lp.h"

#include <QImageReader>
#include <QFileInfo>

#include <cassert>
#include <fstream>

/* readImage – same logic as Project::readImage():
 * 1. Try Qt built-in reader (JPEG, PNG, …)
 * 2. Fall back to ImageDecoder (TIFF, EXR, RAW, …)     */
QImage CalibrationSession::readImage(int i) {
	assert(i >= 0 && i < (int)images.size());

	QImageReader reader(images[i].filename);
	reader.setAutoTransform(false);
	QImage img = reader.read();
	if(!img.isNull())
		return img;

	ImageDecoder dec;
	int w = 0, h = 0;
	if(!dec.init(images[i].filename.toStdString().c_str(), w, h))
		return QImage();

	int ch = dec.numChannels();
	size_t rowBytes = dec.rowSize();
	std::vector<uint8_t> buf(size_t(h) * rowBytes);
	dec.readRows(h, buf.data());
	dec.finish();

	QImage::Format fmt = (ch == 4) ? QImage::Format_RGBA8888 :
	                     (ch == 1) ? QImage::Format_Grayscale8 :
	                                 QImage::Format_RGB888;
	QImage result(w, h, fmt);
	for(int y = 0; y < h; ++y)
		memcpy(result.scanLine(y), buf.data() + size_t(y) * rowBytes, rowBytes);
	return result;
}

void CalibrationSession::addImages(const QStringList &paths) {
	for(const QString &path : paths)
		images.push_back(Image(path));
}

void CalibrationSession::removeImage(int i) {
	assert(i >= 0 && i < (int)images.size());
	images.erase(images.begin() + i);
}

bool CalibrationSession::loadLP(const QString &path) {
	std::vector<Eigen::Vector3f> dirs;
	std::vector<QString> filenames;
	parseLP(path, dirs, filenames);
	if(dirs.empty())
		return false;
	dome.directions = dirs;
	dome.lightSource = Dome::FROM_LP;
	return true;
}

bool CalibrationSession::saveLP(const QString &path) const {
	std::ofstream out(path.toStdString());
	if(!out.is_open())
		return false;
	out << images.size() << "\n";
	for(int i = 0; i < (int)images.size() && i < (int)dome.directions.size(); i++) {
		QFileInfo fi(images[i].filename);
		const auto &d = dome.directions[i];
		out << fi.fileName().toStdString() << " "
		    << d.x() << " " << d.y() << " " << d.z() << "\n";
	}
	return true;
}

bool CalibrationSession::loadJson(const QString &/*path*/) {
	// TODO: deserialise full session bundle (images + dome + lens_calib + flatfield)
	return false;
}

bool CalibrationSession::saveJson(const QString &/*path*/) const {
	// TODO: serialise full session bundle
	return false;
}

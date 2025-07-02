#include "rti.h"
#include "jpeg_decoder.h"
#include "imageset.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>

#include <Eigen/Core>

#include <math.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <assert.h>

using namespace std;
using namespace Eigen;

bool Rti::load(const char *filename, bool loadPlanes) {

	QDir dir;

	QFileInfo info(filename);
	if(info.isDir())
		dir.setPath(filename);
	else
		dir = info.dir();

	//read the json
	QFile file(dir.filePath("info.json"));
	if(!file.open(QFile::ReadOnly)) {
		error = "Could not open file: ";
		return false;
	}
	QByteArray json = file.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(json);
	if(doc.isNull()) {
		error =  "Invalid json.\n";
		return false;
	}
	QJsonObject obj = doc.object();\

	width = obj["width"].toInt();
	height = obj["height"].toInt();
	quality = obj["quality"].toInt();

	map<string, Type> types = {{"ptm", PTM}, {"hsh", HSH}, {"sh", SH}, {"h", H}, {"dmd", DMD}, {"rbf", RBF}, {"bilinear", BILINEAR }};
	QString t = obj["type"].toString();
	if(!types.count(qPrintable(t))) {
		error = "Unknown basis type: " + t.toStdString();
		return false;
	}
	type = types[qPrintable(t)];

	if(type == BILINEAR) {
		resolution = obj["resolution"].toInt();
		ndimensions = resolution*resolution;
	}

	if(type == RBF) {
		sigma = float(obj["sigma"].toDouble());
		QJsonArray jlights = obj["lights"].toArray();
		lights.resize(jlights.size()/3);
		for(size_t i = 0; i < lights.size(); i++) {
			Vector3f &l = lights[i];
			l[0] = float(jlights[i*3+0].toDouble());
			l[1] = float(jlights[i*3+1].toDouble());
			l[2] = float(jlights[i*3+2].toDouble());
		}
		ndimensions = lights.size();
	}


	map<string, ColorSpace> spaces = {{"rgb", RGB}, {"lrgb", LRGB}, {"ycc", YCC}, {"mrgb", MRGB }, { "mycc", MYCC }};
	QString c = obj["colorspace"].toString();
	if(!spaces.count(qPrintable(c))) {
		error = "Unknown basis type: " + c.toStdString();
		return false;
	}
	colorspace = spaces[qPrintable(c)];

	if(colorspace == MYCC) {
		QJsonArray ycc = obj["yccplanes"].toArray();
		if(ycc.size() != 3) {
			error = "Expecting 3 numbers for yccplanes";
			return false;
		}
		yccplanes[0] = ycc[0].toInt();
		yccplanes[1] = ycc[1].toInt();
		yccplanes[2] = ycc[2].toInt();
		nplanes = yccplanes[0] + yccplanes[1] + yccplanes[2];
	} else
		nplanes = obj["nplanes"].toInt();

	QJsonArray mats = obj["materials"].toArray();
	int nmaterials = mats.size();
	assert(nmaterials == 1);

	QJsonObject jm = mats[0].toObject();
	QJsonArray range = jm["range"].toArray();
	QJsonArray offset = jm["bias"].toArray();
	QJsonArray scale = jm["scale"].toArray();
	material.planes.resize(nplanes);
	for(size_t p = 0; p < nplanes; p++) {
		Material::Plane &plane = material.planes[p];
		if(range.size())
			plane.range = range[p].toDouble();
		plane.scale = scale[p].toDouble();
		plane.bias = offset[p].toDouble();
	}

	if(colorspace == MRGB || colorspace == MYCC) {
		size_t basesize = nmaterials * (nplanes+1)*3*ndimensions;
		basis.resize(basesize);

		/*QFile matfile(dir.filePath("materials.bin"));
		if(!matfile.open(QFile::ReadOnly)) {
			cerr << "Could not open materials.bin\n";
			return false;
		}*/

		QJsonArray jbasis = obj["basis"].toArray();

		for(uint32_t p = 0; p < nplanes+1; p++) {
			for(uint32_t c = 0; c < ndimensions; c++) {
				for(int k = 0; k < 3; k++) {
					uint32_t o = (p*ndimensions + c)*3 + k;
					float c = float(jbasis[o].toDouble());
					if(p == 0)
						basis[o] = c;
					else
						basis[o] = ((c - 127.0f)/material.planes[p-1].range);
				}
			}
		}
	}
	if(loadPlanes)
		return loadData(dir.path().toStdString().c_str());
	return true;
}

bool Rti::loadData(const char *folder) {
	QDir dir(folder);
	int njpegs = (nplanes-1)/3 + 1;

	std::vector<JpegDecoder *> planedecs(njpegs);
	for(int i = 0; i < njpegs; i++) {
		planedecs[i] = new JpegDecoder;
		planedecs[i]->setColorSpace(JCS_RGB);

		int w, h;
		planedecs[i]->init(dir.filePath(QString("plane_%1.jpg").arg(i)).toStdString().c_str(), w, h);
	}

	size_t rowSize = width*3;
	vector<uint8_t> row(rowSize);  //buffer for a row read in the jpeg

	planes.resize(nplanes);
	for(auto &p: planes)
		p.resize(width*height);

	for(uint32_t y = 0; y < height; y++) {

		//read the plane jpegs
		for(uint32_t i = 0; i < planedecs.size(); i++) {
			JpegDecoder *dec = planedecs[i];
			dec->readRows(1, row.data());
			for(uint32_t x = 0; x < width; x++) {
				uint32_t o = x + y*width;
				planes[i*3][o] = row[x*3 + 0];
				if(i*3+1 < nplanes)
					planes[i*3+1][o] = row[x*3 + 1];
				if(i*3+2 < nplanes)
					planes[i*3+2][o] = row[x*3 + 2];
			}
		}
	}

	for(size_t i = 0; i < planedecs.size(); i++) {
		//planedecs[i]->finish();
		delete planedecs[i];
	}

	headersize = 0;
	QFileInfo i_info(dir.filePath("info.json"));
	headersize += i_info.size();

	QFileInfo m_info(dir.filePath("materials.bin"));
	headersize += m_info.size();

	filesize = headersize;
	for(int p = 0; p < njpegs; p++) {
		QFileInfo p_info(dir.filePath(QString("plane_%1.jpg").arg(p)));
		size_t psize = p_info.size();
		filesize += psize;
		planesize.push_back(psize/3);
		planesize.push_back(psize/3);
		planesize.push_back(psize/3);
	}
	return true;
}


void Rti::clip(int left, int bottom, int right, int top) {
	assert(left >= 0 && right > left && right  <= (int)width);
	assert(bottom >= 0 && top > bottom && top  <= (int)height);

	int w = right - left;
	int h = top - bottom;
	vector<vector<uint8_t>> tmp(9);
	for(auto &d: tmp)
		d.resize(w*h);

	for(int k = 0; k < 9; k++) {
		for(int y = bottom, dy = 0; y < top; y++, dy++) {
			memcpy(tmp[k].data() + dy*w, planes[k].data() + y*width + left, w);
		}
	}
	for(int k = 0; k < 9; k++)
		swap(tmp[k], planes[k]);
	width = w;
	height = h;
}


Rti Rti::clipped(int left, int bottom, int right, int top) {
	//NRO optimization avoid copying this object in return.
	Rti tmp = *this;

	assert(left >= 0 && right > left && right  <= (int)width);
	assert(bottom >= 0 && top > bottom && top  <= (int)height);

	int w = tmp.width = right - left;
	int h = tmp.height = top - bottom;
	for(auto &d: tmp.planes)
		d.resize(w*h);

	for(int k = 0; k < 9; k++)
		for(int y = bottom, dy = 0; y < top; y++, dy++)
			memcpy(tmp.planes[k].data() + dy*w, planes[k].data() + y*width + left, w);
	return tmp;
}

void Rti::render(float lx, float ly, uint8_t *buffer, int stride, uint32_t renderplanes ) {
	if(stride == 4)
		for(size_t i = 0; i < width*height; i++)
			buffer[i*4+3] = 255;
	if(renderplanes == 0)
		renderplanes = nplanes;

	vector<float> lweights = lightWeights(lx, ly);

	switch(colorspace) {
	case LRGB: {
		for(size_t y = 0; y < height; y++) {
			for(size_t x = 0; x < width; x++) {
				size_t i = x + y*width;
				//size_t j = x + (height-y-1)*width;
				float l = 0;
				for(uint32_t p = 3; p < nplanes; p++)
					l += lweights[p-3]*material.planes[p].dequantize(planes[p][i]);
				l /= 255.0;
				//this should be in the range [0-255];
				buffer[i*stride+0] = std::max(0, std::min(255, (int)(l*planes[0][i])));
				buffer[i*stride+1] = std::max(0, std::min(255, (int)(l*planes[1][i])));
				buffer[i*stride+2] = std::max(0, std::min(255, (int)(l*planes[2][i])));
			}
		}
		break;
	}
	case RGB: {

		for(uint32_t i = 0; i < width*height; i++) {
			for(int c = 0; c < 3; c++) {
				float l = 0.0f;
				for(uint32_t p = c; p < nplanes; p += 3)
					l += lweights[p/3]*material.planes[p].dequantize(planes[p][i]);
				buffer[i*stride+c] = std::max(0, std::min(255, (int)(l)));
			}
		}
		break;
	}
	case YCC: {
		for(uint32_t i = 0; i < width*height; i++) {
			Color3f color(0.0f, 0.0f, 0.0f);
			color[1] = material.planes[1].dequantize(planes[1][i]);
			color[2] = material.planes[2].dequantize(planes[2][i]);

			for(uint32_t p = 0; p < nplanes; p += 3)
				color[0] += lweights[p/3]* material.planes[p+0].dequantize(planes[p+0][i]);

			color *= 1/255.0f;
			color = color.YCbCrToRgb();
			color *= 255.0f;
			for(int c = 0; c < 3; c++)
				buffer[i*stride+c] = std::max(0, std::min(255, (int)(color[c])));
		}
		break;
	}
	case MYCC:
	case MRGB:
		for(uint32_t y = 0; y < height; y++) {
			//compare the two values
			for(uint32_t x = 0; x < width; x++) {

				uint32_t off = 0;//3*(nplanes+1);
				Color3f c(0.0f, 0.0f, 0.0f);
				c[0] = lweights[off + 0];
				c[1] = lweights[off + 1];
				c[2] = lweights[off + 2];

				if(colorspace == MRGB) {
					for(uint32_t p = 0; p < renderplanes; p++) {
						Material::Plane &plane = material.planes[p];
						float val = plane.dequantize(planes[p][x + y*width]);
						c.r += val*lweights[off + 3*(p+1) + 0];
						c.g += val*lweights[off + 3*(p+1) + 1];
						c.b += val*lweights[off + 3*(p+1) + 2];
					}

				} else { //MYCC
					for(uint32_t p = 0; p < yccplanes[1]; p++) {
						for(int k = 0; k < 3; k++) {
							Material::Plane &plane = material.planes[p*3 + k];
							float val = plane.dequantize(planes[p*3 + k][x + y*width]);
							c[k] += val*lweights[off + 3*(p*3 + k + 1) + k];
						}
					}
					for(uint32_t p = yccplanes[1]*3; p < renderplanes; p++) {
						Material::Plane &plane = material.planes[p];
						float val = plane.dequantize(planes[p][x + y*width]);
						c.r += val*lweights[off + 3*(p+1) + 0];
					}
				}
				//ToDO cleanup
				if(colorspace == MYCC)
					c = c.toRgb();

				if(gammaFix) {
					c.r /= sqrt(255.0f);
					c.g /= sqrt(255.0f);
					c.b /= sqrt(255.0f);
					c.r *= c.r;
					c.g *= c.g;
					c.b *= c.b;
				}

				buffer[(x + y*width)*stride + 0] = std::max(0, std::min(255, (int)c.r));
				buffer[(x + y*width)*stride + 1] = std::max(0, std::min(255, (int)c.g));
				buffer[(x + y*width)*stride + 2] = std::max(0, std::min(255, (int)c.b));
			}
		}
		break;
	}
}

std::vector<float> Rti::lightWeights(float lx, float ly) {
	switch(type) {
	case PTM:      return lightWeightsPtm(lx, ly);
	case HSH:      return lightWeightsHsh(lx, ly);
	case SH:       return lightWeightsSh(lx, ly);
	case H:        return lightWeightsH(lx, ly);
	case DMD:      return lightWeightsDmd(lx, ly);
	case RBF:      return lightWeightsRbf(lx, ly);
	case BILINEAR: return lightWeightsBilinear(lx, ly);
	default: return vector<float>();
	}
}

std::vector<float> Rti::lightWeightsPtm(float lx, float ly) {
	// 1, x, y, xx, xy, yy,   xxx xxy xyy yyy etc.
	if(colorspace == RGB)
		assert(nplanes %3 == 0);

	uint32_t nweights = (colorspace == LRGB)? (nplanes-3) : nplanes/3;

	uint32_t count = 0;
	int degree = 0;
	vector<float> coeffs(nweights);
	while(true) {
		for(int k = 0; k <= degree; k++) {
			float c = 1.0;
			for(int j = 0; j < k; j++)
				c *= ly;
			for(int j = k; j < degree; j++)
				c *= lx;

			coeffs[count++] = c;
			if(count == nweights)
				goto done;
		}
		degree++;
	}

done:

	return coeffs;
}

std::vector<float> Rti::lightWeightsDmd(float /*lx*/, float /*ly*/) {
	cerr << "Not implemented!" << endl;
	if(colorspace == RGB)
		assert(nplanes %3 == 0);

	uint32_t nweights = (colorspace == LRGB)? (nplanes-3) : nplanes/3;

	vector<float> coeffs(nweights);

	return coeffs;
}

std::vector<float> Rti::lightWeightsHsh(float lx, float ly) {
	float lz = sqrt(1.0f - lx*lx - ly*ly);
	float phi = atan2(ly, lx);
	if (phi < 0.0f)
		phi = 2.0f * M_PI + phi;
	float theta = std::min(acos(lz), (float)M_PI / 2.0f - 0.01f);

	float cosP = cos(phi);
	float cosT = cos(theta);
	float cosT2 = cosT * cosT;

	float sz = sqrt(lz/(1 + lz));

	vector<float> xweights(9);
	vector<float> lweights(9);
	xweights[0] = lweights[0] = 1.0f / sqrt(2.0f * M_PI);

	lweights[1] = sqrt(6.0f / M_PI) * (cosP * sqrt(cosT-cosT2));
	xweights[1] = sqrt(6.0f / M_PI) * lx * sz;

	lweights[2] = sqrt(3.0f / (2.0f * M_PI)) * (-1.0f + 2.0f*cosT);
	xweights[2] = sqrt(3.0f / (2.0f *M_PI)) * (2.0* lz - 1);

	lweights[3] = sqrt(6.0f / M_PI) * (sqrt(cosT - cosT2) * sin(phi));
	xweights[3] = sqrt(6.0f / M_PI) * ly * sz;


	lweights[4] = sqrt(30.0f / M_PI) * (cos(2.0f * phi) * (-cosT + cosT2));
	xweights[4] = -sqrt(30.0f / M_PI) * (lx*lx - ly*ly)* lz / (1 + lz);

	lweights[5] = sqrt(30.0f / M_PI) * (cosP*(-1.0f + 2.0f * cosT) * sqrt(cosT - cosT2));
	xweights[5] = sqrt(30.0f / M_PI) * lx * (2*lz - 1)*sz;

	lweights[6] = sqrt(5.0f / (2.0f * M_PI)) * (1.0f - 6.0f * cosT + 6.0f * cosT2);
	xweights[6] = sqrt(5.0f / (2.0f * M_PI)) *(6.0f *lz * lz - 6.0f *lz + 1);

	lweights[7] = sqrt(30.0f / M_PI) * ((-1.0f + 2.0f * cosT) * sqrt(cosT - cosT2) * sin(phi));
	xweights[7] = sqrt(30.0f / M_PI) * ly * (2*lz - 1)*sz;

	lweights[8] = sqrt(30.0f / M_PI) * ((-cosT + cosT2) * sin(2.0f*phi));
	xweights[8] = -2.0*sqrt(30.0f / M_PI) * lx * ly *lz / (1 + lz);

	return lweights;
}

std::vector<float> Rti::lightWeightsSh(float lx, float ly, float lz) {
	if(lz == 0.0f)
		lz = sqrt(1.0f - lx*lx - ly*ly);

	const float c0 = 0.282095f;
	 const float c1 = 0.488603f;
	 const float c2 = 1.092548f;
	 const float c3 = 0.315392f;
	 const float c4 = 0.546274f;

	 return {
		 c0,                     // Y00
		 c1 * ly,                 // Y1-1
		 c1 * lz,                 // Y10
		 c1 * lx,                 // Y11
		 c2 * lx * ly,             // Y2-2
		 c2 * ly * lz,             // Y2-1
		 c3 * (3 * lz * lz - 1),   // Y20
		 c2 * lx * lz,             // Y21
		 c4 * (lx * lx - ly * ly)    // Y22
	 };
/*	float lz = sqrt(1.0f - lx*lx - ly*ly);
	float phi = atan2(ly, lx);
	if (phi < 0.0f)
		phi = 2.0f * M_PI + phi;
	float theta = std::min(acos(lz), (float)M_PI / 2.0f - 0.01f);


	float sinP = sin(phi);
	float cosP = cos(phi);
	float cosT = cos(theta);
	float sinT = sin(theta);


	vector<float> xweights(9);
	vector<float> lweights(9);
	xweights[0] = lweights[0] = 0.5 / sqrt(M_PI);

	lweights[1] = 0.5*sqrt(3.0f / M_PI) * sinP*sinT;
	xweights[1] = 0.5*sqrt(3.0f / M_PI) * lx;

	lweights[2] = 0.5*sqrt(3.0f / M_PI) * cosT;
	xweights[2] = 0.5*sqrt(3.0f / M_PI) * lz;

	lweights[3] = 0.5*sqrt(3.0f / M_PI) * cosP*sinT;
	xweights[3] = 0.5*sqrt(3.0f / M_PI) * ly;

	lweights[4] = 0.25*sqrt(15 / M_PI) * sin(2*phi)*sinT*sinT;
	xweights[4] = 0.5*sqrt(15 / M_PI) * lx*ly;

	lweights[5] = 0.5*sqrt(15 / M_PI) * sinT * cosT * sinP;
	xweights[5] = 0.5*sqrt(15 / M_PI) * lx*lz;

	lweights[6] = 0.25*sqrt(5 / M_PI)*(3*cosT*cosT - 1);
	xweights[6] = 0.25*sqrt(5 / M_PI) * (3*lz*lz -1);

	lweights[7] = 0.5*sqrt(15 / M_PI) * sinT*cosT*cosP;
	xweights[7] = 0.5*sqrt(15 / M_PI) *lz *ly;

	lweights[8] = 0.25*sqrt(15 / M_PI)*sinT*sinT*cos(2*phi);
	xweights[8] = 0.25*sqrt(15 / M_PI) *(-lx*lx + ly*ly);

	return lweights;*/
}

std::vector<float> Rti::lightWeightsH(float /*lx*/, float /*ly*/) {
//	float lz = sqrt(1.0f - lx*lx - ly*ly);
	//TODO
	throw 1;
}

std::vector<float> Rti::rbfWeights(float lx, float ly) {
	vector<float> weights(lights.size());

	float radius = 49.0f;
	float lz = sqrt(1 - lx*lx - ly*ly);
	Vector3f n(lx, ly, lz);

	//compute rbf weights
	float totw = 0.0f;
	for(size_t i = 0; i < lights.size(); i++) {
		float d2 = (n - lights[i]).squaredNorm();
		float w = exp(-radius * d2);

		weights[i] = w;
		totw += w;
	}
	for(float &w: weights)
		w /= totw;

	return weights;
}

std::vector<float> Rti::lightWeightsRbf(float lx, float ly) {
	size_t np = nplanes;
	
	//float radius = 49.0f; //previsoou sigma value
	float radius = 1.0f/(sigma*sigma);
	float lz = sqrt(1 - lx*lx - ly*ly);

	vector<pair<int, float>> weights(lights.size());
	Vector3f n(lx, ly, lz);

	//compute rbf weights
	float totw = 0.0f;
	for(size_t i = 0; i < lights.size(); i++) {
		float d2 = (n - lights[i]).squaredNorm();
		float w = exp(-radius * d2);
		//float w = sqrt(sigma*sigma + d2);

		weights[i] = std::make_pair(i, w);
		totw += w;
	}
	//pick only most significant and renormalize
	float retotw = 0.0f;
	int count = 0;
	for(size_t i = 0; i < weights.size(); i++) {
		float &w = weights[i].second;

		w /= totw;
		if(w > 0.001) { //might fail for extreme smoothing.
			weights[count++] =  weights[i];
			retotw += w;
		}
	}
	weights.resize(count);
	for(auto &w: weights)
		w.second /= retotw;

	//now iterate basis:
	vector<float> lweights((np+1) * 3, 0.0f);
	for(size_t p = 0; p < np+1; p++) {
		for(size_t k = 0; k < 3; k++) {
			float &w = lweights[3*p + k];
			for(auto &light: weights) {
				int o = (p*ndimensions + light.first)*3 + k;
				w += light.second*basis[o];
			}
		}
	}
	return lweights;
}

size_t Rti::basePixelOffset(size_t m, size_t p, size_t x, size_t y, size_t k) {
	return ((m*(nplanes+1) + p)*ndimensions + (x + y*resolution))*3 + k;
}

std::vector<float> Rti::lightWeightsBilinear(float lx, float ly) {
	size_t np = nplanes;
	float lz = sqrt(1 - lx*lx - ly*ly);
	float s = fabs(lx) + fabs(ly) + fabs(lz);
	//rotate 45 deg.
	float x = (lx + ly)/s;
	float y = (ly - lx)/s;

	x = (x + 1.0f)/2.0f;
	y = (y + 1.0f)/2.0f;
	x = x*(resolution - 1.0f);
	y = y*(resolution - 1.0f);

	float sx = std::min(int(resolution-2), std::max(0, int(floor(x))));
	float sy = std::min(int(resolution-2), std::max(0, int(floor(y))));
	float dx = x - sx;
	float dy = y - sy;

	float s00 = (1 - dx)*(1 - dy);
	float s10 =      dx *(1 - dy);
	float s01 = (1 - dx)* dy;
	float s11 =      dx * dy;

	vector<float> lweights((np+1) * 3, 0.0f);

	//TODO optimize away basePixel
	for(size_t p = 0; p < np+1; p++) {
		for(size_t k = 0; k < 3; k++) {
			float &w = lweights[3*(p) + k];
			int o00 = basePixelOffset(0, p, sx, sy, k);
			w += s00*basis[o00];

			int o10 = basePixelOffset(0, p, sx+1, sy, k);
			w += s10*basis[o10];

			int o01 = basePixelOffset(0, p, sx, sy+1, k);
			w += s01*basis[o01];

			int o11 = basePixelOffset(0, p, sx+1, sy+1, k);
			w += s11*basis[o11];
		}
	}
	return lweights;
}

QRgb ramp(uint8_t t) {
	int r, g, b;
	if(t < 128) {
		r = 0;
		g = 2*t;
		b = 255 - 2*t;
	} else {
		b = 0;
		g = 255 - 2*t;
		r = 2*t;
	}
	return qRgb(r, g, b);
}

QRgb ramp(float v, float min, float max) {
	v = (v - min)/(max - min);
	v = std::max(0.0f, std::min(1.0f, v));
	return ramp((int8_t)(255*v));
}

double Rti::evaluateError(ImageSet &imageset, Rti &rti, QString output, int reference) {

	uint32_t size = rti.width*rti.height*3;
	vector<uint8_t> original(size);
	vector<uint8_t> buffer(size);

	uint32_t nlights = imageset.lights().size();
	vector<float> errors(rti.width*rti.height, 0.0f);
	double tot = 0.0;

	int count = 0;
	for(int nl = 0; nl < (int)nlights; nl++) {
		if(reference >= 0 && nl != reference) {
			continue;
		}

		count++;
		Vector3f &light = imageset.lights()[nl];

		rti.render(light[0], light[1], buffer.data());

		imageset.decode(nl, original.data());
		
		/*		if(nl == reference) {
			QImage img(rti.width, rti.height, QImage::Format_RGB888);
			rti.render(light[0], light[1], img.bits());
			img.save(QString("%1.png").arg(nl));
		}*/

		double e = 0.0;
		for(uint32_t i = 0; i < size; i++) {
			double d = (double)original[i] - (double)buffer[i];
			e += d*d;
			errors[i/3] += d*d;
		}

		tot += e/size;
	}
	tot /= count;

	//double psnr = 20*log10(255.0) - 10*log10(tot);

	if(!output.isEmpty()) {
		QImage errorimg(rti.width, rti.height, QImage::Format_RGB32);
		float min = 0.0f; //256.0f;
		float max = 25.0f;
		for(float &e: errors)
			e = sqrt(e/(nlights*3));

		for(uint32_t i = 0; i < rti.width*rti.height; i++)
			errorimg.setPixel(i%rti.width, i/rti.width, ramp(errors[i], min, max));

		errorimg.save(output);
	}
	return tot;
}

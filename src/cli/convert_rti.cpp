#include "../src/legacy_rti.h"
#include "rtibuilder.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QFile>
#include <QDir>


#include <vector>
#include <iostream>
using namespace std;

int convertRTI(const char *file, const char *output, int quality) {
	LRti lrti;
	if(!lrti.load(file)) {
		throw QString("Failed loading file %1: %2").arg(file).arg(lrti.error.c_str());
	}

	RtiBuilder rti;
	rti.width = lrti.width;
	rti.height = lrti.height;
	rti.chromasubsampling = lrti.chromasubsampled;
	switch(lrti.type) {
	case LRti::UNKNOWN:
		throw QString("Unknown RTI type!\n");
		return 1;
	case LRti::PTM_LRGB:
		rti.type = Rti::PTM;
		rti.colorspace = Rti::LRGB;
		rti.nplanes = 9;
		break;
	case LRti::PTM_RGB:
		rti.type = Rti::PTM;
		rti.colorspace = Rti::RGB;
		rti.nplanes = 18;
		break;
	case LRti::HSH_RGB:
		rti.type = Rti::HSH;
		rti.colorspace = Rti::RGB;
		rti.nplanes = lrti.scale.size()*3;
		break;
	}

	vector<int> order = { 5,3,4, 0,2,1};

	if(lrti.type == LRti::PTM_LRGB) {
		rti.scale = {1, 1, 1}; //rgb coeff in lptm have no scale or bias;
		rti.bias = {0, 0, 0};
		for(int i = 0; i < 6; i++) {
			rti.scale.push_back(lrti.scale[order[i]]);
			rti.bias.push_back(float(lrti.bias[order[i]]));
		}

	} else if(lrti.type == LRti::PTM_RGB){
		for(int i = 0; i < 6; i++) {
			for(int k = 0; k < 3; k++) {
				rti.scale.push_back(lrti.scale[order[i]]);
				rti.bias.push_back(lrti.bias[order[i]]);
			}
		}
	} else { //HSH
		rti.scale.resize(lrti.scale.size()*3);
		rti.bias.resize(lrti.scale.size()*3);
		for(size_t i = 0; i < lrti.scale.size(); i++) {
			rti.scale[i*3] = rti.scale[i*3+1] = rti.scale[i*3+2] = lrti.scale[i];
			rti.bias[i*3] = rti.bias[i*3+1] = rti.bias[i*3+2] = lrti.bias[i];

		}
	}

	QDir dir(output);
	if(!dir.exists()) {
		QDir here("./");
		if(!here.mkdir(output))
			throw QString("Could not create output dir!\n");
	}

	rti.saveJSON(dir, quality);
	for(uint32_t p = 0; p < rti.nplanes; p += 3) {
		lrti.encodeJPEGtoFile(p, quality, dir.filePath("plane_%1.jpg").arg(p/3).toStdString().c_str());
	}
	return 0;
}



//0 ok, 1 warning 2 error
int convertToRTI(const char *filename, const char *output, int quality, QString &msg) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly))
		throw QString("Failed opening: %1").arg(filename);

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();

	LRti lrti;
	lrti.width = obj["width"].toInt();
	lrti.height = obj["height"].toInt();
	QString type = obj["type"].toString();
	QString colorspace = obj["colorspace"].toString();
	//if the original data is low quality there is no reason to use a higher quality
	quality = std::min(quality, obj["quality"].toInt());
	uint nplanes = uint(obj["nplanes"].toInt());
	lrti.scale.resize(nplanes);
	lrti.bias.resize(nplanes);

	QJsonObject material = obj["materials"].toArray()[0].toObject();
	QJsonArray scale = material["scale"].toArray();
	QJsonArray bias = material["bias"].toArray();

	vector<uint> order;

	if(type == "ptm") {
		order = { 5,3,4, 0,2,1};
		lrti.scale.resize(6);
		lrti.bias.resize(6);
		if(colorspace == "rgb") {
			lrti.type = LRti::PTM_RGB;
			if(nplanes != 18)
				throw QString("Wrong number of planes (%1) was expecting 18.").arg(nplanes);

			for(uint i = 0; i < 6; i++) {
				lrti.scale[order[i]] = float(scale[i*3].toDouble());
				lrti.bias[order[i]] = float(bias[i*3].toDouble());
			}

		} else if(colorspace == "lrgb") {
			lrti.type = LRti::PTM_LRGB;
			if(nplanes != 9)
				throw QString("Wrong number of planes (%1) was expecting 9.").arg(nplanes);

			for(uint i = 0; i < 6; i++) {
				lrti.scale[order[i]] = float(scale[i+3].toDouble());
				lrti.bias[order[i]] = float(bias[i+3].toDouble());
			}

		} else {
			throw QString("Cannot convert PTM relight with colorspace: %1").arg(colorspace);
		}
	} else if(type == "hsh" && colorspace == "rgb") {
		order = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
		lrti.type = LRti::HSH_RGB;
		if(nplanes != 27 && nplanes != 12)
			throw QString("Wrong number of planes (%1) was expecting 12 or 27.").arg(nplanes);

		for(size_t i = 0; i < lrti.scale.size(); i++) {
			lrti.scale[i] = float(scale[int(i*3)].toDouble());
			lrti.bias[i]  = float(bias[int(i*3)].toDouble());
		}

	} else {
		throw QString("Cannot convert relight format: %1 and colorspace: %2").arg(type).arg(colorspace);
	}

	lrti.data.resize(nplanes);
	QFileInfo info(filename);
	QString path = info.dir().path();
	for(uint i = 0; i < nplanes/3; i++) {
		QString imagepath = path + QString("/plane_%1.jpg").arg(i);
		QFile image(imagepath);
		if(!image.open(QFile::ReadOnly))
			throw QString("Could not find or open file: %1").arg(imagepath);

		QByteArray buffer = image.readAll();
		bool ok = lrti.decodeJPEGfromFile(buffer.size(), (unsigned char *)buffer.data(), i*3, i*3+1, i*3+2);
		if(!ok)
			throw QString(lrti.error.c_str());
	}


	LRti::Encoding encoding = LRti::JPEG;
	//RTIViewer does not support RGB PTM  in JPEG format.
	if(type == "ptm" && colorspace == "rgb")
		encoding = LRti::RAW;

	//supports only RAW!
	QString out(output);
	if(out.endsWith("rti")) {
		lrti.encodeUniversal(output, quality);
		if(lrti.type == LRti::PTM_LRGB || lrti.type == LRti::PTM_RGB) {
			msg = "RTIViewer wont read universal rti (.rti) with PTM basis, use .ptm";
			return 1;
		}

	} else if(out.endsWith("ptm")) {
		lrti.encode(encoding, output, quality);
		if((lrti.width%8) != 8) {
			msg = "RTIVIewer does not support compressed .ptm when width is not a multiple of 8.\n"
					"The image will be slightly cropped.";
			return 1;
		}
	}

	return 0;
}

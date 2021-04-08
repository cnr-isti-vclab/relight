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
	if(!lrti.load(file))
		return 1;

	RtiBuilder rti;
	rti.width = lrti.width;
	rti.height = lrti.height;
	rti.chromasubsampling = lrti.chromasubsampled;
	switch(lrti.type) {
	case LRti::UNKNOWN:
		cerr << "Unknown RTI type!\n" << endl;
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
	case LRti::HSH:
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
	} else {
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
		if(!here.mkdir(output)) {
			cerr << "Could not create output dir!\n" << endl;
			return 1;
		}
	}

	rti.saveJSON(dir, quality);
	for(uint32_t p = 0; p < rti.nplanes; p += 3) {
		lrti.encodeJPEGtoFile(p, quality, dir.filePath("plane_%1.jpg").arg(p/3).toStdString().c_str());
	}
	return 0;
}



int convertToRTI(const char *filename, const char *output) {
	QFile file(filename);
	if(!file.open(QFile::ReadOnly)) {
		cerr << "Failed opening: " << filename << endl;
		return 1;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();

	LRti lrti;
	lrti.width = obj["width"].toInt();
	lrti.height = obj["height"].toInt();
	QString type = obj["type"].toString();
	QString colorspace = obj["colorspace"].toString();
	int quality = obj["quality"].toInt();
	uint nplanes = uint(obj["nplanes"].toInt());
	lrti.scale.resize(nplanes);
	lrti.bias.resize(nplanes);

	QJsonObject material = obj["materials"].toArray()[0].toObject();
	QJsonArray scale = material["scale"].toArray();
	QJsonArray bias = material["bias"].toArray();

	vector<uint> order = { 5,3,4, 0,2,1};

	if(type == "ptm") {
		if(colorspace == "rgb") {
			lrti.type = LRti::PTM_RGB;
			if(nplanes != 18) {
				cerr << "Wrong number of planes (" << nplanes << ") was expecting 18." << endl;
				return 1;
			}
			int count = 0;
			lrti.scale.resize(6);
			lrti.bias.resize(6);
			for(uint i = 0; i < 6; i++) {
				lrti.scale[order[i]] = float(scale[count*3].toDouble());
				lrti.bias[order[i]] = float(bias[count*3].toDouble());
				count++;
			}

		} else if(colorspace == "lrgb") {
			lrti.type = LRti::PTM_LRGB;
			if(nplanes != 9) {
				cerr << "Wrong number of planes (" << nplanes << ") was expecting 9." << endl;
				return 1;
			}
			int count = 3;
			for(uint i = 0; i < 6; i++) {
				lrti.scale[order[i]] = float(scale[count].toDouble());
				lrti.bias[order[i]] = float(bias[count].toDouble());
				count++;
			}
			lrti.scale.resize(6);
			lrti.bias.resize(6);
		} else {
			cerr << "Cannot convert PTM relight with colorspace: " << qPrintable(colorspace) << endl;
			return 1;
		}
	} else if(type == "hsh" && colorspace == "rgb") {
		lrti.type = LRti::HSH;
		if(nplanes != 27 && nplanes != 12) {
			cerr << "Wrong number of planes (" << nplanes << ") was expecting 12 or 27." << endl;
			return 1;
		}

		for(size_t i = 0; i < lrti.scale.size(); i++) {
			lrti.scale[i] = float(scale[int(i*3)].toDouble());
			lrti.bias[i]  = float(bias[int(i*3)].toDouble());
		}

	} else {
		cerr << "Cannot convert relight format: " << qPrintable(type) << " and colorspace: " << qPrintable(colorspace) << endl;
		return 1;
	}

	lrti.data.resize(nplanes);
	QFileInfo info(filename);
	QString path = info.dir().path();
	for(uint i = 0; i < nplanes/3; i++) {
		QString imagepath = path + QString("/plane_%1.jpg").arg(i);
		QFile image(imagepath);
		if(!image.open(QFile::ReadOnly)) {
			cerr << "Could not find or open file: " << qPrintable(imagepath) << endl;
			return 1;
		}
		QByteArray buffer = image.readAll();
		lrti.decodeJPEGfromFile(buffer.size(), (unsigned char *)buffer.data(), i*3, i*3+1, i*3+2);
	}


	LRti::PTMFormat ptmformat = LRti::JPEG;
	//RTIViewer does not support RGB PTM  in JPEG format.
	if(type == "ptm" && colorspace == "rgb")
		ptmformat = LRti::RAW;
	lrti.encode(ptmformat, output, quality);

	return 0;
}

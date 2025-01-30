#include <math.h>

#include <iostream>
#include <vector>

#include <QImage>
#include <QPoint>
#include <QDir>
//#include <QGuiApplication>

//#include "aligndialog.h"

using namespace std;

/* Align: fine alignmend of accidentally slightly misaligned images during capture.
 *
 * max number of pixl in input
 * mode: initially only translate is supported, later probably add rotate
 * test run (wont convert the images, just return translations)
 * Final images are cropped to the common portion
 * Result should be inspectable in an interface.
 *
 * Mutual information is used for alignment on a few selected region.
 * can be specified, or auto (defaults).
 */

//a is the x axis (col) b is the y axs (the col)
double mutualInformation(QImage a, QImage b, int max, int dx, int dy) {
	std::vector<int> histo(256*256, 0);
	std::vector<int> aprob(256, 0);
	std::vector<int> bprob(256, 0);
	int width = a.width();
	int height = a.height();
	//x and y refer to the image pixels!
	for(int y = max; y < height - max; y++) {
		for(int x = max; x < width - max; x++) {
			QRgb ca = qGray(a.pixel(x, y));
			QRgb cb = qGray(b.pixel(x + dx, y + dy));
			histo[ca + 256*cb]++;
			aprob[ca]++;
			bprob[cb]++;
		}
	}
	double tot = (height - 2*max)*(width - 2*max);
	double info = 0.0;

	for(int y = 0; y < 256; y++) {
		for(int x = 0; x < 256; x++) {
			double p = histo[x + 256*y]/tot;
			if(p == 0) continue;
			double pa = aprob[x]/tot;
			double pb = bprob[y]/tot;
			info += p * log(p/(pa*pb));
		}
	}
	return info;
}


//a and b image must be decently larger than max min is 3 timess
//bw images
QPoint align(QImage a, QImage b, int max, double &best_info, double &initial) {

	best_info = 0.0;
	QPoint best(0, 0);
	for(int dy = -max; dy <= max; dy++) {
		for(int dx = -max; dx <= max; dx++) {
			double info = mutualInformation(a, b, max, dx, dy);
			if(dx == 0 && dy == 0) {
				initial = info;
			}
			//cout << info << " ";
			if(info > best_info) {
				best_info = info;
				best.rx() = dx;
				best.ry() = dy;
			}
		}
		//cout << endl;
	}
	return best;
}

#include "../src/getopt.h"
#include <QImage>

int main(int argc, char *argv[]) {

	if(argc == 1) {
/*		QGuiApplication app(argc, argv);
		auto dialog = new AlignDialog;
		dialog->show();
		int res = dialog->exec();
		return res; */
	}

	//dir max_offset crop
	if(argc != 4) {
		cerr << "Usage: " << argv[0] << " directory max_offset top:left:width:height" << endl;
		return 0;
	}

	QDir dir = QDir(argv[1]);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(argv[1]) << " folder.\n";
		return -1;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG";;
	QStringList images = dir.entryList(img_ext);
	if(!images.size()) {
		cerr << "No images to align!" << endl;
		return -1;
	}
	QImage first(images[0]);
	if(first.isNull()) {
		cerr << "Could not load image: " << qPrintable(images[0]) << endl;
		return -1;
	}

	int max_offset = QString(argv[2]).toInt();
	QStringList c = QString(argv[3]).split(":");
	QRect crop(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt());
	crop.adjust(-max_offset, -max_offset, max_offset, max_offset);

	if(!QRect(0, 0, first.width(), first.height()).contains(crop)) {
		cerr << "Alignment sample (+offset) is not contained in the image" << endl;
		return -1;
	}

	vector<QImage> samples;
	for(int i = 0; i < images.size(); i++) {
		cout << qPrintable(images[i]) << endl;
		QImage img(images[i]);
		QImage sub = img.copy(crop);
		samples.push_back(sub);
	}

	int reference = 0;
	vector<QPoint> offsets;
	for(int i = 0; i < samples.size(); i++) {
		double best = 0.0f;
		double initial = 0.0f;
		QPoint p = align(samples[reference], samples[i], max_offset, best, initial);
		cout << p.x() << " " << p.y() << ": " << best << endl;
		offsets.push_back(p);
	}

	/*
	QDir dir("./");
	dir.setNameFilters(QStringList()<<"*.jpg"<<"*.JPG");
	QStringList images = dir.entryList();

	int reference = 0;
	int max = 20;
	int width = 190; //40;
	int height = 190; //40;
	int offx = 3017;//51; //100;
	int offy = 3837;//157; //100;
	vector<QImage> samples;
	for(int i = 0; i < images.size(); i++) {
		cout << qPrintable(images[i]) << endl;
		QImage img(images[i]);
		QImage sub = img.copy(offx, offy, width, height);
		samples.push_back(sub);
	}

	vector<QPoint> offsets;
	for(int i = 0; i < samples.size(); i++) {
		if(i == reference) {
			offsets.push_back(QPoint(0, 0));
			continue;
		}
		QPoint p = align(samples[reference], samples[i], max);
		cout << p.x() << " " << p.y() << endl << endl;
		offsets.push_back(p);
	}
	dir.mkdir("aligned");
	for(int i = 0; i < images.size(); i++) {
		QPoint &o = offsets[i];
	//	QImage img(images[i]);
		QImage img = samples[i];
		QImage tmp = img.copy(max + o.x(), max + o.y(), img.width() - 2*max, img.height() - 2*max);
		tmp.save("aligned/A_" + images[i]);
	} */

	return 0;
}


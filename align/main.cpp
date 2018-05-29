#include <math.h>

#include <iostream>
#include <vector>

#include <QImage>
#include <QPoint>
#include <QDir>
#include <QApplication>

#include "aligndialog.h"

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
QPoint align(QImage a, QImage b, int max) {

	double best_info = 0.0;
	QPoint best(0, 0);
	for(int dy = -max; dy <= max; dy++) {
		for(int dx = -max; dx <= max; dx++) {
			double info = mutualInformation(a, b, max, dx, dy);
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

int main(int argc, char *argv[]) {

	const auto &app = QApplication(argc, argv);

	auto dialog = new AlignDialog;
	dialog->show();
	int res = dialog->exec();


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


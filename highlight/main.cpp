#include <QGuiApplication>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QPointF>
#include <QTextStream>

#include "mainwindow.h"
#include <iostream>
#include <vector>
using namespace std;



/* usage: dome_calibration <ball> [<color_table>] */


void calibrateColors(QString dir) {
	QDir colors_dir(dir);
	if(!colors_dir.exists()) {
		cerr << "Could not find " << qPrintable(dir) << " folder.\n";
		exit(0);
	}

	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	QStringList colors = colors_dir.entryList(img_ext);
	if(!colors.size()) {
		cerr << "Could not find images in directory: " << qPrintable(dir) << endl;
		exit(0);
	}
}


int main(int argc, char *argv[]) {
	cout << "!) Cancel when saving!" << endl;
	cout << "Do notlook for highlight if no sphere is selected!" << endl;
	cout << "Do not export if no sphere selected" << endl;
	cout << "autodetext sphere.lp in dir and ask for loading" << endl;
	cout << "Status bar: show number of images, width, height, position of mouse," << endl;
	const auto &app = QApplication(argc, argv);

	auto mainwindow = new MainWindow();
	mainwindow->showMaximized();
	app.exec();
	return 0;
}


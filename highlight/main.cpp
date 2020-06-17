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
	const auto &app = QApplication(argc, argv);

	auto mainwindow = new MainWindow();
	mainwindow->show();
	app.exec();
	return 0;
}


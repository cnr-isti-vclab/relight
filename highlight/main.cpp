#include <QGuiApplication>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QPointF>
#include <QTextStream>

#include "ballpickerdialog.h"
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
	if(argc < 2 || argc > 3) {
		cerr << "Usage: dome_calibration <balls folder> [<color table folder>]\n\n";
		return 0;
	}

	const auto &app = QApplication(argc, argv);


	auto dialog = new BallPickerDialog;
	//bool ok = dialog->init(argv[1]);
	//if(!ok) return -1;

	dialog->show();
	int res = dialog->exec();
	vector<QPointF> lights =  dialog->lights;

	//if(argc == 3)
	//	calibrateColors(argv[2]);

	//app.exec();

	return 0;
}


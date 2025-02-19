#include <QApplication>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QImageReader>

#include "relightapp.h"
#include "mainwindow.h"

#include <locale.h>

#include <iostream>
using namespace std;

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

Project project;

int main(int argc, char *argv[]) {


	RelightApp app(argc, argv);

	QApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
	setlocale(LC_ALL, "en_US.UTF8"); //needs to be called AFTER QApplication creation.

#if QT_VERSION >= 0x060000
	//large images wont load because of this limit!
	QImageReader::setAllocationLimit(0);
#endif

	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("RelightLab");
	QCoreApplication::setApplicationVersion(RELIGHT_STRINGIFY(RELIGHT_VERSION));

	app.run();

	return app.exec();
}


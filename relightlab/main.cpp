#include <QApplication>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include "relightapp.h"
#include "mainwindow.h"

#include <locale.h>

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

Project project;

int main(int argc, char *argv[]) {

	RelightApp app(argc, argv);

	QApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
	setlocale(LC_ALL, "en_US.UTF8"); //needs to be called AFTER QApplication creation.

#if QT_VERSION >= 0x060000
	//large images wont load because of this limit!
	QImage::setAllocationLimit(32768*32768*4);
#endif


	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("RelightLab");
	QCoreApplication::setApplicationVersion(RELIGHT_STRINGIFY(RELIGHT_VERSION));

	app.run();

	return app.exec();
}


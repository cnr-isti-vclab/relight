#include <QApplication>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include "relightapp.h"
#include "mainwindow.h"

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

Project project;

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, ".UTF8");

	RelightApp app(argc, argv);

	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("RelightLab");
	QCoreApplication::setApplicationVersion(RELIGHT_STRINGIFY(RELIGHT_VERSION));

	app.run();

	return app.exec();
}


#include "mainwindow.h"

#include <QApplication>
#include <QImageReader>
#include <locale.h>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	QApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
	setlocale(LC_ALL, "en_US.UTF8");

#if QT_VERSION >= 0x060000
	QImageReader::setAllocationLimit(0);
#endif

	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("BrdfLab");

    MainWindow window;
	window.show();
	return app.exec();
}

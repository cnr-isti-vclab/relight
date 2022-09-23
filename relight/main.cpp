#include <QGuiApplication>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QPointF>
#include <QTextStream>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QPainter>
#include <QSettings>
#include <QTemporaryDir>

#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include "mainwindow.h"
#include "processqueue.h"
#include <iostream>
#include <vector>
using namespace std;

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

/* usage: dome_calibration <sphere> [<color_table>] */


void calibrateColors(QString dir) {
	QDir colors_dir(dir);
	if(!colors_dir.exists()) {
		cerr << "Could not find " << qPrintable(dir) << " folder.\n";
		exit(0);
	}

	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.nef" << "*.NEF" << "*.CR2";
	QStringList colors = colors_dir.entryList(img_ext);
	if(!colors.size()) {
		cerr << "Could not find images in directory: " << qPrintable(dir) << endl;
		exit(0);
	}
}


class MyProxyStyle : public QProxyStyle {
public:
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap & pixmap, const QStyleOption * option) const {
		if(iconMode == QIcon::Disabled) {
			QPixmap r = pixmap;
			QPainter painter(&r);
			painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
			painter.setBrush(QColor(0, 0, 0, 110));
			painter.drawRect(QRect(QPoint(0, 0), r.size()));
			return r;
		}
		return QProxyStyle::generatedIconPixmap(iconMode, pixmap, option);
	}

};

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("RelightLab");
	QCoreApplication::setApplicationVersion(RELIGHT_STRINGIFY(RELIGHT_VERSION));

	if (argc > 1) {
		if (std::string(argv[1]) == std::string("--version") ||
			std::string(argv[1]) == std::string("-v")) {
			std::cout << "ReLightLab " << RELIGHT_STRINGIFY(RELIGHT_VERSION) << std::endl;
			return 0;
		}
	}

	//	AutoStyle autostyle;
	//	app.connect(&autostyle, SIGNAL(resetStyle(QString)), SLOT(setStyleSheet(QString)));
	QFile style(":/darkorange/stylesheet.txt");
	style.open(QFile::ReadOnly);
	app.setStyleSheet(style.readAll());
	app.setStyle(new MyProxyStyle());

	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();
	
	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		qDebug() << "Could not create a temporary file for the scripts. Use File->Preferences";
	}
	QDir scripts(tmp.path());
	QSettings().setValue("tmp_scripts_path", tmp.path());
	scripts.mkdir("normals");
	QStringList files;
	files << "deepzoom.py" << "tarzoom.py" << "itarzoom.py" << "rotate_rti.py"
		<< "normals/normalmap.py" << "normals/psutil.py" << "normals/rpsnumerics.py" << "normals/rps.py";

	for(QString file: files)
		QFile::copy(":/scripts/" + file, tmp.path() + "/" + file);

	auto mainwindow = new MainWindow();
	mainwindow->showMaximized();
	app.exec();

	return 0;
}


#include <QGuiApplication>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QPointF>
#include <QTextStream>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QPainter>

#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include "mainwindow.h"
#include "processqueue.h"
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
	QCoreApplication::setApplicationName("Relight");


	//	AutoStyle autostyle;
	//	app.connect(&autostyle, SIGNAL(resetStyle(QString)), SLOT(setStyleSheet(QString)));
	QFile style(":/darkorange/stylesheet.txt");
	style.open(QFile::ReadOnly);
	app.setStyleSheet(style.readAll());

	app.setStyle(new MyProxyStyle());
/*	qDebug() << QStyleFactory::keys();
	//app.setStyle("Fusion");
/()	QPalette palette = QPalette();
	palette.setColor(QPalette::Window, QColor(53, 53, 53));
	palette.setColor(QPalette::WindowText, Qt::white);
	palette.setColor(QPalette::Base, QColor(25, 25, 25));
	palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
	palette.setColor(QPalette::ToolTipBase, Qt::black);
	palette.setColor(QPalette::ToolTipText, Qt::white);
	palette.setColor(QPalette::Text, Qt::white);
	palette.setColor(QPalette::Button, QColor(53, 53, 53));
	palette.setColor(QPalette::ButtonText, Qt::white);
	palette.setColor(QPalette::BrightText, Qt::red);
	palette.setColor(QPalette::Link, QColor(42, 130, 218));
	palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	palette.setColor(QPalette::HighlightedText, Qt::black);
	app.setPalette(palette); */

	//palette.setColor(QPalette::Disabled, QPalette::, QColor(127, 127, 127));
	//palette.setColor(QPalette::Disabled, QPalette::Color, QColor(127, 127, 127));
	ProcessQueue &queue = ProcessQueue::instance();
	queue.start();

	auto mainwindow = new MainWindow();
	mainwindow->showMaximized();
	app.exec();


	return 0;
}


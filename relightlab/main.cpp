#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QMessageBox>
#include "mainwindow.h"
#include "../src/project.h"

#define RELIGHT_STRINGIFY0(v) #v
#define RELIGHT_STRINGIFY(v) RELIGHT_STRINGIFY0(v)

Project project;

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, ".UTF8");

	QApplication app(argc, argv);

	QCoreApplication::setOrganizationName("VCG");
	QCoreApplication::setOrganizationDomain("vcg.isti.cnr.it");
	QCoreApplication::setApplicationName("RelightLab");
	QCoreApplication::setApplicationVersion(RELIGHT_STRINGIFY(RELIGHT_VERSION));

	QFile style(":/css/style.txt");
	style.open(QFile::ReadOnly);
	app.setStyleSheet(style.readAll());



	MainWindow w;

	w.showMaximized();

	QTemporaryDir tmp;
	if(!tmp.isValid()) {
		QMessageBox::critical(&w, "Temporary folder is needed", "Could not create a temporary file for the scripts. Use File->Preferences");
	}

/*
 * 	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	 QGuiApplication app(argc, argv);
	  QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (engine.rootObjects().isEmpty())
		return -1; */
	return app.exec();
}


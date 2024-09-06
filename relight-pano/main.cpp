#include "mainwindow.h"
#include <QDir>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include "panobuilder.h"
using namespace std;

void help(){
	cout << R"(relight-pano

usage: relight-pano <folder> [-i]

	folder: folder containing a set of rti photo datasets
	-i: open user interface
)";
}
int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationName("relight-pano");


	QCommandLineParser parser;
	parser.setApplicationDescription("relight-pano: builds an RTI panorama");
	parser.addHelpOption();
	//folder
	parser.addPositionalArgument("folder", "path folder containing rti datasets");

	QCommandLineOption interactiveOption(QStringList() << "i" << "interactive",
										 "open users interface");
	parser.addOption(interactiveOption);

	QCommandLineOption stepOption(QStringList() << "s" << "step",
										 "starting step (rti, tapioca, )", "rti");
	parser.addOption(stepOption);

	// Process the actual command line arguments given by the user
	parser.process(app);

	const QStringList args = parser.positionalArguments();
	if (args.size() < 1) {
		cerr << "Missing command line Rti folder parameter." << endl;
		return -1;
	}

	QString folder = args[0];
	QDir dir(folder);
	if (!dir.exists()) {
		cerr << "Error: folder '"<< qPrintable(folder) << "' does not exist" << endl;
		return -1;
	}

	bool interactive = parser.isSet(interactiveOption);
	PanoBuilder::Steps startingStep = PanoBuilder::RTI;
	bool steps = parser.isSet(stepOption);
	if (interactive && steps) {
		cout << "run in interactive mode" << endl;
		MainWindow w;
		w.show();
	} else {
		try{
			PanoBuilder builder(folder);
			if(steps){
				QString step_name = parser.value(stepOption);
				int s = builder.findStep(step_name);
				if(s==-1){
					cerr << "Unknown step: " << qPrintable(step_name) << endl;
					return -1;
				}
				startingStep = (PanoBuilder::Steps) s;
			}
			builder.setMm3d("/Users/erika/Desktop/micmac/bin/mm3d");
			builder.process(startingStep);
		}
		catch(QString error){
			cerr << qPrintable(error) << endl;
		}
		return 0;
	}


	return app.exec();
}

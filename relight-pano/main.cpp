#include "mainwindow.h"
#include <QDir>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include <QtXml>
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

	QCommandLineOption verboseOption(QStringList() << "v" << "verbose", "Enable verbose output");
	parser.addOption(verboseOption);

	QCommandLineOption debugOption(QStringList() << "d" << "debug", "Enable debug output");
	parser.addOption(debugOption);

	//Using DefCor for malt_mec function, which indicates the level of correlation between pixels
	//higher values ​​of DefCor force a stronger correlation between adjacent pixels, which might improve spatial coherence in reconstructed images, but reduce detail.
	//Too high a value could cause excessive smoothness between pixels.
	QCommandLineOption defCorOption(QStringList() << "c" << "DefCor", "Specifies the correlation between pixels (DefCor parameter)",
									"Higher values increase pixel correlation. Default is 0.2", "0.2");
	parser.addOption(defCorOption);
	//A higher value for Regul might impose greater smoothing or stability,
	//while lower values ​​would allow for more variability and detail, but might result in greater sensitivity to noise.
	QCommandLineOption regulOption(QStringList() << "r" << "Regul", "Sets the regularization parameter (Regul)",
													"Regul check the amount of smoothing applied during the orthorectification process. Default is 0.05", "0.05");
	parser.addOption(regulOption);

	QCommandLineOption light3dOption(QStringList() << "3" << "Light3d", "Sets the regularization parameter ", "help", "");
	parser.addOption(light3dOption);

	QCommandLineOption stepOption(QStringList() << "s" << "step",
										 "starting step (rti, tapioca, schnaps, tapas, apericloud, orthoplane, tarama, malt_mec,"
										 "c3dc, depthmap, malt_ortho, jpg)", "rti");
	parser.addOption(stepOption);


	QCommandLineOption stopOption(QStringList() << "S" << "stop",
								  "stop after first step");
	parser.addOption(stopOption);


	// Process the actual command line arguments given by the user
	parser.process(app);

	const QStringList args = parser.positionalArguments();
	if (args.size() < 1) {
		cerr << "Missing command line Rti folder parameter." << endl;
		return -1;
	}

	QString datasetsFolder = args[0];
	QDir dir(datasetsFolder);
	if (!dir.exists()) {
		cerr << "Error: folder '"<< qPrintable(datasetsFolder) << "' does not exist" << endl;
		return -1;
	}

	double defCor = parser.value(defCorOption).toDouble();
	double regul = parser.value(regulOption).toDouble();
	if (defCor < 0) {
		cerr << "Error: DefCor must be between 0 and 1. Value provided: " << defCor << endl;
		return -1;
	}
	if(regul < 0) {
		cerr << "Error: Regul must be between 0 and 1. Value provided: " << regul << endl;
		return -1;
	}
	cout << "DefCor value: " << defCor << endl;
	cout << "Regul value: " << regul << endl;

	QString light3d = parser.value(light3dOption);

	bool interactive = parser.isSet(interactiveOption);
	bool stop = parser.isSet(stopOption);
	bool verbose = parser.isSet(verboseOption);
	bool debug = parser.isSet(debugOption);

	PanoBuilder::Steps startingStep = PanoBuilder::RTI;
	bool steps = parser.isSet(stepOption);
	if (interactive && steps) {
		cout << "Run in interactive mode" << endl;
		MainWindow w;
		w.show();
		return app.exec();
	} else {
		try{
			PanoBuilder builder(datasetsFolder);
			builder.DefCor= defCor;
			builder.Regul = regul;
			builder.light3d = light3d;
			builder.verbose = verbose;
			builder.debug = debug;

			if(steps){
				QString step_name = parser.value(stepOption);
				int s = builder.findStep(step_name);
				if(s==-1){
					cerr << "Unknown step: " << qPrintable(step_name) << endl;
					return -1;
				}
				startingStep = (PanoBuilder::Steps) s;
			}
			// builder.setMm3d("/home/ponchio/devel/micmac/bin/mm3d");
			//builder.setMm3d("/Users/erika/Desktop/micmac/bin/mm3d");
			builder.setMm3d("/home/erika/micmac/bin/mm3d");
			//builder.setRelightCli("/home/ponchio/devel/relight/relight-cli/relight-cli");
			//builder.setRelightCli("/Users/erika/Desktop/projects/relight/relight-cli/relight-cli");
			builder.setRelightCli("/home/erika/relight/relight-cli/relight-cli");
			//builder.setRelightMerge("/home/ponchio/devel/relight/relight-merge/relight-merge");
			//builder.setRelightMerge("/Users/erika/Desktop/projects/relight/build/relight-merge/relight-merge");
			builder.setRelightMerge("/home/erika/relight/relight-merge/relight-merge");

			builder.process(startingStep, stop);
		}
		catch(QString error){
			cerr << qPrintable(error) << endl;
		}

	}
	return 0;
}

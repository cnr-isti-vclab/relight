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
	parser.addPositionalArgument("folder", "path folder containing rti datasets");

	QCommandLineOption micmacOption(QStringList()      << "M" << "micmac",          "Micmac mm3d path",     "micbin", "/home/erika/micmac/bin/mm3d");
	QCommandLineOption cliOption(QStringList()         << "C" << "relight-cli",     "Relight-cli path",     "clibin", "/home/erika/relight/relight-cli/relight-cli");
	QCommandLineOption mergeOption(QStringList()       << "G" << "relight-merge",   "Relight-merge path",   "mergebin", "/home/erika/relight/relight-merge/relight-merge");
	QCommandLineOption normalsOption(QStringList()     << "N" << "relight-normals", "Relight-normals path", "normalsbin", "/home/erika/relight/relight-normals/relight-normals");

	QCommandLineOption interactiveOption(QStringList() << "i" << "interactive",  "Open users interface");
	QCommandLineOption verboseOption(QStringList()     << "v" << "verbose",      "Enable verbose output");
	QCommandLineOption debugOption(QStringList()       << "d" << "debug",        "Enable debug output");
	QCommandLineOption stepOption(QStringList()        << "s" << "step",
								  "Starting step (means, tapioca, schnaps, tapas, apericloud, orthoplane, tarama, malt_mec, c3dc, rti, depthmap, malt_ortho, jpg)", "step");
	QCommandLineOption stopOption(QStringList()        << "S" << "stop",         "Stop after first step");

	parser.addOption(micmacOption);
	parser.addOption(cliOption);
	parser.addOption(mergeOption);
	parser.addOption(normalsOption);

	parser.addOption(interactiveOption);
	parser.addOption(verboseOption);
	parser.addOption(debugOption);
	parser.addOption(stepOption);
	parser.addOption(stopOption);


	//Using DefCor for malt_mec function, which indicates the level of correlation between pixels
	//higher values ​​of DefCor force a stronger correlation between adjacent pixels, which might improve spatial coherence in reconstructed images, but reduce detail.
	//Too high a value could cause excessive smoothness between pixels.
	QCommandLineOption defCorOption(QStringList()      << "c" << "DefCor", "Specifies the correlation between pixels (DefCor parameter)",
									"Higher values increase pixel correlation. Default is 0.2", "0.2");
	parser.addOption(defCorOption);
	//A higher value for Regul might impose greater smoothing or stability,
	//while lower values ​​would allow for more variability and detail, but might result in greater sensitivity to noise.
	QCommandLineOption regulOption(QStringList()       << "r" << "Regul", "Sets the regularization parameter (Regul)",
													"Regul check the amount of smoothing applied during the orthorectification process. Default is 0.05", "0.05");
	parser.addOption(regulOption);

	QCommandLineOption light3dOption(QStringList()     << "3" << "Light3d", "Sets the regularization parameter ", "help", "");
	parser.addOption(light3dOption);

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
	bool stop        = parser.isSet(stopOption);
	bool verbose     = parser.isSet(verboseOption);
	bool debug       = parser.isSet(debugOption);



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

			builder.setMm3d(parser.value(micmacOption));
			builder.setRelightCli(parser.value(cliOption));
			builder.setRelightMerge(parser.value(mergeOption));

			//builder.setMm3d("/home/erika/micmac/bin/mm3d");
			// builder.setRelightCli("/home/erika/relight/relight-cli/relight-cli");
			// builder.setRelightMerge("/home/erika/relight/relight-merge/relight-merge");

			PanoBuilder::Steps  startingStep = PanoBuilder::MEANS;
			if(steps) {
				QString step_name = parser.value(stepOption);
				int s = builder.findStep(step_name);
				if(s == -1){
					cerr << "Unknown step: " << qPrintable(step_name) << endl;
					return -1;
				}
				startingStep = (PanoBuilder::Steps) s;
			}

			builder.process(startingStep, stop);
		}
		catch(QString error){
			cerr << qPrintable(error) << endl;
		}

	}
	return 0;
}

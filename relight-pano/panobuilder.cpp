#include "panobuilder.h"
#include <assert.h>
#include <QDir>
#include <iostream>
#include <QProcess>
#include <QFileInfo>
#include <QImage>
#include "exiftransplant.h"
using namespace std;

PanoBuilder::PanoBuilder(QString dataset_path)
{
	datasets_dir = QDir(QDir(dataset_path).absolutePath());
	assert(datasets_dir.exists());
	base_dir = datasets_dir;
	base_dir.cdUp();
	QDir::setCurrent(base_dir.absolutePath());
}
void PanoBuilder::setMm3d(QString path){
	ensureExecutable(path);
	mm3d_path = path;
}
void PanoBuilder::setRelightCli(QString path){
	ensureExecutable(path);

	relight_cli_path = path;
}
void PanoBuilder::setRelightMerge(QString path){
	ensureExecutable(path);
	relight_merge_path = path;
}

void PanoBuilder::ensureExecutable(QString path){
	QFileInfo info(path);
	if(!info.exists())
		throw QString("File do not exists: ") + path;
	if(!info.isExecutable() || !info.isFile())
		throw QString("File is not executable") + path;

}
void PanoBuilder::cd(QString path, bool create){
	QDir::setCurrent(base_dir.absolutePath());
	QDir dir(path);
	if(!dir.exists()){
		if(create){
			if (!base_dir.mkdir(path)) {
				throw QString("Could not create directory: ") + path;
			}
		}
		else
			throw QString("Directory do not exist:") + path;
	}
	QDir::setCurrent(dir.absolutePath());
}
int PanoBuilder::findStep(QString step){
	return (steps.indexOf(step));
}

/* directory structures:
 * datasets
 *		face_A
 *		face_B
 * rti
 *		face_A
 *		face_B
 * photogrammetry
 *		Malt
 *		Ori-Abs
 *		Ori-Rel
 *		...
 *
 */
void PanoBuilder::process(Steps starting_step){
	switch (starting_step) {
	case RTI: rti();
	case TAPIOCA: tapioca();
	case SCHNAPS: schnaps();
	case APERICLOUD: apericloud();
	case ORTHOPLANE: orthoplane();
	case TARAMA: tarama();
	case MALT_MEC: malt_mec();
	case C3DC: c3dc();
	case MALT_ORTHO: malt_ortho();
	case TAWNY: tawny();
	case JPG: jpg();

	}
}
//crea la directory rti se non esiste
//per ogni sottodir cerca file .relight e si passa nella command line di relight-cli
//command line specifica di fare le medie -m
// spostare le mean dentro fotogrammetry rinomina da rti/face_A/mean.jpg a photogrammetry/face_A.jpg
// chiama il merge, che ti chiede un'altra directory "merge", cancella la cartella rti e rinomina "merge" come "rti"
// output stampato a schermo e controlla gli errori



void PanoBuilder::rti(){
	QDir rtiDir(base_dir.filePath("rti"));
	if (!rtiDir.exists()) {
		if (!base_dir.mkdir("rti")) {
			throw QString("Could not create 'rti' directory");

		}
	}

	QDir mergeDir(base_dir.filePath("merge"));
	if (!mergeDir.exists()) {
		if (!base_dir.mkdir("merge")) {
			throw QString("Could not create 'merge' directory");
		}
	}

	//search the subdirectory, the QDir::Dirs | filter QDir::NoDotAndDotDot to get all subdirectories inside the root directory
	QStringList subDirs = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &subDirName : subDirs) {
		QDir subDir(datasets_dir.filePath(subDirName));

	//search file .relight
		QStringList relightFiles = subDir.entryList(QStringList() << "*.relight", QDir::Files);
		if(relightFiles.size()==0)
			throw QString("Missing .relight file in folder " )+ subDir.path();
		QString relightFile = relightFiles[0];


		QStringList arguments;
		arguments << subDir.absoluteFilePath(relightFile) << rtiDir.filePath(subDir.dirName()) <<"-b" << "ptm" << "-p" << "18" << "-m";

		QString command = relight_cli_path + " " + arguments.join(" ");
		cout << "print command: " << qPrintable(command) <<endl;
		QProcess process_cli;
		process_cli.start(relight_cli_path, arguments);

		if(!process_cli.waitForStarted()){
			throw QString("fail to start ") + process_cli.program();
		}

		//wait for the process to finish
		if(!process_cli.waitForFinished(-1)) {
			throw QString("fail to run ") + process_cli.readAllStandardError();
		}


		QStringList arguments_merge;
		arguments_merge << "rti" << "merge";
		QProcess process_merge;
		process_merge.start(relight_merge_path, arguments_merge);

		if(!process_merge.waitForStarted()){
			throw QString("fail to start ") + process_merge.program();
		}

		//wait for the process to finish
		if(!process_merge.waitForFinished(-1)) {
			throw QString("fail to run ") + process_merge.readAllStandardError();
		}

	}
}
void PanoBuilder::tapioca(){
	cd("photogrammetry", true);

	cout << qPrintable(base_dir.absolutePath()) << endl;
	cout << qPrintable(base_dir.absoluteFilePath("rti")) << endl;
	QDir rtiDir(base_dir.absoluteFilePath("rti"));
	if (!rtiDir.exists()) {
		throw QString("rti directory does not exist: ") + rtiDir.absolutePath();
	}

	QStringList subDirs = rtiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &subDirName : subDirs) {
		QDir subDir(rtiDir.filePath(subDirName));
		QStringList meanFiles = subDir.entryList(QStringList() << "means.png", QDir::Files);
		if (meanFiles.size() == 0)
			throw QString("Missing 'means.png' not found in ") + subDir.path();
		QString meanFile = meanFiles[0];

		QImage img;
		img.load(subDir.filePath("means.png"));
		img.save(subDir.dirName()+ ".jpg");

		cout << qPrintable(datasets_dir.absolutePath()) << endl;
		QDir dataset(datasets_dir.absoluteFilePath(subDirName));

		QStringList photos = dataset.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
		if (photos.size() == 0)
			throw QString("Missing '*.jpg' not found in ") + subDir.path();
		QString photo = photos[0];

		ExifTransplant exif;
		bool success = exif.transplant(dataset.absoluteFilePath(photo).toStdString().c_str(),
									   (subDir.dirName() + ".jpg").toStdString().c_str());
		if(!success)
			throw QString("Unable to load exif from: ") + QString(exif.error.c_str()) + dataset.absoluteFilePath(photo);
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "tapioca" <<"All" << ".*jpg" << "1500" << "@SFS";

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	if (!process.waitForFinished(-1)) {
		throw QString("Failed to run ") + process.readAllStandardError();
	}
	cout << qPrintable(process.readAllStandardOutput()) << endl;
}

void PanoBuilder::schnaps(){
	//prende l'input dalla sottodir di homol e jpg
	cd("photogrammetry");

	QDir currentDir = QDir::current();
	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists()) {
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();

	}
	cout << qPrintable(homolDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Schnaps" << ".*jpg";

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	if (!process.waitForFinished(-1)) {
		throw QString("Failed to run ") + process.readAllStandardError();
	}
	cout << qPrintable(process.readAllStandardOutput()) << endl;
}

void PanoBuilder::tapas(){
	//prende l'input dalla sottodirectory Homol
	//TODO mostrare i residui
	cd("photogrammetry");

	QDir currentDir = QDir::current();
	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists()) {
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();

	}
	cout << qPrintable(homolDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tapas" << "RadialBasic" << ".*jpg" <<"Out=Relative" << "SH= _mini";

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	if (!process.waitForFinished(-1)) {
		throw QString("Failed to run ") + process.readAllStandardError();
	}
	cout << qPrintable(process.readAllStandardOutput()) << endl;
}

void PanoBuilder::apericloud(){
	//prende l'input dalla sottodirectory Homol
	cd("photogrammetry");

	QDir currentDir = QDir::current();
	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists()) {
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();

	}
	cout << qPrintable(homolDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "AperiCloud" << "Relative" << ".*jpg";

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	if (!process.waitForFinished(-1)) {
		throw QString("Failed to run ") + process.readAllStandardError();
	}
	cout << qPrintable(process.readAllStandardOutput()) << endl;
}

void PanoBuilder::orthoplane(){
	//prende l'input dalla sottodirectory Ori-rel
	cd("photogrammetry");

	QDir currentDir = QDir::current();
	QDir oriRelDir(currentDir.filePath("Ori-Rel"));
	if (!oriRelDir.exists()) {
		throw QString("Ori-Rel directory does not exist in current directory: ") + oriRelDir.absolutePath();
	}


	QString program = mm3d_path;
	QStringList arguments;
	arguments << "" << ""
				 "" << ".*jpg";

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	if (!process.waitForFinished(-1)) {
		throw QString("Failed to run ") + process.readAllStandardError();
	}
	cout << qPrintable(process.readAllStandardOutput()) << endl;
}

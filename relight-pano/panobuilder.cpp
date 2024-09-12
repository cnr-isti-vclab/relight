#include "panobuilder.h"
#include <assert.h>
#include <QDir>
#include <iostream>
#include <QProcess>
#include <QFileInfo>
#include <QImage>
#include <Eigen/Core>
#include "exiftransplant.h"
#include "orixml.h"
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
QDir PanoBuilder::cd(QString path, bool create){
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
	return QDir::current();
}
void PanoBuilder::rmdir(QString path){
	QDir Tmp(QDir::current().filePath(path));
	if(Tmp.exists()){
		Tmp.removeRecursively();
	}

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
	case TAPAS: tapas();
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

	QDir currentDir = cd("photogrammetry", true);
	rmdir("Tmp-MM-Dir");

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
	arguments << "Tapioca" <<"All" << ".*jpg" << "1500" << "@SFS";

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
	QDir currentDir = cd("photogrammetry");

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

	QDir currentDir = cd("photogrammetry");

	//se esiste cancella la directory Tmp-MM-dir
	rmdir("Tmp-MM-Dir");

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
	arguments << "Tapas" << "RadialBasic" << ".*jpg" <<"Out=Relative";

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
	QDir currentDir = cd("photogrammetry");

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
	arguments << "AperiCloud" << ".*jpg" << "Relative";

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
	//prende l'input dalla sottodirectory Ori-relative
	QDir currentDir = cd("photogrammetry");

	QDir oriRelDir(currentDir.filePath("Ori-Relative"));
	if (!oriRelDir.exists()) {
		throw QString("Ori-Relative directory does not exist in current directory: ") + oriRelDir.absolutePath();
	}

	QStringList xmlFiles = oriRelDir.entryList(QStringList() << "Orientation*.xml", QDir::Files);
	if (xmlFiles.isEmpty()) {
		throw QString("No XML files found in Ori-Relative directory");
	}
	QDir oriAbsDir(currentDir.filePath("Ori-Abs"));
	if (!oriAbsDir.exists()){
		if (!currentDir.mkdir("Ori-Abs")) {
			throw QString("Could not create 'Ori-abs' directory");
		}
	}
	OriXml oriXml(oriRelDir.filePath(xmlFiles[0]));

	//Rr0 Matrice di rotazione
	//Cr0 Posizione centro camera.
	Eigen::Matrix3d Rr0 = oriXml.rotation;
	Eigen::Vector3d Cr0 = oriXml.center;

	// Ra0 = Diag(1, -1, -1)
	Eigen::DiagonalMatrix<double, 3> Ra0;
	Ra0.diagonal() << 1, -1, -1;

	// Ca0 = (0, 0, 0)
	Eigen::Vector3d Ca0 = {0, 0, 0};

	//M = (Rr0^-1 * diag(1, -1, -1)) )
	Eigen::Matrix3d M = Rr0.transpose() * Ra0;

	cout << "Matrice M = (Rr0^-1 * diag(1, -1, -1)): " << M << endl;

	oriXml.setOrientation(Ra0, Ca0);
	QString savePath = oriAbsDir.filePath(xmlFiles[0]);
	oriXml.saveOrientation(savePath);

	for (int i = 1; i < xmlFiles.size(); ++i) {
		OriXml ori(oriRelDir.filePath(xmlFiles[i]));
		Eigen::Matrix3d Rr1 = ori.rotation;
		Eigen::Vector3d Cr1 = ori.center;

		Eigen::Matrix3d Ra1 = Rr1 * M;
		Eigen::Vector3d Ca1 = M.transpose() * (Cr1 - Cr0);

		// && Ra2, Ca2 && Ra3, Ca3 && Ra4, Ca4
		ori.setOrientation(Ra1, Ca1);

		QString saveOtherPath = oriAbsDir.filePath(xmlFiles[i]);
		ori.saveOrientation(saveOtherPath);

	}

}

void PanoBuilder::tarama(){
	//prende l'input dalla sottodirectory Ori-abs
	QDir currentDir = cd("photogrammetry");
	rmdir("TA");
	rmdir("Pyram");

	QDir oriAbs(currentDir.filePath("Ori-Abs"));
	if (!oriAbs.exists()) {
		throw QString("Ori-abs directory does not exist in current directory: ") + oriAbs.absolutePath();

	}
	cout << qPrintable(oriAbs.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tarama" << ".*jpg" << "Abs";

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
	cout << qPrintable(process.readAllStandardError()) << endl;
}

void PanoBuilder::malt_mec(){
	//prende l'input dalla sottodirectory TA
	QDir currentDir = cd("photogrammetry");

	QDir taDir(currentDir.filePath("TA"));
	if (!taDir.exists()) {
		throw QString("TA directory does not exist in current directory: ") + taDir.absolutePath();

	}
	cout << qPrintable(taDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Malt" << "Ortho" << ".*jpg" << "Abs" << "ZoomF=4" << "DirMEC=Malt"
			  << "DirTA=TA" << "ImOrtho='Face.*JPG'" << "DirOF=Ortho-RTI";

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

void PanoBuilder::c3dc(){
	//prende l'input dalla sottodirectory
	QDir currentDir = cd("photogrammetry");

//QDir (currentDir.filePath(""));
	//if (!.exists()) {
	//	throw QString(" directory does not exist in current directory: ") + .absolutePath();

	//}
	//cout << qPrintable(taDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "C3DC" << "MicMac" << ".*jpg" << "Abs" <<"DefCor=0.01";

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

void PanoBuilder::malt_ortho(){
	//prende l'input dalla sottodirectory
	QDir currentDir = cd("photogrammetry");

	//QDir (currentDir.filePath(""));
	//if (!.exists()) {
	//	throw QString("TA directory does not exist in current directory: ") + .absolutePath();

//	}
//	cout << qPrintable(.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Malt" << "Ortho" << ".*jpg" << "Abs" << "ZoomF=4" << "DirMEC=Malt"
			  << "DirTA=TA" << "DoMEC=0" << "DoOrtho=1" << "ImOrtho='Face.*JPG'" << "DirOF=Ortho-Couleur";

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

void PanoBuilder::tawny(){
	//prende l'input dalla sottodirectory Malt
	QDir currentDir = cd("photogrammetry");

	QDir maltDir(currentDir.filePath("Malt"));
	if (!maltDir.exists()) {
		throw QString("Ortho-rti directory does not exist in current directory: ") + maltDir.absolutePath();

	}
	cout << qPrintable(maltDir.absolutePath()) << endl;

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tawny" << "Ortho-Couleur" << "RadiomEgal=0" << "" <<"";

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


	//crea un OriXml per ognuno degli xml di orirelative
	//crea Oriabs
	// 1. prendere orientamento della prima camera definisce l'ortopiano

// todo bounding box: definire dove inizia e finisce l'immagine
// 2. prendere varie camere, piano che passa per tutte le camere, es prendendo punto di mezzo delle camere e piano definito. punti tutti le camere e vengono filtrate
// 3. prendere i nuovola punti della nuvola di punti definito in apericloud e leggere la nuvola di punti e definire la nuvola di punti

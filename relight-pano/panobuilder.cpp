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

void PanoBuilder::exportMeans(QDir rtiDir){

	QStringList subDirNames = rtiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	for (const QString &subDirName : subDirNames) {

		QDir currentSubDir(rtiDir.filePath(subDirName));
		QStringList meanFiles = currentSubDir.entryList(QStringList() << "means.png", QDir::Files);
		if (meanFiles.size() == 0)
			throw QString("Missing 'means.png' not found in ") + currentSubDir.path();
		QString meanFile = meanFiles[0];

		QImage img;
		img.load(currentSubDir.filePath("means.png"));
		img.save(subDirName + ".jpg");

		cout << qPrintable(datasets_dir.absolutePath()) << endl;
		QDir dataset(datasets_dir.absoluteFilePath(subDirName));

		QStringList photos = dataset.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
		if (photos.size() == 0)
			throw QString("Missing '*.jpg' not found in ") + dataset.path();
		QString photo = photos[0];

		ExifTransplant exif;
		bool success = exif.transplant(dataset.absoluteFilePath(photo).toStdString().c_str(),
									   (subDirName + ".jpg").toStdString().c_str());
		if(!success)
			throw QString("Unable to load exif from: ") + QString(exif.error.c_str()) + dataset.absoluteFilePath(photo);
	}
}

void PanoBuilder::executeProcess(QString& program, QStringList& arguments) {

	QString command = program + " " + arguments.join(" ");
	cout << "Print command: " << qPrintable(command) << endl;

	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	while(true) {
		bool done = process.waitForFinished(1000);
		QByteArray standardOutput = process.readAllStandardOutput();
		if (!standardOutput.isEmpty()) {
			cout << qPrintable(QString(standardOutput));
		}
		QByteArray standardError = process.readAllStandardError();

		if (!standardError.isEmpty()) {
			cout << "Error: " << qPrintable(QString(standardError)) << endl;
		}
		if(done)
			break;
	}

	if (process.exitStatus() != QProcess::NormalExit) {
		throw QString("Process exited abnormally with exit code: ") + QString::number(process.exitCode());
	}
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
void PanoBuilder::process(Steps starting_step, bool stop){
	switch (starting_step) {
	case RTI:        rti();        if(stop) break;
	case TAPIOCA:    tapioca();    if(stop) break;
	case SCHNAPS:    schnaps();    if(stop) break;
	case TAPAS:      tapas();      if(stop) break;
	case APERICLOUD: apericloud(); if(stop) break;
	case ORTHOPLANE: orthoplane(); if(stop) break;
	case TARAMA:     tarama();     if(stop) break;
	case MALT_MEC:   malt_mec();   if(stop) break;
	case C3DC:       c3dc();       if(stop) break;
	case MALT_ORTHO: malt_ortho(); if(stop) break;
	case TAWNY:      tawny();      if(stop) break;
	case JPG:        jpg();        if(stop) break;

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


	//search the subdirectory, the QDir::Dirs | filter QDir::NoDotAndDotDot to get all subdirectories inside the root directory
	QStringList subDirs = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &subDirName : subDirs) {
		QDir subDir(datasets_dir.filePath(subDirName));
		break;
		//search file .relight
		QStringList relightFiles = subDir.entryList(QStringList() << "*.relight", QDir::Files);
		if(relightFiles.size()==0)
			throw QString("Missing .relight file in folder " )+ subDir.path();
		QString relightFile = relightFiles[0];


		QStringList arguments;
		//arguments << subDir.absoluteFilePath(relightFile) << rtiDir.filePath(subDir.dirName()) <<"-b" << "ptm" << "-p" << "18" << "-m"
		//<<"-3" << "2.5:0.21";
		arguments << datasets_dir.filePath(subDirName) << rtiDir.filePath(subDir.dirName()) <<"-b" << "ptm" << "-p" << "18" << "-m"
				  <<"-3" << "2.5:0.21";

		executeProcess(relight_cli_path, arguments);


	}
	QStringList arguments_merge;
	for (const QString &subDirName : subDirs) {
		arguments_merge << rtiDir.filePath(subDirName);
	}

	arguments_merge << "merge";
	rmdir("merge");
	executeProcess(relight_merge_path, arguments_merge);

}
void PanoBuilder::tapioca(){

	QDir currentDir = cd("photogrammetry", true);
	rmdir("Tmp-MM-Dir");
	QDir rtiDir(base_dir.filePath("rti"));

	cout << qPrintable(base_dir.absolutePath()) << endl;
	cout << qPrintable(base_dir.absoluteFilePath("rti")) << endl;
	QDir (base_dir.absoluteFilePath("rti"));
	if (!rtiDir.exists()) {
		throw QString("merge directory does not exist: ") + rtiDir.absolutePath();
	}

	exportMeans(rtiDir);

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tapioca" <<"All" << ".*jpg" << "1500" << "@SFS";

	executeProcess(program, arguments);
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

	executeProcess(program, arguments);
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

	executeProcess(program, arguments);
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

	executeProcess(program, arguments);
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

	QStringList autoCalFiles = oriRelDir.entryList(QStringList() << "*AutoCal*", QDir::Files);
	for(QString s: autoCalFiles) {
		QFile::copy(oriRelDir.absoluteFilePath(s), oriAbsDir.absoluteFilePath(s));
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

	executeProcess(program, arguments);
}

void PanoBuilder::malt_mec(){
	//prende l'input dalla sottodirectory TA
	QDir currentDir = cd("photogrammetry");
	//("Tmp-MM-Dir");

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
			  << "DirTA=TA" << "ImOrtho=.*jpg" << "DirOF=Ortho-Lights" << "NbVI=2";

	executeProcess(program, arguments);

	QDir orthoLightDir(currentDir.filePath("Ortho-Lights"));

	if (orthoLightDir.exists()) {
		if (!orthoLightDir.removeRecursively()) {
			throw QString("Failed to remove Ortho-Lights directory.");
		} else {
			cout << "Successfully removed Ortho-Lights directory." << endl;
		}
	} else {
		cout << "Ortho-Lights directory does not exist." << endl;
	}
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

	executeProcess(program, arguments);
}

void PanoBuilder::malt_ortho(){
	//prende l'input dalla sottodirectory

	QDir currentDir = cd("photogrammetry");

	QDir mergeDir(base_dir.filePath("merge"));
	if (!mergeDir.exists()) {
		throw QString("Merge dir directory does not exist in base directory: ") + mergeDir.absolutePath();

	}
	QDir rtiDir(base_dir.filePath("rti"));
	if (!rtiDir.exists()) {
		throw QString("rti dir directory does not exist in base directory: ") + rtiDir.absolutePath();

	}
	exportMeans(rtiDir);

	QStringList subDirs = mergeDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	if (subDirs.isEmpty()) {
		throw QString("No subdirectories found in 'merge' directory.");
	}
	int n_planes=1;
		//prende le img nella subdir rti planes
	for (int plane =0; plane<n_planes; plane++){

		for (const QString &subDirName : subDirs) {
			QDir subDir(mergeDir.filePath(subDirName));

			if(n_planes==1){
				QStringList planeFiles = subDir.entryList(QStringList() << "plane_*.jpg", QDir::Files);
				n_planes = planeFiles.size();
			}
			QString planeFileName = QString("plane_%1.jpg").arg(plane);
			QString planeFilePath = subDir.filePath(planeFileName);

			if (!QFile::exists(planeFilePath)) {
				cout << "Warning: " << qPrintable(planeFileName) << " does not exist in directory: " << qPrintable(subDir.absolutePath()) << endl;
				continue;
			}

			QString newFileName = QString("%1.jpg").arg(subDirName);
			QString newFilePath = currentDir.filePath(newFileName);


			ExifTransplant exif;
			bool success = exif.transplant(newFilePath.toStdString().c_str(),
										   planeFilePath.toStdString().c_str());
			if(!success)
				throw QString("Unable to load exif from: ") + QString(exif.error.c_str()) + newFilePath;

			QFile::remove(newFilePath);

			if (!QFile::copy(planeFilePath, newFilePath)) {
				throw QString("Failed to copy and rename file: ") + planeFilePath;
			}
			cout << "Copied planes and renamed: " << qPrintable(planeFilePath) << " to " << qPrintable(newFilePath) << endl;


		}
		// crea la dir ortho_plane_n usala dentro malt

		QString orthoPlaneDirName = QString("Ortho_plane_%1").arg(plane);
		rmdir("Tmp-MM-Dir");

		//QStringList ortImages = orthoLightDir.entryList(QStringList() << "Ort_*.tif", QDir::Files);

		//chiama il malt


		QString program = mm3d_path;
		QStringList arguments;
		arguments << "Malt" << "Ortho" << ".*jpg" << "Abs" << "ZoomF=4"
				  << "DirMEC=Malt" << "DirTA=TA" << "DoMEC=0" << "DoOrtho=1"
				  << "ImOrtho=.*jpg" << "DirOF="+orthoPlaneDirName;

		executeProcess(program, arguments);

		QStringList ortImages = QDir(orthoPlaneDirName).entryList(QStringList() << "Ort_*.tif", QDir::Files);
		if (ortImages.isEmpty()) {
			cout << "Error: No output images found in " << qPrintable(orthoPlaneDirName) << endl;
		} else {
			cout << "Output images found in " << qPrintable(orthoPlaneDirName) << ": " << ortImages.join(", ").toStdString() << endl;
		}


	}
}

void PanoBuilder::tawny(){
	//prende l'input dalla sottodirectory Ortho
	QDir currentDir = cd("photogrammetry");
	int n_planes = 5;

	for (int plane = 0; plane <= n_planes; ++plane) {

		QString planeDirName = QString("Ortho_plane_%1").arg(plane);

		QDir orthoDir(currentDir.filePath(planeDirName));
		if (!orthoDir.exists()) {
			cout << "Directory " << qPrintable(orthoDir.absolutePath()) << " does not exist." << endl;
			continue;
		}
		QStringList tifFiles = orthoDir.entryList(QStringList() << "Ort_Face_*.tif", QDir::Files);
		if (tifFiles.isEmpty()) {
			cout << "No .tif files in " << qPrintable(orthoDir.absolutePath()) << endl;
			continue;
		}

		QString program = mm3d_path;

		QStringList arguments;
		arguments << "Tawny" <<  QString("Ortho_plane_%1").arg(plane) <<"RadiomEgal=0" << "DEq=1" << "DegRap=2" << QString("Out=plane_%1.tif").arg(plane);

		executeProcess(program, arguments);
	}
}


void PanoBuilder::jpg() {
	//prende l'input dalla sottodirectory Ortho Plane. plane_0.tif
	QDir currentDir = cd("photogrammetry");

	int n_planes = 5;

	for (int plane = 0; plane < n_planes; ++plane) {
		\
			QString planeDirName = QString("Ortho_plane_%1").arg(plane);

		QDir orthoDir(currentDir.filePath(planeDirName));
		if (!orthoDir.exists()) {
			cout << "Directory " << qPrintable(orthoDir.absolutePath()) << " does not exist." << endl;
			continue;
		}
		QStringList tifFiles = orthoDir.entryList(QStringList() << "plane_*.tif", QDir::Files);
		if (tifFiles.isEmpty()) {
			cout << "No plane_*.tif files in " << qPrintable(orthoDir.absolutePath()) << endl;
			continue;
		}

		QString tifFile = tifFiles.first();
		QString tifFilePath = orthoDir.filePath(tifFile);

		QImage img;
		if (!img.load(tifFilePath)) {
			cout << "Failed to load image: " << qPrintable(tifFilePath) << endl;
			continue;
		}
		QString jpgFileName = QString("%1.jpg").arg(tifFile);
		QString jpgFilePath = orthoDir.filePath(jpgFileName);

		if (!img.save(jpgFilePath)) {
			cout << "Failed to save image: " << qPrintable(jpgFilePath) << endl;
		} else {
			cout << "Saved image as: " << qPrintable(jpgFilePath) << endl;
		}
	}
}

void PanoBuilder::updateJson(){
	QDir currentDir = cd("photogrammetry");

	QDir rtiDir(base_dir.filePath("rti"));
	if (!rtiDir.exists()) {
		throw QString("rti directory does not exist: ") + rtiDir.absolutePath();
	}
	QStringList subDirs = rtiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	if (subDirs.isEmpty()) {
		throw QString("No subdirectories found in 'rti' directory.");
	}

	for (int plane = 0; plane <subDirs.size(); ++plane) {
		const QString &subDirName = subDirs.at(plane);
		QDir subDir(rtiDir.filePath(subDirName));

		QString jsonFilePath = subDir.filePath("info.json");
		if (!QFile::exists(jsonFilePath)) {
			cout << "Error: " << qPrintable(jsonFilePath) << " does not exist." << endl;
			continue;
		}
		QString destOrthoDirName = QString("Ortho_plane_%1").arg(plane);
		QDir destOrthoDir(currentDir.filePath(destOrthoDirName));

		if (!destOrthoDir.exists()) {
			cout << "Destination directory " << qPrintable(destOrthoDir.absolutePath()) << " does not exist." << endl;
			continue;
		}
		QString destJsonFilePath = destOrthoDir.filePath("info.json");

		if (QFile::exists(destJsonFilePath)) {
			if (!QFile::remove(destJsonFilePath)) {
				cout << "Error: Unable to remove existing file " << qPrintable(destJsonFilePath) << endl;
				continue;
			}
		}
		if (!QFile::copy(jsonFilePath, destJsonFilePath)) {
			cout << "Failed to copy " << qPrintable(jsonFilePath) << " to " << qPrintable(destJsonFilePath) << endl;
			continue;
		}

		cout << "Copied: " << qPrintable(jsonFilePath) << " to " << qPrintable(destJsonFilePath) << endl;
	}
}




//crea un OriXml per ognuno degli xml di orirelative
//crea Oriabs
// 1. prendere orientamento della prima camera definisce l'ortopiano
/*		ExifTransplant exif;
bool success = exif.transplant(currentDir.absoluteFilePath(jpgFiles[0]).toStdString().c_str(),
							   newFilePath.toStdString().c_str());
if (!success) {
	throw QString("Unable to load exif from: ") + QString(exif.error.c_str()) + currentDir.absoluteFilePath(jpgFiles[0]);
}
cout << "Copied and renamed: " << qPrintable(planeFilePath) << " to " << qPrintable(newFilePath) << endl;

		QString planeFilePath = subDir.filePath("plane_*.jpg");
		if (!QFile::exists(planeFilePath)) {
			cout << "Warning: images does not exist in directory: " << qPrintable(subDir.absolutePath()) << endl;
			continue;
		}


*/
//		QString oriAbs = subDir.filePath("Orientation-Face_*.jpg.xml");
//if (!QFile::exists(planeFilePath)) {
//	cout << "Warning: Orientation-Face.xml does not exist in directory: " << qPrintable(subDir.absolutePath()) << endl;
//	continue;
//}

// todo bounding box: definire dove inizia e finisce l'immagine
// 2. prendere varie camere, piano che passa per tutte le camere, es prendendo punto di mezzo delle camere e piano definito. punti tutti le camere e vengono filtrate
// 3. prendere i nuovola punti della nuvola di punti definito in apericloud e leggere la nuvola di punti e definire la nuvola di punti

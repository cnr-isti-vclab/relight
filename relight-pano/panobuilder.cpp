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
#include "orthodepthmap.h"
#include "depthmap.h"
//#define TESTING_PLANE_0
using namespace std;



//TODO: fix the mean directory. Ex. command line: /usr/your/testFace
//tapioca: insert datasets folder absolute path to extract the images
//other programs: photogrammetry
//malt_ortho: needs cdUp because search the exportMeans in subdir of Homol in photogrammetry

PanoBuilder::PanoBuilder(QString base_path)
{
	base_dir = QDir(QDir(base_path).absolutePath());
	assert(base_dir.exists());
	//base_dir.cdUp();

	datasets_dir = QDir(base_dir.filePath("datasets"));
	qDebug() << "Base dir:" << base_dir.absolutePath();

	if (!datasets_dir.exists()) {
		throw QString("Directory datasets non trovata in ") + base_dir.absolutePath();
	}
	photogrammetry_dir = QDir(base_dir.filePath("photogrammetry"));
	if (!photogrammetry_dir.exists()) {
		qDebug() << "Creating:" << base_dir.filePath("photogrammetry");
		if (!base_dir.mkdir("photogrammetry")) {
			throw QString("Could not create directory: photogrammetry");
		}
	}

	QDir::setCurrent(base_dir.absolutePath());
	DefCor = 0.1;
	Regul = 0.1;

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

void PanoBuilder::exportMeans(){

	QStringList subDirNames = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	//TODO: se già create le img devono essere sovrascritte
	for (const QString &subDirName : subDirNames) {

		QDir currentSubDir(datasets_dir.filePath(subDirName));
		QString meanFile = subDirName + ".jpg";
		QString meanPath = datasets_dir.filePath(meanFile);

		QDir dataset(datasets_dir.absoluteFilePath(subDirName));

		QStringList photos = dataset.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
		if (photos.size() == 0)
			throw QString("Missing '*.jpg' not found in ") + dataset.path();
		QString photo = photos[0];

		if(format == "jpg"){
			if (!QFile::copy(meanPath, subDirName + ".jpg")) {
				throw QString("Failed to copy %1 to %2").arg(meanPath).arg(subDirName);
			}
			cout << "Copying EXIF from " << qPrintable(dataset.absoluteFilePath(photo))
				 << " to " << qPrintable(meanPath) << endl;
			ExifTransplant exif;
			bool success = exif.transplant(dataset.absoluteFilePath(photo).toStdString().c_str(),
										   (subDirName + ".jpg").toStdString().c_str());
			if(!success)

			throw QString("Unable to load exif from: ") + QString(exif.error.c_str()) + dataset.absoluteFilePath(photo);
		} else {

			//se formato è jpg copia se è un tif devi fare una conversion
			QString newTifFileName = QString("%1.tif").arg(subDirName);
			QString newTifFilePath = datasets_dir.filePath(newTifFileName);


			QString convertCommand = QString("magick -colorspace RGB %1 -colorspace RGB -compress none %2").arg(meanPath, newTifFilePath);
			cout << qPrintable(convertCommand) << endl;
			int convertResult = system(convertCommand.toStdString().c_str());
			if (convertResult != 0)
				throw QString("Error converting %1 to %2").arg(meanPath, newTifFilePath);

			QString exifCommand = QString("exiftool -tagsfromfile %1 %2").arg(meanPath, newTifFilePath);
			int result = system(exifCommand.toStdString().c_str());
			if (result != 0)
				throw QString("Error copying EXIF data from %1 to %2").arg(meanPath, newTifFilePath);
		}
	}

}

void PanoBuilder::executeProcess(QString& program, QStringList& arguments) {

	QString command = program + " " + arguments.join(" ");
	if(verbose){
		cout << qPrintable(QDir::currentPath()) << endl;
		cout << "Print command: " << qPrintable(command) << endl;
	}
	QProcess process;
	process.start(program, arguments);

	if (!process.waitForStarted()) {
		throw QString("Failed to start ") + process.program();
	}

	while(true) {
		bool done = process.waitForFinished(1000);
		QByteArray standardOutput = process.readAllStandardOutput();
		if (!standardOutput.isEmpty()) {
			if(debug)
				cout << qPrintable(QString(standardOutput));
		}
		QByteArray standardError = process.readAllStandardError();

		if (!standardError.isEmpty()) {
			cout << "Error: " << qPrintable(QString(standardError)) << endl;
		}
		if(done)
			break;
	}

	if (process.exitCode() != 0) {
		throw QString("Process exited abnormally with exit code: ") + QString::number(process.exitCode());

	}
	if (process.exitStatus() != QProcess::NormalExit) {
		throw QString("Process crashed.");

	}
}

//timing
void PanoBuilder::startGlobalTimer() {
	stepTimes.clear();
	globalTimer.start();
	qDebug() << "=== START DATASET TIMING ===";
}

void PanoBuilder::stopGlobalTimer() {
	qint64 totalElapsed = globalTimer.elapsed();
	qDebug() << "=== END DATASET TIMING ===";
	double totalSec = static_cast<double>(totalElapsed) / 1000.0;
	qDebug() << "Total time:" << totalSec << "seconds";
	printTimingReport();
}

void PanoBuilder::runWithTiming(const QString &label, std::function<void()> fn) {
	QElapsedTimer timer;
	timer.start();

	qDebug() << "[" << label << "] Started";
	fn();
	qint64 elapsed = timer.elapsed();

	stepTimes[label] = elapsed;
	double elapsedSec = static_cast<double>(elapsed) / 1000.0;
	qDebug() << "[" << label << "] Finished in" << elapsedSec << "seconds";
}

void PanoBuilder::printTimingReport() {
	qDebug() << "=== STEP TIMING REPORT ===";
	for (auto it = stepTimes.begin(); it != stepTimes.end(); ++it) {
		double sec = static_cast<double>(it.value()) / 1000.0;
		qDebug() << it.key() << ":" << sec << "seconds";
	}
}
//1. funzione findn_planes(Dir);
//1.5 controlla se esiste la directory di destinazione del
//2. leggere quanti plane_* ci sono usando entryList
//3. se n_planes=0 si assegna
//4. altrimenti si controlla se è uguale se non uguale esci

int PanoBuilder::findNPlanes(QDir& dir){
	if (!dir.exists()) {
		throw QString("Directory does not exist: ") + dir.absolutePath();
	}

	int n_planes = 0;
	QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	if (subDirs.isEmpty())
		throw QString("No subdirectories found in directory: %1").arg(dir.absolutePath());

	for (const QString &subDirName : subDirs) {
		QDir subDir(dir.filePath(subDirName));

		subDir.setFilter(QDir::Files);
		subDir.setNameFilters(QStringList() << "plane_*.jpg" << "plane_*.JPG" << "plane_*.jpeg" << "plane_*.JPEG");

		QStringList planeFiles = subDir.entryList();
		int n_planesDir = planeFiles.size();
		cout << "Found " << n_planesDir << " planes in " << qPrintable(subDirName) << endl;

		if (n_planesDir == 0)
			throw QString("No plane files found in " + subDirName);

		if (n_planes == 0) {
			n_planes = n_planesDir;
		} else if (n_planes != n_planesDir) {
			throw QString("Inconsistent number of planes across subdirectories — %1 has %2 planes, expected %3")
				.arg(subDirName)
				.arg(n_planesDir)
				.arg(n_planes);
		}
	}
	if (n_planes == 0)
		throw QString("No plane_* directories found in any subfolder of %1").arg(dir.absolutePath());

	return n_planes;
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
 *		Ori-Rel
 *		...
 *
 */
void PanoBuilder::process(Steps starting_step, bool stop){
	startGlobalTimer();
	switch (starting_step) {

	case MEANS:
		runWithTiming("MEANS", [this]() { means(); });
		if (stop) break;
	case TAPIOCA:
		runWithTiming("TAPIOCA", [this]() { tapioca(); });
		if (stop) break;
	case SCHNAPS:
		runWithTiming("SCHNAPS", [this]() { schnaps(); });
		if (stop) break;
	case TAPAS:
		runWithTiming("TAPAS", [this]() { tapas(); });
		if (stop) break;
	case APERICLOUD:
		runWithTiming("APERICLOUD", [this]() { apericloud(); });
		if (stop) break;
	case ORTHOPLANE:
		runWithTiming("ORTHOPLANE", [this]() { orthoplane(); });
		if (stop) break;
	case TARAMA:
		runWithTiming("TARAMA", [this]() { tarama(); });
		if (stop) break;
	case MALT_MEC:
		runWithTiming("MALT_MEC", [this]() { malt_mec(); });
		if (stop) break;
	case C3DC:
		runWithTiming("C3DC", [this]() { c3dc(); });
		if (stop) break;
	case RTI:
		runWithTiming("RTI", [this]() { rti(); });
		if (stop) break;
	case DEPTHMAP:
		runWithTiming("DEPTHMAP", [this]() { depthmap(); });
		if (stop) break;
	case MALT_ORTHO:
		runWithTiming("MALT_ORTHO", [this]() { malt_ortho(); });
		if (stop) break;
	case TAWNY:
		runWithTiming("TAWNY", [this]() { tawny(); });
		if (stop) break;
	case JPG:
		runWithTiming("JPG", [this]() { jpg(); });
		if (stop) break;
	case UPDATEJSON:
		runWithTiming("UPDATEJSON", [this]() { updateJson(); });
		if (stop) break;
	}
	 stopGlobalTimer();

}

void PanoBuilder::means(){
	//1. iterare sulle sottodir di datasets
	QStringList subDirNames = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &subDirName : subDirNames) {
		QDir currentSubDir(datasets_dir.filePath(subDirName));

		QString inputPath = currentSubDir.absolutePath();
		QString outputPath = datasets_dir.filePath(subDirName + ".jpg");

		//"relight-cli -b skip -m", input, output("output.jpg");
		//output si salva dentro la dir datasets
		QString relightCliPath = relight_cli_path;
		QStringList arguments;
		arguments <<"-b" << "skip" << "-m" << inputPath << outputPath;
		executeProcess(relightCliPath, arguments);

		QString command = relightCliPath + " " + arguments.join(" ");
		if (verbose) {
			cout << "Running command: " << qPrintable(command) << endl;
		}

	}

	//2. modifica l'export means dentro photogrammetry
	exportMeans();
}

void PanoBuilder::rti(){
	QDir rtiDir(base_dir.filePath("rti"));
	if (!rtiDir.exists()) {
		if (!base_dir.mkdir("rti")) {
			throw QString("Could not create 'rti' directory");

		}
	}
	//search the subdirectory, the QDir::Dirs | filter QDir::NoDotAndDotDot to get all subdirectories inside the root directory
	QStringList subDirs = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	//TODO
	if (verbose) {
		cout << "Found " << subDirs.size() << " subdirectories in dataset directory: "
			 << qPrintable(datasets_dir.absolutePath()) << endl;
	}
	for (const QString &subDirName : subDirs) {
		QDir subDir(datasets_dir.filePath(subDirName));
		//search file .relight
		QStringList relightFiles = subDir.entryList(QStringList() << "*.relight", QDir::Files);
		/*if(relightFiles.size()==0)
			throw QString("Missing .relight file in folder " )+ subDir.path();
		QString relightFile = relightFiles[0];*/


		QStringList arguments;
		//arguments << subDir.absoluteFilePath(relightFile) << rtiDir.filePath(subDir.dirName()) <<"-b" << "ptm" << "-p" << "18" << "-m"
		//<<"-3" << "2.5:0.21";

		arguments << datasets_dir.filePath(subDirName) << rtiDir.filePath(subDir.dirName()) <<"-b" << "ptm" << "-p" << "18";
			// <<"-3" << "2.5:0.21" 3 1.6:0.04 ;
		if(!light3d.isEmpty())
			arguments << "-3" << light3d;

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
/*Tie Points computation using Tapioca: Tapioca runs Pastis program (for tie points matching) using SIFT algorithm an
transforms the pictures (creating .tif files in grayscale) to make them usable for the
different steps of micmac*/
//TODO: choose to users tapioca parameters 1500 as the scale where the correspondence detection between images is performed.
void PanoBuilder::tapioca(){

	QDir currentDir = cd("photogrammetry", true);
	rmdir("Tmp-MM-Dir");
	QDir rtiDir(base_dir.filePath("rti"));

	QDir (base_dir.absoluteFilePath("rti"));
	if (!rtiDir.exists()) {
		throw QString("merge directory does not exist: ") + rtiDir.absolutePath();
	}

	exportMeans();
	QStringList subDirNames = datasets_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &subDirName : subDirNames) {
		QString meanFile = datasets_dir.filePath(subDirName + ".jpg");
		if (!QFile::exists(meanFile)) {
			throw QString("Error: Expected file missing after exportMeans: ") + meanFile;
		}
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tapioca" <<"All" << ".*" + format << "1500" << "@SFS";

	executeProcess(program, arguments);
}

void PanoBuilder::schnaps(){
	QDir currentDir = cd("photogrammetry");

	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists()) {
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();

	}

	QStringList jpgFiles = currentDir.entryList(QStringList() << "*." + format, QDir::Files);
	if (jpgFiles.isEmpty()) {
		throw QString("No JPEG images found in photogrammetry directory");
	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Schnaps" << ".*" + format << "MoveBadImgs=1";

	executeProcess(program, arguments);

	QDir poubelleDir(currentDir.filePath("Poubelle"));
	if (poubelleDir.exists()) {
		QStringList rejectImg = poubelleDir.entryList(QStringList() << "*." + format, QDir::Files);

		if (!rejectImg.isEmpty()) {
			QString errorMsg = "Error: The following images were moved to Poubelle due to poor alignment:\n";
			for (const QString &rejectImgs : rejectImg) {
				errorMsg += rejectImgs + "\n";
			}
			throw errorMsg;
		}
	} else {
		QString poubelleTxtPath = currentDir.filePath("Schnaps_poubelle.txt");
		QFile poubelleTxt(poubelleTxtPath);
		if (poubelleTxt.exists() && poubelleTxt.size() > 0) {
			throw QString("No images were moved to Poubelle, but suspicious images are listed in Schnaps_poubelle.txt");
		}
	}
}

// different modes for the calibration/orientation
void PanoBuilder::tapas(){

	QDir currentDir = cd("photogrammetry");
	rmdir("Tmp-MM-Dir");

	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists()) {
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();

	}
	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tapas" << "RadialBasic" << ".*" + format << "Out=Relative";

	executeProcess(program, arguments);
}
//note: dataset papiro non usabile perchè troppo allineato, l'errore che dà tapas è che la
//Distortion Inversion by finite difference do not converge (probably ill-conditioned canvas)
//nell homol si trovano le convergenze dove la y è allineata tra un'img e l'altra.

void PanoBuilder::apericloud(){
	QDir currentDir = cd("photogrammetry");

	QDir homolDir(currentDir.filePath("Homol"));
	if (!homolDir.exists())
		throw QString("Homol directory does not exist in current directory: ") + homolDir.absolutePath();
	QString program = mm3d_path;
	QStringList arguments;
	arguments << "AperiCloud" << ".*" + format << "Relative" << "Bin=0" << "WithCam=0";

	executeProcess(program, arguments);
}

// At this moment, the function Ori-Abs is not being used in the upcoming functions
// It will be revisited and debugged later
void PanoBuilder::orthoplane(){

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
	QDir currentDir = cd("photogrammetry");
	rmdir("TA");
	rmdir("Pyram");

	QDir oriRel(currentDir.filePath("Ori-Relative"));
	if (!oriRel.exists()) {
		throw QString("Ori-Relative directory does not exist in current directory: ") + oriRel.absolutePath();

	}

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Tarama" << ".*" + format
			  << "Relative";

	executeProcess(program, arguments);
}

void PanoBuilder::malt_mec(){
	QDir currentDir = cd("photogrammetry");

	QDir taDir(currentDir.filePath("TA"));
	if (!taDir.exists())
		throw QString("TA directory does not exist in current directory: ") + taDir.absolutePath();

	rmdir("Ortho-Lights");
	rmdir("Tmp-MM-Dir");

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "Malt" << "Ortho" << ".*" + format << "Relative" << "DoOrtho=1" << "ZoomF=4" << "DirMEC=Malt"
			  << "DirTA=TA" << "ImOrtho=.*" + format << "DirOF=Ortho-Lights" << "NbVI=3" << "Purge=true"
			  << QString("DefCor=%1").arg(DefCor)
			  << QString("Regul=%1").arg(Regul);
	//DefCor 2 is to big
	executeProcess(program, arguments);

	QString depthmapPath = currentDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT.tif");
	QString maskPath     = currentDir.filePath("Malt/Masq_STD-MALT_DeZoom4.tif");


	QString depthmapBackup = depthmapPath + "_backup.tif";
	QFile::remove(depthmapBackup);
	QFile::copy(depthmapPath, depthmapBackup);

	QString maskBackup = maskPath + "_backup.tif";
	QFile::remove(maskBackup);
	QFile::copy(maskPath, maskBackup);

	// Backup XML + TFW (stessa regola nome + "_backup" + estensione)
	QString depthXml = depthmapPath + ".xml";
	if (QFile::exists(depthXml)) {
		QString depthXmlBackup = depthXml + "_backup.xml";
		QFile::remove(depthXmlBackup);
		QFile::copy(depthXml, depthXmlBackup);
	}

	QString depthTfw = depthmapPath.left(depthmapPath.size() - 4) + ".tfw";
	if (QFile::exists(depthTfw)) {
		QString depthTfwBackup = depthTfw + "_backup.tfw";
		QFile::remove(depthTfwBackup);
		QFile::copy(depthTfw, depthTfwBackup);
	}
}

void PanoBuilder::c3dc(){
	return;
	QDir currentDir = cd("photogrammetry");

	QDir oriDir(currentDir.filePath("Ori-Relative"));
	if (!oriDir.exists())
		throw QString("Ori directory does not exist in current directory: ") + oriDir.absolutePath();

	QString program = mm3d_path;
	QStringList arguments;
	arguments << "C3DC" << "MicMac" << ".*" + format << "Relative" << QString("DefCor=%1").arg(DefCor);;

	executeProcess(program, arguments);
}

void PanoBuilder::depthmap(){

	QDir currentDir = cd("photogrammetry");

	QString depthmapPath = currentDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT.tif");
	QString maskPath     = currentDir.filePath("Malt/Masq_STD-MALT_DeZoom4.tif");
	QString plyFile      = currentDir.filePath("AperiCloud_Relative.ply");
	QString depthXml     = currentDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT.xml");
	QString output_points = base_dir.filePath("points_h.txt");

	// Use backup file
	QString depthmapBackup = depthmapPath + "_backup.tif";
	QString maskBackup     = maskPath + "_backup.tif";
	QString depthXmlBackup = depthXml + "_backup.xml";


	OrthoDepthmap ortho;

	if (!ortho.load(qPrintable(depthmapPath), qPrintable(maskPath))) {
		cerr << "Failed to load depthmap or mask" << endl;
		return;
	}

	QDir datasetsDir(base_dir.filePath("datasets"));
	QDir xmlDir(base_dir.filePath("photogrammetry/Ori-Relative"));

	QStringList extensions = {".tiff", ".tif", ".jpg", ".jpeg"};
	QStringList tiffFilters = {"*.tiff"};

	QFileInfoList tiffFiles = datasetsDir.entryInfoList(tiffFilters, QDir::Files);
	if (tiffFiles.isEmpty()) {
		cerr << "No .tiff file find in " << datasetsDir.absolutePath().toStdString() << endl;
		return;
	}

	QFileInfoList xmlFiles = xmlDir.entryInfoList({"*.xml"}, QDir::Files);
	if (xmlFiles.isEmpty()) {
		cerr << "No .xml file find in " << xmlDir.absolutePath().toStdString() << endl;
		return;
	}

	try {
		ortho.loadDepth(qPrintable(depthmapPath));
		ortho.loadMask(qPrintable(maskPath));
		ortho.loadPointCloud(qPrintable(plyFile));
	} catch (QString e) {
		cerr << "Error loading depth data: " << qPrintable(e) << endl;
		return;
	}

	ortho.beginIntegration();

	for (const QFileInfo &tiffFile : tiffFiles) {
		CameraDepthmap depthCam;
		QString cameraName = tiffFile.completeBaseName();
		QString orientationXmlPath;

		for (const QString &ext : extensions) {
			QString potentialPath = xmlDir.absoluteFilePath("Orientation-" + cameraName + ext + ".xml");
			if (QFile::exists(potentialPath)) {
				orientationXmlPath = potentialPath;
				break;
			}
		}
		if(orientationXmlPath.isEmpty())
			throw(QString("Missing camera parameter xml file for image: %1").arg(cameraName));

		if (!depthCam.camera.loadXml(orientationXmlPath)) {
			throw QString("Missing or invalid XML: " + orientationXmlPath);
		}

		if (!depthCam.loadDepth(qPrintable(tiffFile.absoluteFilePath()))) {
			throw QString("Unable to load depth map: " + tiffFile.fileName());
		}

		if (depthCam.width != depthCam.camera.width || depthCam.height != depthCam.camera.height) {
			qDebug() << "Resizing rti depthmap: " << tiffFile.fileName();
			depthCam.camera.scale(depthCam.width, depthCam.height);
		}

		cout << "Processed: " << tiffFile.fileName().toStdString() << endl;
		ortho.integratedCamera(depthCam, qPrintable(output_points));
	}

	ortho.endIntegration();
	ortho.saveDepth(qPrintable(depthmapPath));
	ortho.saveMask(qPrintable(maskPath));
	//ortho.saveObj("weightsElev3_0125.obj");

}
//for each directory in merge folder, copy the xml file in ori-relative file and rename plane_0, plane_1 ecc. after remove it.
//malt_ortho project the all the plane and create the output folder Ortho_plane inside we found the directory projection.
void PanoBuilder::malt_ortho() {
	QDir currentDir = cd("photogrammetry");

	QDir mergeDir(base_dir.filePath("merge"));
	if (!mergeDir.exists()) {
		throw QString("Merge dir directory does not exist in base directory: ") + mergeDir.absolutePath();
	}

	QDir oriDir(currentDir.filePath("Ori-Relative"));
	if (!oriDir.exists()) {
		throw QString("Ori-Relative directory does not exist in current directory: ") + oriDir.absolutePath();
	}

	QStringList subDirs = mergeDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	if (subDirs.isEmpty())
		throw QString("No subdirectories found in 'merge' directory.");

	int n_planes = findNPlanes(mergeDir);

	for (int plane = 0; plane < n_planes; plane++) {

		QString orthoPlaneDirName = QString("Ortho_plane_%1").arg(plane);
		rmdir("Tmp-MM-Dir");
		rmdir(orthoPlaneDirName);

		QStringList tempXmlPaths;
		QStringList tempPlanePaths;

		// Cp and rename XML file
		for (int i = 0; i < subDirs.size(); i++) {
			QString subDirName = subDirs[i];
			QDir subDir(mergeDir.filePath(subDirName));
			QString planeFileName = QString("plane_%1.jpg").arg(plane);
			QString planeFilePath = subDir.filePath(planeFileName);

			if (!QFile::exists(planeFilePath)) {
				throw QString("Plane image missing: " + planeFilePath);
			}

			// Copia il file plane_X.jpg nella directory photogrammetry/
			QString destPlaneName = QString("plane_%1_%2.jpg").arg(plane).arg(subDirName);
			QString destPlanePath = currentDir.filePath(destPlaneName);
			if (QFile::exists(destPlanePath))
				QFile::remove(destPlanePath);

			if (QFile::copy(planeFilePath, destPlanePath)) {
				tempPlanePaths << destPlanePath;
				if (verbose)
					cout << "Copied " << qPrintable(planeFileName)
						 << " from " << qPrintable(subDirName) << endl;
			} else {
				throw QString("Failed to copy plane image: " + planeFilePath);
			}

			//
			QString sourceXmlName = QString("Orientation-%1.jpg.xml").arg(subDirName);
			QString sourceXmlPath = oriDir.filePath(sourceXmlName);

			if (!QFile::exists(sourceXmlPath)) {
				throw QString("Missing orientation file: " + sourceXmlPath);
			}

			QString planeXmlName = QString("Orientation-plane_%1_%2.jpg.xml").arg(plane).arg(subDirName);
			QString planeXmlPath = oriDir.filePath(planeXmlName);

			if (QFile::exists(planeXmlPath))
				QFile::remove(planeXmlPath);

			if (QFile::copy(sourceXmlPath, planeXmlPath)) {
				tempXmlPaths << planeXmlPath;
				if (verbose)
					cout << "Copied " << qPrintable(sourceXmlName)
						 << " -> " << qPrintable(planeXmlName) << endl;
			} else {
				cout << "Failed to copy " << qPrintable(sourceXmlPath) << endl;
			}
		}


		QString program = mm3d_path;
		QStringList arguments;
		arguments << "Malt" << "Ortho" << "plane_.*" + format << "Relative" << "ZoomF=4" << "DirMEC=Malt" << "DirTA=TA" << "DoMEC=0"
				  << "DoOrtho=1" << "Purge=false" << "ImOrtho=plane_.*" + format << "DirOF=" +orthoPlaneDirName;

		try {
			executeProcess(program, arguments);
		} catch (QString &e) {
			cout << "Error during Malt Ortho for plane " << plane << ": "
				 << qPrintable(e) << endl;
			cout << "Command Line: " << qPrintable(arguments.join(" ")) << endl;
		}

		// Cancella i file temporanei
		for (const QString &tempXml : tempXmlPaths) {
			if (QFile::exists(tempXml)) {
				QFile::remove(tempXml);
				if (verbose)
					cout << "Removed temp xml: " << qPrintable(tempXml) << endl;
			}
				// cancella anche le immagini photogrammetry/plane_
		}
		for (const QString &tempPlane : tempPlanePaths) {
			if (QFile::exists(tempPlane)) {
				QFile::remove(tempPlane);
				if (verbose)
					cout << "Removed temp plane image: " << qPrintable(tempPlane) << endl;
			}
		}


		cout << "Completed Malt Ortho for plane " << plane << endl;
	}
}


//QString exifCommand = QString("exiftool -tagsfromfile %1 %2").arg(planeFilePath, newTifFilePath);
//int result = system(exifCommand.toStdString().c_str());
//if (result != 0)
//	throw QString("Error copying EXIF data from %1 to %2").arg(planeFilePath, newTifFilePath);

//else if (!QFile::exists(newTifFilePath)) {

//QString newFileName = QString("%1.jpg").arg(subDirName);
//QString newFilePath = currentDir.filePath(newFileName);
/*if(verbose)
			cout << "Copied planes and renamed: " << qPrintable(planeFilePath) << " to " << qPrintable(newFilePath) << endl;*/

//QString orthoPlaneDirName = QString("Ortho_plane_%1").arg(plane);
//rmdir("Tmp-MM-Dir");
//rmdir(orthoPlaneDirName);


/*	QString depthmapPath = base_dir.filePath("photogrammetry/Malt/Z_Num7_DeZoom4_STD-MALT.tif");
		if (!QFile::copy(depthmapPath + "_backup.tif", depthmapPath)) {
			cout << "Error copying depthmap" << depthmapPath.toStdString() << endl;
			exit(0);
		}

		QStringList ortImages = QDir(orthoPlaneDirName).entryList(QStringList() << "Ort_*.tif", QDir::Files);
		if (ortImages.isEmpty()) {
			throw QString("Error: No output images found in ").arg(orthoPlaneDirName);
		}

	}
	exportMeans();*/

//sistema di coordinate: capire dove sta il 3d del punto dell'ori rel. trasformazione del punto con formule
// rti fa l img media non la deve fare l rti si crea in tif, sposta rti dopo il malt mec e si fa direttamente l'img media

// trova i coefficienti del tawny per vedere le sovrapposizioni degli ortho
void PanoBuilder::tawny() {
	QDir currentDir = cd("photogrammetry");

	//int n_planes = findNPlanes(currentDir);
	QStringList planeDirs  = currentDir.entryList(QStringList() << "Ortho_plane_*", QDir::Dirs);

	//for (int plane = 0; plane <= n_planes; ++plane) {
	for (const QString& planeDirName : planeDirs) {
		QDir orthoDir(currentDir.filePath(planeDirName));
		QString plane = planeDirName.split("_").last();
		//QString planeDirName = QString("Ortho_plane_%1").arg(plane);
		if (!orthoDir.exists()) {
			throw QString("Directory %1 does not exist").arg(orthoDir.absolutePath());
		}
		QStringList tifFiles = orthoDir.entryList(QStringList() << "Ort_*.tif", QDir::Files);
		if (tifFiles.isEmpty())
			throw QString("No .tif files in ").arg(orthoDir.absolutePath());
		QString program = mm3d_path;


		QStringList arguments;
		arguments << "Tawny" << planeDirName << "DEq=0" << "RadiomEgal=0" << "DegRap=0" << QString("Out=plane_%1.tif").arg(plane);
		try {
			executeProcess(program, arguments);
		} catch (QString &e) {
			cout << "Error during Tawny: " << qPrintable(e) << endl;
			cout << "Command Line: " << qPrintable(arguments.join(" ")) << endl;
		}
	}
}
// <<"DEq=3" << "DegRap=2" << "SzV=5"
//<< "CorThr=0.2"<< "NbPerIm=5e4"
/*1.	DEq: Determina il grado del polinomio per equalizzare le singole immagini. Più alto è il valore, più complessa sarà l’equalizzazione.
	•	DEq=0: Minimo livello di equalizzazione. Potrebbe lasciare visibili i bordi.
	•	DEq=1: Grado 1, è il valore di default. Riduce i bordi, ma può introdurre drift.
	•	DEq=2 o superiore: Gradi più alti per migliorare la continuità delle immagini.
	2.	DegRap: Controlla il polinomio globale per ridurre i drift radiometrici.
	•	DegRap=0: Nessun controllo globale.
	•	DegRap=1: Controllo minimo globale, bilancia l’equalizzazione globale.
	•	DegRap=2 o superiore: Livelli più alti per migliorare la qualità.
	3.	SzV: Definisce la dimensione della finestra per l’equalizzazione locale. Un valore maggiore può migliorare la qualità riducendo i bordi visibili.
	•	SzV=3: Una finestra 3x3 può essere utile per immagini difficili.
	4.	CorThr: Soglia di correlazione per la validazione dei punti omologhi tra le immagini. Un valore più basso accetta più punti, ma potrebbe introdurre più rumore.
	•	CorThr=0.6: Valore tipico. Abbassarlo (es. 0.5) può aiutare a mantenere più punti per l’equalizzazione.
	5.	NbPerIm: Numero di punti di campionamento per immagine. Aumentare questo valore potrebbe aiutare nelle immagini difficili.
	•	NbPerIm=5e4: Di default, si campionano circa 50.000 punti per immagine. Potrebbe essere utile aumentarli a NbPerIm=1e5.*/


void PanoBuilder::jpg() {
	//prende l'input dalla sottodirectory Ortho Plane. plane_0.tif
	QDir currentDir = cd("photogrammetry");
	QStringList planeDirs = currentDir.entryList(QStringList() << "Ortho_plane_*", QDir::Dirs);
	rmdir("Panorama");
	currentDir.mkdir("Panorama");
	QDir panoramaDir("Panorama");
	//	for (int plane = 0; plane < n_planes; ++plane) {
	for (const QString& planeDirName : planeDirs) {
		//QString planeDirName = QString("Ortho_plane_%1").arg(plane);

		QDir orthoDir(currentDir.filePath(planeDirName));
		if (!orthoDir.exists()) {
			throw QString("Directory %1 does not exist").arg(orthoDir.absolutePath());
		}
		QStringList tifFiles = orthoDir.entryList(QStringList() << "plane_*.tif", QDir::Files);
		if (tifFiles.isEmpty()) {
			throw QString("No plane_*.tif files in " + orthoDir.absolutePath());
		}

		QString tifFile = tifFiles.first();
		QString tifFilePath = orthoDir.filePath(tifFile);

		QString baseName = tifFile.split(".").first();
		QImage img;
		if (!img.load(tifFilePath)) {
			throw QString("Failed to load image: ").arg(tifFilePath);
		}
		QString jpgFileName = QString("%1.jpg").arg(baseName);
		QString jpgFilePath = panoramaDir.filePath(jpgFileName);

		if (!img.save(jpgFilePath)) {
			throw QString("Failed to save image: ").arg(jpgFilePath);
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

	QString subDirName = subDirs[0];
	QDir subDir(rtiDir.filePath(subDirName));

	QString jsonFilePath = subDir.filePath("info.json");
	if (!QFile::exists(jsonFilePath)) {
		throw QString("Error: %1 does not exist.").arg(jsonFilePath);
	}
	QDir panoramaDir("Panorama");

	QString destJsonFilePath = panoramaDir.filePath("info.json");

	if (!QFile::copy(jsonFilePath, destJsonFilePath)) {
		throw QString("Failed to copy %1  to ").arg(jsonFilePath).arg(destJsonFilePath);
	}

}



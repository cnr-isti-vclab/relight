#ifndef PANOBUILDER_H
#define PANOBUILDER_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <QMap>

class PanoBuilder : public QObject
{
	Q_OBJECT
public:

	enum Base {
		PTM,
		HSH
	};

	enum Steps{

		MEANS = 0, //rti e merge e medie
		TAPIOCA,
		SCHNAPS,
		TAPAS,
		APERICLOUD,
		ORTHOPLANE,
		TARAMA,
		MALT_MEC,
		C3DC,
		RTI,
		NORMALS,
		DEPTHMAP,
		MALT_ORTHO, //copy orientation xml and exif + ortho per image
		TAWNY,
		JPG,//convert to jpg
		UPDATEJSON
	};


	QStringList steps = {"means", "tapioca", "schnaps", "tapas", "apericloud", "orthoplane", "tarama", "malt_mec", "c3dc", "rti", "normals", "depthmap", "malt_ortho", "tawny", "jpg", "updateJson"};
	QDir base_dir;
	QDir datasets_dir;
	QDir additional_dir;
	QDir photogrammetry_dir;
	QString mm3d_path;
	QString relight_cli_path;
	QString relight_merge_path;
	QString relight_seam_path;
	QString relight_normals_path;
	void setSeam(const QString &path) { relight_seam_path = path; }
	void setRelightNormals(QString path);


	QFile log;
	//use DefCor and Regul in malt_mec function, set with default value
	double DefCor = 0.2;
	double Regul = 0.05;
	bool verbose = false;
	bool debug = false;
	QString format = "jpg";
	QString light3d;
	Base base = HSH;


	PanoBuilder(QString path);
	void setMm3d(QString path);
	void setRelightCli(QString path);
	void setRelightMerge(QString path);
	int findStep(QString step);
	int findNPlanes(QDir& dir);
	void exportMeans();
	void exportAdditional();
	void transplantExif();
	void executeProcess(QString& process, QStringList& arguments);
	void process(Steps starting_step = RTI, bool stop = false);
	void means();
	//create the directory rti process the datasets and relight-merge the rti planes
	void rti();
	void tapioca();
	void schnaps();
	void tapas();
	void apericloud();
	void orthoplane();
	void tarama();
	void malt_mec();
	void c3dc();
	void loadOrthoPlanes();
	void tawny();
	void normals();
	void depthmap();
	void malt_ortho();
	void jpg();
	void updateJson();

signals:
private:
	void ensureExecutable(QString path);
	void transplantExif(const QString &src, const QString &dest);
	QDir cd(QString path, bool create = false);
	void rmdir(QString path);

	QElapsedTimer globalTimer;
	QMap<QString, qint64> stepTimes;

	void startGlobalTimer();
	void stopGlobalTimer();
	void runWithTiming(const QString &label, std::function<void()> fn);
	void printTimingReport();

};
#endif // PANOBUILDER_H

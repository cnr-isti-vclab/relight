#ifndef PANOBUILDER_H
#define PANOBUILDER_H

#include <QObject>
#include <QFile>
#include <QDir>

class PanoBuilder : public QObject
{
	Q_OBJECT
public:
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
		MALT_ORTHO, //copy orientation xml and exif + ortho per image
		TAWNY,
		JPG,//convert to jpg
		UPDATEJSON
	};

	QStringList steps = {"means", "tapioca", "schnaps", "tapas", "apericloud", "orthoplane", "tarama", "malt_mec", "c3dc","rti", "malt_ortho", "tawny", "jpg", "updateJson"};
	QDir base_dir;
	QDir datasets_dir;
	QDir photogrammetry_dir;
	QString mm3d_path;
	QString relight_cli_path;
	QString relight_merge_path;

	QFile log;
	//use DefCor and Regul in malt_mec function, set with default value
	double DefCor = 0.2;
	double Regul = 0.05;
	bool verbose = false;
	bool debug = false;
	QString format = "jpg";

	PanoBuilder(QString path);
	void setMm3d(QString path);
	void setRelightCli(QString path);
	void setRelightMerge(QString path);
	int findStep(QString step);
	int findNPlanes(QDir& dir);
	void exportMeans();
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
	void tawny();
	void malt_ortho();
	void jpg();
	void updateJson();


signals:
private:
	void ensureExecutable(QString path);
	QDir cd(QString path, bool create = false);
	void rmdir(QString path);

};
#endif // PANOBUILDER_H

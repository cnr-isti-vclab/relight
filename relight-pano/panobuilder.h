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
		RTI = 0, //rti e merge e medie
		TAPIOCA,
		SCHNAPS,
		TAPAS,
		APERICLOUD,
		ORTHOPLANE,
		TARAMA,
		MALT_MEC,
		C3DC,
		MALT_ORTHO, //copy orientation xml and exif + ortho per image
		TAWNY,
		JPG //convert to jpg
	};

	QStringList steps = {"rti", "tapioca"};
	QDir base_dir;
	QDir datasets_dir;
	QString mm3d_path;
	QString relight_cli_path;
	QString relight_merge_path;

	QFile log;

	PanoBuilder(QString path);
	void setMm3d(QString path);
	void setRelightCli(QString path);
	void setRelightMerge(QString path);
	int findStep(QString step);

	void process(Steps starting_step = RTI);
	//create the directory rti process the datasets and relight-merge the rti planes
	void rti();
	void tapioca();
	void schnaps(){};
	void tapas(){};
	void apericloud(){};
	void orthoplane(){};
	void tarama(){};
	void malt_mec(){};
	void c3dc(){};
	void malt_ortho(){};
	void tawny(){};
	void jpg(){};



signals:
private:
	void ensureExecutable(QString path);
	void cd(QString path, bool create = false);
};
#endif // PANOBUILDER_H

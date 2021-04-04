#ifndef RTIEXPORT_H
#define RTIEXPORT_H

#include <QDialog>
#include <map>
#include <QFutureWatcher>
#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"

#include "httpserver.h"

namespace Ui {
	class RtiExport;
}

class QProgressDialog;
class QGraphicsPixmapItem;

class RtiExport : public QDialog
{
	Q_OBJECT

public:
	QStringList images;
	std::vector<Vector3f> lights;
	QRect crop;
	QString path;

	QProgressDialog *progressbar;
	QFutureWatcher<void> watcher;
	bool cancel = false;

	explicit RtiExport(QWidget *parent = nullptr);
	~RtiExport();
	Rti::Type basis();
	Rti::ColorSpace colorSpace();
	
	void setImages(QStringList images);
	void setCrop(QRect rect);
	void showImage(QPixmap pix);
	
	int quality();
	int planes();
	int chroma();
	int top();
	int left();
	int width();
	int height();

	QVariant getOption(QString key);
	void setOption(QString key, QVariant value);
	bool callback(std::string s, int n);

	enum Format { PTM, RTI, RELIGHT, DEEPZOOM, TARZOOM };

	void makeRti(QString output, QRect rect = QRect(0, 0, 0, 0), Format format = RELIGHT, bool means = false, bool normals = false, bool highNormals = false);

public slots:
	void changeBasis(int n);
	void changePlanes(int n);
	void createRTI(QString output);
	void createRTI(bool view = false);
	void createNormals();
	void createRTIandView();
	void finishedProcess();
	void cancelProcess();
	void close();

	void showCrop();
	void acceptCrop();
	void rejectCrop();
	void cropChanged(QRect rect); //someone moved the crop rectangle
	void updateCrop(); //someone edited the spinboxs
	void setAspectRatio(int aspect);

signals:
	void progressText(const QString &str);
	void progress(int n);
private:
	Ui::RtiExport *ui;
	HttpServer server;
	bool viewAfter = false;
	QString outputFolder;
};

#endif // RTIEXPORT_H


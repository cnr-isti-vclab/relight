#ifndef RTIEXPORT_H
#define RTIEXPORT_H

#include <QDialog>
#include <map>
#include <QFutureWatcher>

#include "httpserver.h"
#include "../src/relight_vector.h"

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
	
	void setImages(QStringList images);
	void setCrop(QRect rect);
	void showImage(QPixmap pix);

	enum Format { PTM, RTI, RELIGHT, DEEPZOOM, TARZOOM };

public slots:
	void setTabIndex(int index);

	void changeBasis(int n);
	void changePlanes(int n);

	void createRTI();
    void createRTI(QString output);

	void createNormals();
	void close();

	void showCrop();
	void acceptCrop();
	void resetCrop();
	void rejectCrop();
	void cropChanged(QRect rect); //someone moved the crop rectangle
	void updateCrop(); //someone edited the spinboxs
	void setAspectRatio(int aspect);

signals:
	void progressText(const QString &str);
	void progress(int n);

public:
	Ui::RtiExport *ui;
	HttpServer server;
	QString outputFolder;
};

#endif // RTIEXPORT_H


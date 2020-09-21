#ifndef RTIEXPORT_H
#define RTIEXPORT_H

#include <QDialog>
#include <map>
#include <QFutureWatcher>
#include "../src/rti.h"
#include "../relight/rtibuilder.h"

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
	QString path;

	QProgressDialog *progressbar;
	QFutureWatcher<void> watcher;


	explicit RtiExport(QWidget *parent = 0);
	~RtiExport();
	Rti::Type basis();
	Rti::ColorSpace colorSpace();
	
	void setImages(QStringList images);
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
	void callback(std::string s, int n);

	void makeRti(QString output, QRect rect = QRect(0, 0, 0, 0));

public slots:
	void changeBasis(int n);
	void changePlanes(int n);
	void createRTI();
	void finishedProcess();

	void showCrop();
	void acceptCrop();
	void rejectCrop();
	void cropChanged(QRect rect); //someone moved the crop rectangle
	void updateCrop(); //someone edited the spinboxs

signals:
	void progressText(const QString &str);
	void progress(int n);
private:
	Ui::RtiExport *ui;
};

#endif // RTIEXPORT_H


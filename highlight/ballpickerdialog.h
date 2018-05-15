#ifndef BALLPICKERDIALOG_H
#define BALLPICKERDIALOG_H

#include <QDialog>
#include <QDir>
#include <vector>

namespace Ui {
	class BallPickerDialog;
}

class QGraphicsScene;
class QGraphicsEllipseItem;
class QImage;
class QString;

class BallPickerDialog : public QDialog
{
	Q_OBJECT

public:
	QPointF center;
	float radius;
	float smallradius;
	QSize imgsize;
	QRect inner;

	bool fitted;
	bool processed;

	QDir dir;
	QStringList balls;
	std::vector<QPointF> lights;

	explicit BallPickerDialog(QWidget *parent = 0);
	bool init(QString dir);
	bool loadImage(QString filename);
	~BallPickerDialog();

	virtual void mouseDoubleClickEvent(QMouseEvent *event);

public slots:
	void open();
	void fit();
	void process();
	void save();

	//todo: next image, previous image. write image number.

private:
	QPointF findLightDir(QImage &sphere, QString filename);

	Ui::BallPickerDialog *ui;
	QGraphicsScene *scene;
	QGraphicsEllipseItem *circle;
};

#endif // BALLPICKERDIALOG_H

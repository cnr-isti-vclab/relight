#ifndef ALIGNDIALOG_H
#define ALIGNDIALOG_H

#include <QDialog>
#include <QDir>

#include <vector>

namespace Ui {
	class AlignDialog;
}

class QGraphicsScene;
class QGraphicsRectItem;

class AlignDialog : public QDialog {
	Q_OBJECT

public:
	int side = 120;
	int max_shift = 20;
	bool inspecting = false;

	QSize imgsize;
	QDir dir;
	QStringList images;
	std::vector<QImage> samples;
	std::vector<QPointF> offsets;
	size_t reference = 0;


	QGraphicsScene *scene = NULL;
	QGraphicsRectItem *area = NULL;

	explicit AlignDialog(QWidget *parent = 0);
	~AlignDialog();

	bool init(QString dir);
	bool loadImage(QString filename);

	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual bool eventFilter(QObject *target, QEvent *event);


public slots:
	void open();
	void align();
	void inspect();
	void save();

private:

	QPoint alignSample(QImage a, QImage b);
	double mutualInformation(QImage a, QImage b, int dx, int dy);

	Ui::AlignDialog *ui;
};

#endif // ALIGNDIALOG_H

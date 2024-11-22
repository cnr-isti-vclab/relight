#ifndef SCALEFRAME_H
#define SCALEFRAME_H

#include <QFrame>

class ImageViewer;
class QDoubleSpinBox;
class QLabel;
class QGraphicsPathItem;
class QGraphicsLineItem;
class QGraphicsTextItem;


/* mark a segment and specify  a distance */


class ScaleFrame: public QFrame {
	Q_OBJECT
public:
	ScaleFrame(QWidget *parent = nullptr);
	~ScaleFrame();
	void clear();
	void init();
	void setFirst(QPointF p);
	void setSecond(QPointF p);
	void setLengthLabel(double length);

public slots:
	void click(QPoint p);
	void startPicking();
	void cancelPicking();
	void removeScale();
	void setLength(double length); //called when user modified the value in the spinbox.

signals:
	void pixelSizeChanged();


protected:
	enum Measuring { NOTHING = 0, FIRST_POINT = 1, SECOND_POINT = 2, DONE = 3 };
	Measuring status;

	QDoubleSpinBox *scale = nullptr;
	QLabel *pixelSize = nullptr;
	ImageViewer *viewer = nullptr;

	QGraphicsPathItem *first = nullptr;
	QGraphicsPathItem *second = nullptr;
	QGraphicsLineItem *line = nullptr;
	QGraphicsTextItem *text = nullptr;
};

#endif // SCALEFRAME_H

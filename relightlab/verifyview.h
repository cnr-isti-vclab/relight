#ifndef VERIFYVIEW_H
#define VERIFYVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>

class Sphere;
class VerifyView;


class ReflectionPoint: public QGraphicsEllipseItem {
public:
	ReflectionPoint(VerifyView *_view, QRectF rect, QGraphicsItem *parent = Q_NULLPTR);
	void init();
	virtual ~ReflectionPoint() {}

protected:
	VerifyView *view;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class AlignPoint: public QGraphicsPathItem {
public:
	AlignPoint(VerifyView *_view, QGraphicsItem *parent = Q_NULLPTR);
	AlignPoint(VerifyView *_view, qreal x, qreal y, QGraphicsItem *parent = Q_NULLPTR);
	void init();
	virtual ~AlignPoint() {}

protected:
	VerifyView *view;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};


class VerifyView: public QGraphicsView {
	Q_OBJECT
public:
	VerifyView(QImage &image, int height, QPointF &pos, QWidget *parent = nullptr);
	virtual void update() = 0;

protected:
	void resizeEvent(QResizeEvent *event);

protected:
	QGraphicsPixmapItem *img_item;
	QGraphicsScene scene;
	QImage &image;
	QPointF &pos;
	int id;
	int height;
};

class ReflectionVerify: public VerifyView {
	Q_OBJECT
public:
	double lightSize = 10.0;

	ReflectionVerify(QImage &image,int height,  QPointF &pos, QWidget *parent = nullptr);
	virtual void update();

private:
	ReflectionPoint *reflection;

};

class AlignVerify: public VerifyView {
	Q_OBJECT
public:
	AlignVerify(QImage &image,int height,  QPointF &pos, QWidget *parent = nullptr);
	virtual void update();

private:
	AlignPoint *align;
};

#endif // VERIFYVIEW_H

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
	ReflectionPoint(VerifyView *_view, qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent = Q_NULLPTR);
	void init();
	virtual ~ReflectionPoint() {}

protected:
	VerifyView *view;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};


class VerifyView: public QGraphicsView {
	Q_OBJECT
public:
	double lightSize = 10.0;
//id is just image number
	VerifyView(QImage &image, QPointF &pos, int height, QWidget *parent = nullptr);

public slots:
	void updateReflection();

protected:
	void resizeEvent(QResizeEvent *event);

private:
	QGraphicsPixmapItem *img_item;
	ReflectionPoint *reflection;
	QGraphicsScene scene;
	QImage &image;
	QPointF &pos;
	int id;
	int height;
};
#endif // VERIFYVIEW_H

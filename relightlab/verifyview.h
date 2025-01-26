#ifndef VERIFYVIEW_H
#define VERIFYVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>

class Sphere;
class VerifyView;

class VerifyMarker: public QGraphicsItem {
public:
	enum Marker { REFLECTION, ALIGN };
	Marker marker;

	bool active = true;

	float radius = 0.0f;
	QPointF center;

	VerifyView *view = nullptr;

	VerifyMarker(VerifyView *_view, Marker _marker, QGraphicsItem *parent = Q_NULLPTR);
	QPainterPath shape() const override;
	QRectF boundingRect() const override;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

class VerifyView: public QGraphicsView {
	Q_OBJECT
public:
	VerifyMarker::Marker marker;

	VerifyView(QImage &image, int height, QPointF &pos, VerifyMarker::Marker _marker, QWidget *parent = nullptr);
	void update();

protected:
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
protected:
	VerifyMarker *marker_item = nullptr;
	QGraphicsPixmapItem *img_item = nullptr;
	QGraphicsScene scene ;
	QImage &image;
	QPointF center;
	QPointF &pos;
	int id;
	int height;
};

#endif // VERIFYVIEW_H

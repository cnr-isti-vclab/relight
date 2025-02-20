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
	bool selected = false;

	VerifyView(QImage &image, int height, QPointF &pos, VerifyMarker::Marker _marker, QWidget *parent = nullptr);
	void setImageNumber(int n);
	void update(); //update pos from marker
	void set(); //update marker from pos
	void setSelected(bool isSelected);

signals:
	void clicked(QMouseEvent *event);

protected:
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override {
		emit clicked(event);
	}
	VerifyMarker *marker_item = nullptr;
	QGraphicsPixmapItem *img_item = nullptr;
	QGraphicsSimpleTextItem *img_number = nullptr;
	QGraphicsRectItem *border = nullptr;
	QGraphicsScene scene ;
	QImage &image;
	QPointF center;
	QPointF &pos;
	int id;
	int height;
};

#endif // VERIFYVIEW_H

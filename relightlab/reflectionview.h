#ifndef REFLECTIONVIEW_H
#define REFLECTIONVIEW_H


#include <QGraphicsView>
#include <QGraphicsScene>
#include <vector>

#include "../src/crop.h"

class Sphere;
class QGraphicsEllipseItem;
class QGraphicsRectItem;

class MarkerOverview: public QGraphicsView {
	Q_OBJECT
public:
	MarkerOverview(int height, QWidget *parent = nullptr);
	void init(); //load pixmaps and set size.
	void setImage(const QPixmap &pix); //set an arbitrary image directly.
	virtual void update() = 0;


protected:
	void resizeEvent(QResizeEvent *event);

	int height;
	QGraphicsScene scene;
	QGraphicsPixmapItem *img_item = nullptr;
};

class SphereOverview: public MarkerOverview {
	Q_OBJECT
public:
	SphereOverview(QPointF center, double radius, int height, QWidget *parent = nullptr);
	virtual void update();

	QPointF center;
	double radius;

private:
	QGraphicsEllipseItem *ellipse = nullptr;
};

class AlignOverview: public MarkerOverview {
	Q_OBJECT
public:
	AlignOverview(QRectF rect, int height, QWidget *parent = nullptr);
	virtual void update();
	QRectF rect;
	float angle = 0.0f;
protected:
	QGraphicsRectItem *item = nullptr;
};

class ReflectionOverview: public QGraphicsView {
	Q_OBJECT
public:
	double lightRadius = 2.0;

	ReflectionOverview(Sphere *sphere, int height, QWidget *parent = nullptr);
	~ReflectionOverview();
	void init(); //call this when the sphere changes position
	void update(); //this when detecting highlights.

	//public slots:

protected:
	void resizeEvent(QResizeEvent *event);

private:
	int height;
	Sphere *sphere;
	QGraphicsScene scene;
	QGraphicsPixmapItem *img_item = nullptr;
	QGraphicsEllipseItem *area = nullptr;
	std::vector<QGraphicsEllipseItem *> lights;

};

class ZoomOverview: public MarkerOverview {
	Q_OBJECT
public:
	ZoomOverview(Crop _crop, int height, QWidget *parent = nullptr);
	Crop crop;
	QGraphicsRectItem *item = nullptr;
	virtual void update();

public slots:
	void setCrop(Crop _crop) {
		crop = _crop;
		update();
	}
	void showNormalmap(const QString &path);
};

// Small thumbnail showing the 4 reference points for plane flattening.
class PlaneOverview: public MarkerOverview {
	Q_OBJECT
public:
	PlaneOverview(int height, QWidget *parent = nullptr);
	void setPoints(const std::vector<QPointF> &pts);
	virtual void update() override;

private:
	std::vector<QGraphicsEllipseItem *> dot_items;
	std::vector<QPointF> points;
};

#endif // REFLECTIONVIEW_H

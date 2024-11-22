#ifndef REFLECTIONVIEW_H
#define REFLECTIONVIEW_H


#include <QGraphicsView>
#include <QGraphicsScene>
#include <vector>

class Sphere;
class QGraphicsEllipseItem;

class PositionView: public QGraphicsView {
	Q_OBJECT
public:
	PositionView(Sphere *sphere, int height, QWidget *parent = nullptr);
	void update();
protected:
	void resizeEvent(QResizeEvent *event);
private:
	int height;
	Sphere *sphere;
	QGraphicsScene scene;
	QGraphicsPixmapItem *img_item = nullptr;
	QGraphicsEllipseItem *ellipse = nullptr;
};

class ReflectionView: public QGraphicsView {
	Q_OBJECT
public:
	double lightRadius = 2.0;

	ReflectionView(Sphere *sphere, int height, QWidget *parent = nullptr);
	~ReflectionView();
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


#endif // REFLECTIONVIEW_H

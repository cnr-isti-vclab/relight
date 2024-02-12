#ifndef REFLECTIONVIEW_H
#define REFLECTIONVIEW_H


#include <QGraphicsView>
#include <QGraphicsScene>

class Sphere;

class ReflectionView: public QGraphicsView {
	Q_OBJECT
public:
	double lightSize = 10.0;

	ReflectionView(Sphere *sphere, QWidget *parent = nullptr);
	void update();
//public slots:


private:
	Sphere *sphere;
	QGraphicsScene scene;
	QGraphicsPixmapItem *img_item;

};


#endif // REFLECTIONVIEW_H

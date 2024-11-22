#ifndef DIRECTIONSVIEW_H
#define DIRECTIONSVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>

class Dome;

class DirectionsView: public QGraphicsView {
	Q_OBJECT
public:
	double lightSize = 10.0;

	DirectionsView(QWidget *parent = nullptr);
	void initFromDome(Dome &dome);

public slots:
	void highlight(int n);
	void clear();

private:
	QGraphicsScene scene;
};

#endif // DIRECTIONSVIEW_H

#include "ball.h"
#include <QGraphicsEllipseItem>
#include <QPen>

#include <math.h>
#include <assert.h>
#include "mainwindow.h"

#include <iostream>

using namespace std;

BorderPoint::~BorderPoint() {}

QVariant BorderPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if (change == ItemPositionChange  && scene() || change == ItemScenePositionHasChanged) {
		RTIScene *s = (RTIScene *)scene();
		emit s->borderPointMoved();
	}
	return QGraphicsItem::itemChange(change, value);
}

Ball::Ball() {}

bool Ball::fit(QSize imgsize) {
	vector<QPointF> centers;
	for(QGraphicsEllipseItem *item: border) {
		centers.push_back( item->rect().center() + QPointF(item->x(), item->y()));
		cout << "center: " << item->x() << " " << item->y() << endl;
		QPointF p = item->rect().center();
		cout << "rect center: " << p.x() << " " << p.y() << endl << endl;
	}


	if(centers.size() < 3)
		return false;

	float n = centers.size();
	float sx = 0, sy = 0, sxy = 0, sx2 = 0, sy2 = 0, sx3 = 0, sy3 = 0, sx2y = 0, sxy2 = 0;
	for(size_t k = 0; k < centers.size(); k++) {
		float x = centers[k].x();
		float y = centers[k].y();
		sx += x;
		sy += y;
		sxy += x*y;
		sx2 += x*x;
		sy2 += y*y;
		sx3 += x*x*x;
		sy3 += y*y*y;
		sx2y += x*x*y;
		sxy2 += x*y*y;
	}

	float d11 = n*sxy - sx*sy;
	float d20 = n*sx2 - sx*sx;
	float d02 = n*sy2 - sy*sy;
	float d30 = n*sx3 - sx2*sx;
	float d03 = n*sy3 - sy2*sy;
	float d21 = n*sx2y - sx2*sy;
	float d12 = n*sxy2 - sx*sy2;

	float a = ((d30 + d12)*d02 - (d03 + d21)*d11)/(2*(d02*d20 - d11*d11));
	float b = ((d03 + d21)*d20 - (d30 + d12)*d11)/(2*(d20*d02 - d11*d11));

	float c = (sx2 +sy2  -2*a*sx - 2*b*sy)/n;
	float r = sqrt(c + a*a + b*b);

	center = QPointF(a, b);
	radius = r;

	//float max_angle = (52.0/180.0)*M_PI; //60 deg  respect to the vertical
	float max_angle = (50.0/180.0)*M_PI; //slightly over 45. hoping not to spot reflexes
	smallradius = radius*sin(max_angle);

	int startx = (int)floor(center.x() - smallradius);
	int endx = ceil(center.x() + smallradius+1);

	int starty = (int)floor(center.y() - smallradius);
	int endy = (int)ceil(center.y() + smallradius+1);

	inner = QRect(startx, starty, endx - startx, endy - starty);

//	sphere =QImage(endx - startx, endy - starty, QImage::Format_ARGB32);
//	sphere.fill(0);

	if(startx < 0 || starty < 0 || endx >= imgsize.width() || endy >= imgsize.height()) {
		fitted = false;
		return false;
	}
	fitted = true;
	return true;
}


void Ball::findHighlight(QImage img, int n) {
	sphere = QImage(inner.width(), inner.height(), QImage::Format_ARGB32);
	uchar threshold = 220;

	QPointF bari(0, 0); //in image coords
	int count = 0;
	while(count < 10 && threshold > 100) {
		bari = QPointF(0, 0);
		count = 0;
		for(int y = inner.top(); y < inner.bottom(); y++) {
			for(int x = inner.left(); x < inner.right(); x++) {

				float X = x - inner.left(); //coordinates in outer rect
				float Y = y - inner.top();

				float cx = X - smallradius;
				float cy = Y - smallradius;
				float d = sqrt(cx*cx + cy*cy);
				if(d > smallradius) continue;

				QRgb c = img.pixel(x, y);
				int g = qGray(c);

				int mg = qGray(sphere.pixel(X, Y));
				assert(X < sphere.width());
				assert(Y < sphere.height());
				if(g > mg) sphere.setPixel(X, Y, qRgb(g, g, g));

				if(g < threshold) continue;

				bari += QPointF(x, y);
				count++;

			}
		}
		if(count > 0) {
			bari.rx() /= count;
			bari.ry() /= count;
		}
		threshold -= 10;
	}
	cout << "Bari: " << bari.x() << " " << bari.y() << endl;
	lights[n] = bari;
	valid[n] = !(bari == QPointF(0, 0));

}


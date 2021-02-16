#include "ball.h"

#include <QGraphicsEllipseItem>
#include <QPen>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

#include <math.h>
#include <assert.h>
#include "mainwindow.h"
#include "project.h"
#include <iostream>

using namespace std;

BorderPoint::~BorderPoint() {}

QVariant BorderPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		RTIScene *s = (RTIScene *)scene();
		emit s->borderPointMoved();
	}
	return QGraphicsItem::itemChange(change, value);
}


HighlightPoint::~HighlightPoint() {}

QVariant HighlightPoint::itemChange(GraphicsItemChange change, const QVariant &value)	{
	if ((change == ItemPositionChange  && scene()) || change == ItemScenePositionHasChanged) {
		RTIScene *s = (RTIScene *)scene();
		emit s->highlightMoved();
	}
	return QGraphicsItem::itemChange(change, value);
}


Ball::Ball() {}

Ball::~Ball() {
	if(circle)
		delete circle;
	if(smallcircle)
		delete smallcircle;
	if(highlight)
		delete highlight;
	for(auto b: border)
		delete b;
	if(sphere)
		delete sphere;
}

void Ball::resetHighlight(size_t n) {
	lights[n] = QPointF();
	directions[n] = Vector3f();
}

void Ball::setActive(bool _active) {
	active = _active;
	QPen pen;
	pen.setCosmetic(true);
	pen.setColor(active? Qt::yellow : Qt::lightGray);
	if(!active) {

	}

	if(circle) circle->setPen(pen);

	QVector<qreal> dashes;
	dashes << 4 << 4;
	pen.setDashPattern(dashes);

	if(smallcircle) smallcircle->setPen(pen);
	for(auto p: border)
		p->setPen(pen);

}

bool Ball::fit(QSize imgsize) {
	vector<QPointF> centers;
	for(QGraphicsEllipseItem *item: border) {
		centers.push_back( item->rect().center() + QPointF(item->x(), item->y()));
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
	//TODO hack!
	if(n == 0) {
		sphereImg = QImage(inner.width(), inner.height(), QImage::Format_ARGB32);
		sphereImg.fill(0);
	}
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

				int mg = qGray(sphereImg.pixel(X, Y));
				assert(X < sphereImg.width());
				assert(Y < sphereImg.height());
				if(g > mg) sphereImg.setPixel(X, Y, qRgb(g, g, g));

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

	//find biggest spot by removing outliers.
	int radius = ceil(0.5*inner.width());
	while(radius > ceil(0.02*inner.width())) {
		QPointF newbari(0, 0); //in image coords
		double weight = 0.0;
		int starty = std::max(inner.top(),    int(floor(bari.ry())) - radius);
		int endy   = std::min(inner.bottom(), int( ceil(bari.ry())) + radius);
		int startx = std::max(inner.left(),   int(floor(bari.rx())) - radius);
		int endx   = std::min(inner.right(),  int( ceil(bari.rx())) + radius);
		for(int y = starty; y < endy; y++) {
			for(int x = startx; x < endx; x++) {

				float X = x - inner.left(); //coordinates in outer rect
				float Y = y - inner.top();

				float cx = X - smallradius;
				float cy = Y - smallradius;
				float d = sqrt(cx*cx + cy*cy);
				if(d > smallradius) continue;

				QRgb c = img.pixel(x, y);
				int g = qGray(c);

				int mg = qGray(sphereImg.pixel(X, Y));
				assert(X < sphereImg.width());
				assert(Y < sphereImg.height());
				if(g > mg) sphereImg.setPixel(X, Y, qRgb(g, g, g));

				if(g < threshold) continue;

				newbari += QPointF(x*double(g), y*double(g));
				weight += g;
			}
		}
		if(!weight) break;
		bari = newbari/weight;
		radius *= 0.5;
	}

	if(!sphere) {
		sphere = new QGraphicsPixmapItem(QPixmap::fromImage(sphereImg));
		sphere->setZValue(-0.5);
		sphere->setPos(inner.topLeft());
	} else {
		sphere->setPixmap(QPixmap::fromImage(sphereImg));
	}

	lights[n] = bari;
}

void Ball::computeDirections() {

	directions.resize(lights.size());
	for(size_t i = 0; i < lights.size(); i++) {
		if(lights[i].isNull()) {
			directions[i] = Vector3f(0, 0, 0);
			continue;
		}

		float x = lights[i].x();
		float y = lights[i].y();
		x = (x - inner.left() - smallradius)/radius;
		y = (y - inner.top() - smallradius)/radius;

		float d = sqrt(x*x + y*y);
		float a = asin(d)*2;
		float r = sin(a);
		x *= r/d;
		y *= r/d;
		y *= -1; //cooordinates inverted!

		float z = sqrt(1.0 - x*x - y*y);
		directions[i] = Vector3f(x, y, z);
	}

}

QJsonObject Ball::toJsonObject() {
	QJsonObject ball;
	QJsonArray jcenter = { center.x(), center.y() };
	ball["center"] = jcenter;
	ball["radius"] = radius;
	ball["smallradius"] = smallradius;
	QJsonObject jinner;
	jinner.insert("left", inner.left());
	jinner.insert("top", inner.top());
	jinner.insert("width", inner.width());
	jinner.insert("height", inner.height());
	ball["inner"] = jinner;

	QJsonArray jlights;
	for(QPointF l: lights) {
		QJsonArray jlight = { l.x(), l.y() };
		jlights.append(jlight);
	}
	ball["lights"] = jlights;

	QJsonArray jdirections;
	for(Vector3f l: directions) {
		QJsonArray jdir = { l[0], l[1], l[2] };
		jdirections.append(jdir);
	}
	ball["directions"] = jdirections;

	QJsonArray jborder;
	for(BorderPoint *p: border) {
		QJsonArray b = { p->pos().x(), p->pos().y() };
		jborder.push_back(b);
	}
	ball["border"] = jborder;
	return ball;
}

void Ball::fromJsonObject(QJsonObject obj) {
	auto jcenter = obj["center"].toArray();
	center.setX(jcenter[0].toDouble());
	center.setY(jcenter[1].toDouble());
	fitted = !center.isNull();

	radius = obj["radius"].toDouble();
	smallradius = obj["smallradius"].toDouble();

	auto jinner = obj["inner"].toObject();
	inner.setLeft(jinner["left"].toInt());
	inner.setTop(jinner["top"].toInt());
	inner.setWidth(jinner["width"].toInt());
	inner.setHeight(jinner["height"].toInt());

	lights.clear();
	for(auto jlight: obj["lights"].toArray()) {
		auto j = jlight.toArray();
		lights.push_back(QPointF(j[0].toDouble(), j[1].toDouble()));
	}

	directions.clear();
	for(auto jdir: obj["directions"].toArray()) {
		auto j = jdir.toArray();
		directions.push_back(Vector3f(j[0].toDouble(), j[1].toDouble(), j[2].toDouble()));
	}
	border.clear();
	for(auto jborder: obj["border"].toArray()) {
		auto b = jborder.toArray();
		//TODO cleanp this code replicated in mainwindow.
		QBrush blueBrush(Qt::blue);
		QPen outlinePen(Qt::white);
		outlinePen.setWidth(5);
		outlinePen.setCosmetic(true);


		auto borderPoint = new BorderPoint(-3, -3, 6, 6);
		borderPoint->setPos(b[0].toDouble(), b[1].toDouble());
		borderPoint->setPen(outlinePen);
		borderPoint->setBrush(blueBrush);
		borderPoint->setFlag(QGraphicsItem::ItemIsMovable);
		borderPoint->setFlag(QGraphicsItem::ItemIsSelectable);
		borderPoint->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
		borderPoint->setCursor(Qt::CrossCursor);
		border.push_back(borderPoint);
	}
}

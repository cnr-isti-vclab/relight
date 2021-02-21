#include "measure.h"

#include <QJsonObject>
#include <QJsonArray>

#include <QPen>
#include <QFont>
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QGraphicsScene>


Measure::Measure() {

	QPainterPath path;
	path.moveTo(-8, -8);
	path.lineTo(-3, -3);
	path.moveTo( 8, -8);
	path.lineTo( 3, -3);
	path.moveTo( 8,  8);
	path.lineTo( 3,  3);
	path.moveTo(-8,  8);
	path.lineTo(-3,  3);

	first = new QGraphicsPathItem(path);
	second = new QGraphicsPathItem(path);
	line = new QGraphicsLineItem();
	text = new QGraphicsTextItem();
	QPen pen;
	pen.setColor(Qt::yellow);
	pen.setWidth(1);

	first->setPen(pen);
	second->setPen(pen);
	line->setPen(pen);
	text->setDefaultTextColor(Qt::yellow);
	text->setFont(QFont("Arial", 16));
}

Measure::~Measure() {
	delete first;
	delete second;
	delete line;
	delete text;
}

QJsonObject Measure::toJson() {
	QJsonObject obj;
	obj.insert("unit", "mm");
	obj.insert("length", length);
	QJsonArray jfirst = { first->pos().x(), first->pos().y() };
	obj.insert("first", jfirst);
	QJsonArray jsecond = { second->pos().x(), second->pos().y() };
	obj.insert("second", jsecond);
	return obj;
}

void Measure::fromJson(QJsonObject obj) {
	QString junit = obj["unit"].toString();
	if(junit != "mm") {
		throw QString("Unsupported unit: " + junit);
	}
	length = obj["length"].toDouble();
	QJsonArray jfirst = obj["first"].toArray();
	QJsonArray jsecond = obj["second"].toArray();

	set(QPointF(jfirst[0].toDouble(), jfirst[1].toDouble()),
		QPointF(jsecond[0].toDouble(), jsecond[1].toDouble()),
		length);
}

void Measure::set(QPointF p1, QPointF p2, double measure) {
	first->setPos(p1);
	second->setPos(p2);
	line->setLine(QLineF(first->pos(), second->pos()));
	text->setPos((p1 + p2)/2);
	text->setPlainText(QString::number(measure) + "mm");
	measuring = DONE;
	setVisible(true);
}

void Measure::setVisible(bool visible) {
	first->setVisible(visible);
	second->setVisible(visible);
	line->setVisible(visible);
	text->setVisible(visible);
}

void Measure::setScene(QGraphicsScene *scene) {
	scene->addItem(first);
	scene->addItem(second);
	scene->addItem(line);
	scene->addItem(text);
}
void Measure::setFirstPoint(QPointF p) {
	first->setPos(p);
	first->setVisible(true);
	measuring = SECOND_POINT;
}

void Measure::setSecondPoint(QPointF p) {
	second->setPos(p);
	line->setLine(QLineF(first->pos(), second->pos()));
	setVisible(true);
	measuring = DONE;
}

void Measure::setLength(double d) {
	length = d;
	text->setPlainText(QString::number(d) + "mm");
	text->setPos((first->pos() + second->pos())/2);
}

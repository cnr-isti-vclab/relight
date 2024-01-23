#include "qmeasuremarker.h"
#include "../src/measure.h"
#include "../src/sphere.h"

#include <QInputDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QApplication>
#include <QLabel>
#include <QToolButton>
#include <QLabel>
#include <QStyle>

MeasureMarker::MeasureMarker( Measure *m, QGraphicsView *_view, QWidget *parent):
	Marker(_view, parent), measure(m) {

	label->setText("Measure");

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

	if(!m->first.isNull() && !m->second.isNull()) {
		first->setPos(m->first);
		second->setPos(m->second);
		line->setLine(QLineF(m->first, m->second));
		scene->addItem(first);
		scene->addItem(second);
		scene->addItem(line);

		if(m->length != 0) {
			text->setPlainText(QString::number(m->length) + "mm");
			text->setPos((m->first + m->second)/2);
			scene->addItem(text);
		}
#include <QGraphicsEllipseItem>
#include <QPen>
	}
}

MeasureMarker::~MeasureMarker() {
	delete first;
	delete second;
	delete line;
	delete text;
}


void MeasureMarker::setSelected(bool value) {
	QPen pen = first->pen();
	pen.setWidth(value? 2 : 1);
	first->setPen(pen);
	second->setPen(pen);
	line->setPen(pen);

	Marker::setSelected(value);
}


void MeasureMarker::startMeasure() {
	setEditing(true);
	measuring = FIRST_POINT;
}


void MeasureMarker::onEdit() {
	qDebug("measure editing");
	setSelected(true);
	setEditing(!editing);

	if(!editing)
		return;


	if(measure->first.isNull()) {
		measuring = FIRST_POINT;

	} else {
		askMeasure();
		setEditing(false);
	}
}

void MeasureMarker::askMeasure() {
	bool ok = true;
	double length = QInputDialog::getDouble(this, "Set up scale (px/mm)", "Specify the distance between the two points in mm.", 0.0, 0.0, 1000000.0, 1, &ok);
	if(!ok)
		return;

	measure->length = length;

	if(!length) {
	} else {
		text->setPlainText(QString::number(length) + "mm");
		text->setPos((measure->first + measure->second)/2);
		scene->addItem(text);
	}
	text->setVisible(length != 0);
}

void MeasureMarker::click(QPointF pos) {
	switch(measuring) {
	case FIRST_POINT:
		measure->first = pos;
		first->setPos(pos);
		scene->addItem(first);
		measuring = SECOND_POINT;
		break;

	case SECOND_POINT:
		measure->second = pos;
		second->setPos(pos);
		scene->addItem(second);
		line->setLine(QLineF(measure->first, measure->second));
		scene->addItem(line);

		setEditing(false);
		measuring = DONE;

		onEdit();
		break;
	default:
		break;
	}
}



void MeasureMarker::cancelEditing() {
	setEditing(false);
	if(measuring != DONE) {
		measuring = FIRST_POINT;

		first->setVisible(false);
		second->setVisible(false);
		line->setVisible(false);
		text->setVisible(false);
		measure->first = measure->second = QPointF(0, 0);
	}
}



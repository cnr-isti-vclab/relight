#include "scaleframe.h"
#include "imageview.h"
#include "relightapp.h"
#include "../src/measure.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGraphicsPathItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>

#include <assert.h>

ScaleFrame::ScaleFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);

	QHBoxLayout *controls = new QHBoxLayout;
	content->addLayout(controls);

	QPushButton *take = new QPushButton("Take a new measurement...");
	controls->addWidget(take);

	scale = new QDoubleSpinBox;
	controls->addWidget(scale);

	pixelSize = new QLabel;
	controls->addWidget(pixelSize);

	QPushButton *remove = new QPushButton(QIcon::fromTheme("trash-2"), "");
	controls->addWidget(remove);

	viewer = new ImageViewer;
	viewer->showImage(0);
	content->addWidget(viewer);

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


	connect(take, SIGNAL(clicked()), this, SLOT(startPicking()));
	connect(viewer->view, SIGNAL(clicked(QPoint)), this, SLOT(click(QPoint)));
	connect(scale, SIGNAL(valueChanged(double)), this, SLOT(setLength(double)));
	connect(remove, SIGNAL(clicked()), this, SLOT(removeScale()));
}

ScaleFrame::~ScaleFrame() {
	auto &scene = viewer->scene();

	clear();

	delete first;
	delete second;
	delete line;
	delete text;
}

void ScaleFrame::clear() {
	auto &scene = viewer->scene();

	if(first->scene())
		scene.removeItem(first);
	if(second->scene())
		scene.removeItem(second);
	if(line->scene())
		scene.removeItem(line);
	if(text->scene())
		scene.removeItem(text);
	scale->setValue(0.0);
	status = NOTHING;
}

void ScaleFrame::init() {
	auto &scene = viewer->scene();

	viewer->showImage(0);

	Project &project = qRelightApp->project();
	if(project.measures.size()) {
		Measure *measure = project.measures[0];
		setFirst(measure->first);
		setSecond(measure->second);
		setLengthLabel(measure->length);
		status = DONE;
	}
}

void ScaleFrame::removeScale() {
	QMessageBox::StandardButton proceed = QMessageBox::question(this, "Removing measurement", "You are removing a measurement. Proceed?");
	if(proceed != QMessageBox::Yes)
		return;

	Project &project = qRelightApp->project();
	for(Measure *m: project.measures)
		delete m;
	project.measures.clear();

	clear();
}

void ScaleFrame::startPicking() {
	if(status == DONE) {
		QMessageBox::StandardButton proceed = QMessageBox::question(this, "Taking a new measurement", "You are removing a measurement and taking a new one. Proceed?");
		if(proceed != QMessageBox::Yes)
			return;
	}
	clear();
	status = FIRST_POINT;
	QApplication::setOverrideCursor(Qt::CrossCursor);
}

void ScaleFrame::cancelPicking() {
	clear();
	QApplication::restoreOverrideCursor();
}

void ScaleFrame::click(QPoint q) {
	QPointF p = viewer->view->mapToScene(q);

	if(status == FIRST_POINT) {
		setFirst(p);
		status = SECOND_POINT;
	} else if(status == SECOND_POINT) {
		setSecond(p);
		status = DONE;
		QApplication::restoreOverrideCursor();
	}

}

void ScaleFrame::setFirst(QPointF p) {
	auto &scene = viewer->scene();

	first->setPos(p);
	scene.addItem(first);
}

void ScaleFrame::setSecond(QPointF p) {
	auto &scene = viewer->scene();

	second->setPos(p);
	line->setLine(QLineF(first->pos(), p));
	text->setPos((first->pos() + second->pos())/2);

	scene.addItem(second);
	scene.addItem(line);
}

void ScaleFrame::setLengthLabel(double length) {
	auto &scene = viewer->scene();

	text->setPlainText(QString::number(length) + "mm");
	text->setPos((first->pos() + second->pos())/2);
	if(!text->scene())
		scene.addItem(text);

	scale->setValue(length);
	pixelSize->setText(QString("Pixel size in mm: %1").arg(qRelightApp->project().pixelSize));

}

void ScaleFrame::setLength(double length) {
	auto &scene = viewer->scene();

	text->setPlainText(QString::number(length) + "mm");
	if(!text->scene())
		scene.addItem(text);

	Project &project = qRelightApp->project();
	for(Measure *m: project.measures)
		delete m;
	project.measures.clear();

	Measure *measure = new Measure;
	measure->first = first->pos();
	measure->second = second->pos();
	measure->length = scale->value();
	project.measures.push_back(measure);
	project.computePixelSize();
	pixelSize->setText(QString("Pixel size in mm: %1").arg(qRelightApp->project().pixelSize));
	emit pixelSizeChanged();
}



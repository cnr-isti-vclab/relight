#include "qmarker.h"


#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QVariant>
#include <QApplication>


QMarker::QMarker(QGraphicsView *_view, QWidget *parent):
	QWidget(parent), view(_view) {

	scene = view->scene();

	label = new QLabel();
	edit = new QToolButton();
	edit->setCheckable(true);
	edit->setIcon(QIcon(":/icons/feather/edit.svg"));
	remove = new QToolButton();
	remove->setIcon(QIcon(":/icons/feather/trash-2.svg"));

	QLayout *layout = new QHBoxLayout();
	layout->addWidget(label);
	layout->addWidget(edit);
	layout->addWidget(remove);

	setLayout(layout);

	setBackgroundRole(QPalette::ColorRole::Base);
	setAutoFillBackground(true);

	connect(edit, SIGNAL(clicked()), this, SLOT(onEdit()));
	connect(remove, SIGNAL(clicked()), this, SLOT(onRemove()));
}

QMarker::~QMarker() {}


void QMarker::setSelected(bool value) {
	if(selected == value)
		return;
	if(value == false && editing == true)
		setEditing(false);

	selected = value;
	label->setStyleSheet(selected ? "QLabel { color : #fff; }" : "/* */");
}

void QMarker::setEditing(bool value) {
	if(value == editing)
		return;

	editing = value;
	this->edit->setChecked(editing);
	//this->edit->setStyleSheet(editing ? "background-color: #999; color:#000; " : "/* */");

	if(value)
		QApplication::setOverrideCursor(Qt::CrossCursor);
	else
		QApplication::restoreOverrideCursor();
}



void QMarker::enterEvent(QEvent* event) {
	QWidget::enterEvent(event);
}

void QMarker::leaveEvent(QEvent* event) {
	QWidget::leaveEvent(event);
}



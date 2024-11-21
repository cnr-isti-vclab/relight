#include "qmarker.h"


#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QVariant>
#include <QApplication>


Marker::Marker(QGraphicsView *_view, QWidget *parent):
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

Marker::~Marker() {}


void Marker::setSelected(bool value) {
	if(value == false)
		setEditing(false);

	if(selected == value)
		return;

	selected = value;
	label->setStyleSheet(selected ? "QLabel { color : #fff; }" : "/* */");
}

void Marker::setEditing(bool value) {
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


#if QT_VERSION >= 0x060000
void Marker::enterEvent(QEnterEvent *event) {
#else
void Marker::enterEvent(QEvent *event) {
#endif
	QWidget::enterEvent(event);
}

void Marker::leaveEvent(QEvent* event) {
	QWidget::leaveEvent(event);
}



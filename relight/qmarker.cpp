#include "qmarker.h"


#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QVariant>

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

void QMarker::setSelected(bool value) {
	if(selected == value)
		return;
	if(value == false && editing == true)
		setEditing(false);

	selected = value;
	label->setStyleSheet(selected ? "QLabel { color : #fff; }" : "/* */");
}

void QMarker::setEditing(bool value) {
	editing = value;
	this->edit->setChecked(editing);
	//this->edit->setStyleSheet(editing ? "background-color: #999; color:#000; " : "/* */");
}

	/*
   call_once(initPalettes, [self = this]()
   {
	  _DefaultBackground = self->palette();

	  _HighBackground = self->palette();
	  _HighBackground.setColor(QPalette::ColorRole::Base, _HighBackground.color(QPalette::Highlight).lighter(210));

	  _SelectedBackground = self->palette();
	  _SelectedBackground.setColor(QPalette::ColorRole::Base, _SelectedBackground.color(QPalette::Highlight));
   }); */

QMarker::~QMarker() {}

void QMarker::enterEvent(QEvent* event) {
	QWidget::enterEvent(event);
	HighlightBackground(true);
}

void QMarker::leaveEvent(QEvent* event) {
	QWidget::leaveEvent(event);
	HighlightBackground(false);
}

void QMarker::HighlightBackground(bool high) {
	/*
   if (high)
   {
	  if (_HighlightedItems.size() > 0)
	  {
		 _HighlightedItems.back()->setPalette(_HighlightedItems.back()->_selected ? _SelectedBackground : _DefaultBackground);
		 _HighlightedItems.back()->update();
	  }

	  _HighlightedItems.push_back(this);
	  setPalette(_HighBackground);
   }
   else
   {
	  setPalette(_selected ? _SelectedBackground : _DefaultBackground);
	  auto pos = std::find(_HighlightedItems.begin(), _HighlightedItems.end(), this);
	  if (pos != _HighlightedItems.end())
		 _HighlightedItems.erase(pos);

	  if (_HighlightedItems.size() > 0)
	  {
		 _HighlightedItems.back()->setPalette(_HighBackground);
		 _HighlightedItems.back()->update();
	  }
   }
   update(); */
}


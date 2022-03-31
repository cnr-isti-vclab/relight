#include "qmarkerlist.h"
#include "qmarker.h"

#include <QMouseEvent>
#include <QBoxLayout>
using namespace std;


ListMarker::ListMarker(QWidget* parent): QFrame(parent) {
	setBackgroundRole(QPalette::ColorRole::Base);
	setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	setMinimumSize(QSize(20, 20));
	//setFrameStyle(QFrame::Box); //not stretching

	_layout = new QBoxLayout(QBoxLayout::TopToBottom);
	_layout->setSizeConstraint(QLayout::SetMinimumSize);
	_layout->setContentsMargins(0, 0, 0, 0);
	_layout->setSpacing(0);
	setLayout(_layout);

	_layout->addStretch(0);
}

void ListMarker::setSelected(Marker *target) {
	for(auto marker: getItems())
		marker->setSelected(marker == target);
}

void ListMarker::clear() {
	for (auto child : getItems())
		delete child;
}

Marker* ListMarker::addItem(Marker* item, int index)
{
	if (!item)
		return nullptr;

	if (!_layout)
		return nullptr;

	if (index < 0 || index >= _layout->count())
		index = _layout->count() - 1;

	_layout->insertWidget(index, item);

	setMinimumWidth(max(minimumWidth(), item->sizeHint().width() + contentsMargins().left() + contentsMargins().right()));

//	propagateMinimumDimension();
	return item;
}

void ListMarker::removeItem(Marker* item) {
	item->setParent(nullptr);
	_layout->removeWidget(item);
	_layout->update();
}

vector<Marker*> ListMarker::getItems(bool selected) const {
	vector<Marker*> widgets;

	const int c = _layout->count();
	for (int i = 0; i < c; ++i)
		if (auto w = dynamic_cast<Marker*>(_layout->itemAt(i)->widget()))
			 if (!selected || w->selected)
				widgets.push_back(w);

	return widgets;
}

std::vector<Marker*> ListMarker::getSelectedItems() const {
	return getItems(true);
}


void ListMarker::mouseReleaseEvent(QMouseEvent* event) {
	Marker* widget = itemAt(event->pos());
	if (!widget)
		return;

	setSelected(widget);
}

Marker* ListMarker::itemAt(const QPoint& pt) const {
	for (auto child = childAt(pt); child; child = child->parentWidget())
		if (auto widget = dynamic_cast<Marker*>(child))
			return widget;

	return nullptr;
}


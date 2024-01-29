#include "canvas.h"

#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>
#include <qmath.h>

#include <iostream>
using namespace std;

Canvas::Canvas(QWidget *parent): QGraphicsView(parent) {
	viewport()->installEventFilter(this);
	setMouseTracking(true);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setInteractive(true);

	_modifiers = Qt::NoModifier; //Qt::ControlModifier;
	_zoom_factor_base = 1.0015;
}

void Canvas::gentle_zoom(double factor) {
	scale(factor, factor);
	centerOn(target_scene_pos);
	QPointF delta_viewport_pos = target_viewport_pos - QPointF(viewport()->width() / 2.0,
															   viewport()->height() / 2.0);
	QPointF viewport_center = mapFromScene(target_scene_pos) - delta_viewport_pos;
	centerOn(mapToScene(viewport_center.toPoint()));
	emit zoomed();
}

void Canvas::set_modifiers(Qt::KeyboardModifiers modifiers) {
	_modifiers = modifiers;

}

void Canvas::set_zoom_factor_base(double value) {
	_zoom_factor_base = value;
}

bool Canvas::eventFilter(QObject *object, QEvent *event) {
	QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

	if (event->type() == QEvent::MouseMove) {
		QPointF delta = target_viewport_pos - mouse_event->pos();
		if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
			target_viewport_pos = mouse_event->pos();
			target_scene_pos = mapToScene(mouse_event->pos());
		}

	} else if (event->type() == QEvent::Wheel) {
		QWheelEvent* wheel_event = static_cast<QWheelEvent*>(event);
		if (QApplication::keyboardModifiers() == _modifiers) {
			if (wheel_event->angleDelta().y() != 0) {
				double angle = wheel_event->angleDelta().y();
				double factor = qPow(_zoom_factor_base, angle);
				gentle_zoom(factor);
				return true;
			}
		}

	} else if(event->type() == QEvent::MouseButtonPress) {
		pressPosition = mouse_event->pos();

	} else if(event->type() == QEvent::MouseButtonRelease) {
		QPoint p = mouse_event->pos();
		pressPosition -= p;
		if(pressPosition.manhattanLength() < click_threshold)
			emit clicked(p);

	} else if(event->type() == QEvent::MouseButtonDblClick) {
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		QPoint p = mouse_event->pos();

		emit dblClicked(p);
		return true;
	}

	Q_UNUSED(object)
	return false;
}

void Canvas::zoomIn() {
	gentle_zoom(1.19706);
}

void Canvas::zoomOut() {
	gentle_zoom(0.835383);
}


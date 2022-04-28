#include "graphics_view_zoom.h"
#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>
#include <qmath.h>

#include <iostream>
using namespace std;

Graphics_view_zoom::Graphics_view_zoom(QGraphicsView* view)
	: QObject(view), _view(view) {
	_view->viewport()->installEventFilter(this);
	_view->setMouseTracking(true);
	_modifiers = Qt::NoModifier; //Qt::ControlModifier;
	_zoom_factor_base = 1.0015;
}

void Graphics_view_zoom::gentle_zoom(double factor) {
	_view->scale(factor, factor);
	_view->centerOn(target_scene_pos);
	QPointF delta_viewport_pos = target_viewport_pos - QPointF(_view->viewport()->width() / 2.0,
															   _view->viewport()->height() / 2.0);
	QPointF viewport_center = _view->mapFromScene(target_scene_pos) - delta_viewport_pos;
	_view->centerOn(_view->mapToScene(viewport_center.toPoint()));
	emit zoomed();
}

void Graphics_view_zoom::set_modifiers(Qt::KeyboardModifiers modifiers) {
	_modifiers = modifiers;

}

void Graphics_view_zoom::set_zoom_factor_base(double value) {
	_zoom_factor_base = value;
}

bool Graphics_view_zoom::eventFilter(QObject *object, QEvent *event) {
	QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);	
	
	if (event->type() == QEvent::MouseMove) {
		QPointF delta = target_viewport_pos - mouse_event->pos();
		if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
			target_viewport_pos = mouse_event->pos();
			target_scene_pos = _view->mapToScene(mouse_event->pos());
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

void Graphics_view_zoom::zoomIn() {
	gentle_zoom(1.19706);
}

void Graphics_view_zoom::zoomOut() {
	gentle_zoom(0.835383);
}


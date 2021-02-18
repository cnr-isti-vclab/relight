#ifndef GRAPHICS_VIEW_ZOOM_H
#define GRAPHICS_VIEW_ZOOM_H

#include <QObject>
#include <QGraphicsView>


class Graphics_view_zoom : public QObject {
	Q_OBJECT
public:
	Graphics_view_zoom(QGraphicsView* view);
	void gentle_zoom(double factor);
	void set_modifiers(Qt::KeyboardModifiers modifiers);
	void set_zoom_factor_base(double value);

private:
	QGraphicsView* _view;
	Qt::KeyboardModifiers _modifiers;
	double _zoom_factor_base;
	QPointF target_scene_pos, target_viewport_pos;
	QPoint pressPosition;
	double click_threshold = 2;
	bool eventFilter(QObject* object, QEvent* event);

public slots:
	void zoomIn();
	void zoomOut();

signals:
	void zoomed();
	void clicked(QPoint);
	void dblClicked(QPoint);
};


#endif // GRAPHICS_VIEW_ZOOM_H

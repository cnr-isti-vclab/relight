#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>

class Canvas : public QGraphicsView {
	Q_OBJECT
public:
	Canvas(QWidget *parent = nullptr);
	void gentle_zoom(double factor);
	void set_modifiers(Qt::KeyboardModifiers modifiers);
	void set_zoom_factor_base(double value);

private:
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

#endif // CANVAS_H

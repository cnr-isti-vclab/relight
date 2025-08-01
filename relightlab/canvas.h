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
	void setCursor(Qt::CursorShape cursor);

	double min_zoom_scale = 0.8f;
	double min_scale = 0.0f;
	double max_scale = 4.0f;
	void fitInView(const QRectF &rect, Qt::AspectRatioMode aspectRadioMode = Qt::KeepAspectRatio);

protected:
	void resizeEvent(QResizeEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	Qt::KeyboardModifiers _modifiers;
	double _zoom_factor_base;
	QPointF target_scene_pos, target_viewport_pos;
	QPoint pressPosition;
	double click_threshold = 2;
	Qt::CursorShape view_cursor = Qt::OpenHandCursor;

	//this stuff is needed to call fitinview before the image is properly resized.
	bool needs_fit = false;
	bool resized = false;
	QRectF rect_fit;
	Qt::AspectRatioMode aspect_fit;

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

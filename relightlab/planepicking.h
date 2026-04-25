#ifndef PLANEPICKING_H
#define PLANEPICKING_H

#include "imageview.h"

#include <QDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QRect>
#include <QShowEvent>
#include <vector>

class PlanePicking;
class QLabel;
class QDialogButtonBox;

// ── Draggable green dot ──────────────────────────────────────────────────────

class PlanePoint: public QGraphicsEllipseItem {
public:
	PlanePoint(PlanePicking *_picker, QPointF pos, QGraphicsItem *parent = nullptr);

protected:
	PlanePicking *picker;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

// ── Image viewer that lets the user place exactly 4 points ──────────────────

class PlanePicking: public ImageViewer {
	Q_OBJECT
public:
	PlanePicking(QWidget *parent = nullptr);

	void setPoints(const std::vector<QPointF> &pts);
	std::vector<QPointF> getPoints() const;
	void setCrop(const QRect &crop);
	void clear();

	// Called by PlanePoint when it is dragged.
	void onPointMoved();

signals:
	void pointsChanged(int count);

public slots:
	void click(QPoint p);

protected:
	void keyReleaseEvent(QKeyEvent *e) override;

private:
	void addPoint(QPointF pos);

	QRect crop_rect;
	QGraphicsRectItem *crop_item = nullptr;
	std::vector<PlanePoint *> points;
};

// ── Full-screen dialog wrapping PlanePicking ─────────────────────────────────

class PlanePickingDialog: public QDialog {
	Q_OBJECT
public:
	PlanePickingDialog(QWidget *parent = nullptr);

	void setPoints(const std::vector<QPointF> &pts);
	std::vector<QPointF> getPoints() const;
	void setCrop(const QRect &crop);

protected:
	void showEvent(QShowEvent *e) override;

private slots:
	void onPointsChanged(int count);

private:
	PlanePicking *picking = nullptr;
	QDialogButtonBox *button_box = nullptr;
	QLabel *status_label = nullptr;
};

#endif // PLANEPICKING_H

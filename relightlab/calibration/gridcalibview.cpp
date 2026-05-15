#include "gridcalibview.h"

#include <QVBoxLayout>
#include <QPen>

GridCalibView::GridCalibView(QWidget *parent): QFrame(parent) {
	scene_  = new QGraphicsScene(this);
	view    = new Canvas(this);
	view->setScene(scene_);
	view->setBackgroundBrush(QBrush(Qt::black));
	view->min_zoom_scale = 0.05;
	view->max_scale      = 32.0;

	QVBoxLayout *lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->addWidget(view);
}

void GridCalibView::setImage(const QPixmap &px) {
	clearCorners();
	if (!pixItem) {
		pixItem = scene_->addPixmap(px);
		pixItem->setZValue(0);
	} else {
		pixItem->setPixmap(px);
	}
	scene_->setSceneRect(pixItem->boundingRect());
}

void GridCalibView::setCorners(const std::vector<QPointF> &pts, int cols) {
	clearCorners();
	if (!pixItem || pts.empty()) return;

	// Colour-cycle row by row so each row has a distinct hue
	const int rows = (cols > 0) ? (int)((pts.size() + cols - 1) / cols) : 1;

	for (int i = 0; i < (int)pts.size(); ++i) {
		int row = (cols > 0) ? (i / cols) : 0;
		// Hue cycles from 0° (red) to 300° (magenta) across rows
		qreal hue = (rows > 1) ? (300.0 * row / (rows - 1)) : 0.0;
		QColor fill = QColor::fromHsvF(hue / 360.0, 0.9, 1.0, 0.85);
		QColor edge(0, 0, 0, 180);

		const QPointF &p = pts[i];
		QGraphicsEllipseItem *dot = scene_->addEllipse(
			p.x() - DOT_R, p.y() - DOT_R, DOT_R * 2, DOT_R * 2,
			QPen(edge, 1.5), QBrush(fill));
		dot->setZValue(1);
		dotItems.push_back(dot);
	}
}

void GridCalibView::clearCorners() {
	for (auto *item : dotItems)
		scene_->removeItem(item);
	dotItems.clear();
}

void GridCalibView::fit() {
	if (pixItem)
		view->fitInView(pixItem->boundingRect(), Qt::KeepAspectRatio);
}

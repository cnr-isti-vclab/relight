#include "planepicking.h"
#include "relightapp.h"
#include "../src/project.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPen>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

// ── PlanePoint ───────────────────────────────────────────────────────────────

PlanePoint::PlanePoint(PlanePicking *_picker, QPointF pos, QGraphicsItem *parent)
	: QGraphicsEllipseItem(-5, -5, 10, 10, parent), picker(_picker)
{
	setPos(pos);
	QPen pen(Qt::green);
	pen.setWidth(2);
	pen.setCosmetic(true);
	setPen(pen);
	setBrush(QBrush(QColor(0, 200, 0, 160)));
	setCursor(Qt::SizeAllCursor);
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
}

QVariant PlanePoint::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if(change == ItemScenePositionHasChanged || change == ItemPositionHasChanged)
		picker->onPointMoved();
	return QGraphicsEllipseItem::itemChange(change, value);
}

// ── PlanePicking ─────────────────────────────────────────────────────────────

PlanePicking::PlanePicking(QWidget *parent): ImageViewer(parent)
{
	connect(view, &Canvas::clicked, this, &PlanePicking::click);
	view->setCursor(Qt::CrossCursor);
	showImage(0);
}

void PlanePicking::setPoints(const std::vector<QPointF> &pts)
{
	clear();
	for(const QPointF &p : pts)
		addPoint(p);
	emit pointsChanged((int)points.size());
}

std::vector<QPointF> PlanePicking::getPoints() const
{
	std::vector<QPointF> result;
	result.reserve(points.size());
	for(const PlanePoint *p : points)
		result.push_back(p->pos());
	return result;
}

void PlanePicking::clear()
{
	for(PlanePoint *p : points) {
		scene().removeItem(p);
		delete p;
	}
	points.clear();
	emit pointsChanged(0);
}

void PlanePicking::setCrop(const QRect &crop)
{
	crop_rect = crop;
	if(!crop_item) {
		QPen pen(Qt::yellow);
		pen.setCosmetic(true);
		QVector<qreal> dashes;
		dashes << 6 << 4;
		pen.setDashPattern(dashes);
		crop_item = new QGraphicsRectItem();
		crop_item->setPen(pen);
		crop_item->setBrush(Qt::NoBrush);
		crop_item->setZValue(10);
		scene().addItem(crop_item);
	}
	crop_item->setRect(QRectF(crop));
}

void PlanePicking::click(QPoint p)
{
	QPointF pos = view->mapToScene(p);
	if(crop_rect.isValid() && !crop_rect.contains(pos.toPoint()))
		return; // outside the active crop region — ignored

	addPoint(pos);
	emit pointsChanged((int)points.size());
}

void PlanePicking::addPoint(QPointF pos)
{
	auto *pt = new PlanePoint(this, pos);
	scene().addItem(pt);
	points.push_back(pt);
}

void PlanePicking::onPointMoved()
{
	emit pointsChanged((int)points.size());
}

void PlanePicking::keyReleaseEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
		// Remove selected points.
		std::vector<PlanePoint *> remaining;
		for(PlanePoint *p : points) {
			if(p->isSelected()) {
				scene().removeItem(p);
				delete p;
			} else {
				remaining.push_back(p);
			}
		}
		points = remaining;
		emit pointsChanged((int)points.size());
	}
	ImageViewer::keyReleaseEvent(e);
}

// ── PlanePickingDialog ────────────────────────────────────────────────────────

PlanePickingDialog::PlanePickingDialog(QWidget *parent): QDialog(parent)
{
	setWindowTitle("Select reference points on a flat surface (at least 4)");
	setModal(true);

	QVBoxLayout *layout = new QVBoxLayout(this);

	status_label = new QLabel("Click to place 4 points on a flat (planar) region of the object.");
	layout->addWidget(status_label);

	picking = new PlanePicking;
	layout->addWidget(picking, 1);

	button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	button_box->button(QDialogButtonBox::Ok)->setEnabled(false);
	layout->addWidget(button_box);

	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(picking, &PlanePicking::pointsChanged, this, &PlanePickingDialog::onPointsChanged);

	showMaximized();
}

void PlanePickingDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	picking->fit();
}

void PlanePickingDialog::setCrop(const QRect &crop)
{
	picking->setCrop(crop);
}

void PlanePickingDialog::setPoints(const std::vector<QPointF> &pts)
{
	picking->setPoints(pts);
}

std::vector<QPointF> PlanePickingDialog::getPoints() const
{
	return picking->getPoints();
}

void PlanePickingDialog::onPointsChanged(int count)
{
	button_box->button(QDialogButtonBox::Ok)->setEnabled(count >= 4);
	if(count < 4)
		status_label->setText(QString("Click to place at least 4 points — %1 placed so far. Delete key removes selected points.").arg(count));
	else
		status_label->setText(QString("%1 points placed. Add more for better accuracy, or click OK.").arg(count));
}

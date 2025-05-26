#include "imagecropper.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QList>

#include <math.h>
#include <iostream>
#include <algorithm>
using namespace std;

namespace {
static const QSize WIDGET_MINIMUM_SIZE(300, 300);
}

QRectF CornerMarker::boundingRect() const {
	return QRectF(-8, -8, 16, 16);
}

QVariant CornerMarker::itemChange(GraphicsItemChange change, const QVariant &value) {
	if ((change == ItemPositionChange  && scene()) ) {

		QPointF newPos = value.toPointF();
		QRectF rect = scene()->sceneRect();
		if (!rect.contains(newPos)) {
			// Keep the item inside the scene rect.
			newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
			newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
		}
		return newPos;
	}

	if(!silent && change == ItemScenePositionHasChanged) {
		emit itemChanged();
	}
	return QGraphicsItem::itemChange(change, value);
}

ImageCropper::ImageCropper(QWidget* parent):	ImageView(parent) {
	setMinimumSize(WIDGET_MINIMUM_SIZE);
	setMouseTracking(true);

	QPen border(borderColor);

	scene.addItem(boundary = new QGraphicsRectItem);
	boundary->setPen(border);
	boundary->setBrush(QColor(255, 255, 255, 50));

	for(int i = 0; i < 9; i++) {
		scene.addItem(corners[i] = new CornerMarker);
		corners[i]->setPen(border);
		corners[i]->setRect(-5, -5, 10, 10);
		corners[i]->setBrush(QColor(255, 255, 255, 128));
		connect(corners[i], &CornerMarker::itemChanged, [this, i]() { cornerMoved(i); });
	}
	QList<qreal> pattern;
	pattern << 5 << 5;
	border.setDashPattern(pattern);
	scene.addItem(guide[0] = new QGraphicsLineItem);
	guide[0]->setPen(border);
	scene.addItem(guide[1] = new QGraphicsLineItem);
	guide[1]->setPen(border);
}

ImageCropper::~ImageCropper() {}

void ImageCropper::setImage(const QPixmap& _image) {
	imagePixmap->setPixmap(_image);
	imagePixmap->setPos(imagePixmap->boundingRect().center());
	crop.setRect(QRect(QPoint(0, 0), imagePixmap->pixmap().size()));
	crop.angle = 0.0f;
	update();
}

void ImageCropper::setBackgroundColor(const QColor& _backgroundColor) {
	backgroundColor = _backgroundColor;
	update();
}

void ImageCropper::setCroppingRectBorderColor(const QColor& _borderColor) {
	borderColor = _borderColor;
	update();
}

void ImageCropper::setProportion(const QSizeF& _proportion) {
	if (proportion == _proportion)
		return;
	proportion = _proportion;
	if ( isProportionFixed ) {
		enforceBounds(crop, CursorPositionUndefined);
	}
	emit areaChanged(crop);
}

void ImageCropper::setProportionFixed(const bool _isFixed) {
	isProportionFixed = _isFixed;
}

void ImageCropper::updateCrop() {
	boundary->setRect(crop);
	for(int y = 0; y < 3; y++) {
		for(int x = 0; x < 3; x++) {
			float Y = crop.top() + crop.height() * x/2.0f;
			float X = crop.left() + crop.width() * y/2.0f;

			corners[x + y*3]->silent = true;
			corners[x + y*3]->setPos(X, Y);
			corners[x + y*3]->silent = false;
		}
	}
	float mx = crop.left() + crop.width()/2.0f;
	float my = crop.top() + crop.height()/2.0f;
	guide[0]->setLine(QLineF(crop.left(), my, crop.right(), my));
	guide[1]->setLine(QLineF(mx, crop.top(), mx, crop.bottom()));
}



void ImageCropper::setCrop(Crop _crop) {
	if(!_crop.isValid())
		return;
	crop = _crop;
	enforceBounds(crop, CursorPositionMiddle);
}

void ImageCropper::setRect(QRect rect) {
	crop.setRect(rect);
	enforceBounds(crop, CursorPositionMiddle);
}

void ImageCropper::resetCrop() {
	crop.angle = 0.0f;
	setRect(imagePixmap->pixmap().rect());
}

void ImageCropper::setWidth(int w) {
	if(crop.width() == w) return;
	crop.setWidth(w);
	enforceBounds(crop, CursorPositionRight);
}
void ImageCropper::setHeight(int h) {
	if(crop.height() == h) return;
	crop.setHeight(h);
	enforceBounds(crop, CursorPositionBottom);
}

void ImageCropper::setTop(int t) {
	if(crop.top() == t) return;
	crop.moveTop(t);
	enforceBounds(crop, CursorPositionMiddle);
}
void ImageCropper::setLeft(int l) {
	if(crop.left() == l) return;
	crop.moveLeft(l);
	enforceBounds(crop, CursorPositionMiddle);
}

void ImageCropper::setAngle(float a) {
	crop.angle = a;
	enforceBounds(crop, CursorPositionMiddle);
}

void ImageCropper::maximizeCrop() {
	setRect(getRotatedSize());
}

void ImageCropper::centerCrop() {
	crop.moveCenter(getRotatedSize().center());
	setRect(crop);
}

QRect ImageCropper::getRotatedSize() {
	QTransform transform;
	transform.rotate(crop.angle);
	QRect r(QPoint(0, 0), imagePixmap->pixmap().size() - QSize(1, 1));
	QPolygon rotated = transform.map(QPolygon(r));
	return rotated.boundingRect();
}

void ImageCropper::resizeEvent(QResizeEvent *event) {
	//here we need to set the camera correctly
}

void ImageCropper::cornerMoved(int i) {
	QPoint pos = corners[i]->pos().toPoint();

	/*CursorPositionUndefined = 0,
	CursorPositionMiddle = 1 | 2 | 4 | 8,
	CursorPositionLeft = 1,
	CursorPositionTop = 2,
	CursorPositionRight = 4,
	CursorPositionBottom = 8,
	CursorPositionTopLeft = 1 | 2,
	CursorPositionTopRight = 2 | 4,
	CursorPositionBottomLeft = 8 | 1,
	CursorPositionBottomRight = 8 | 4 */

	/*int positions[9] = {
		1|2, 2, 4|2,
		1,   1|2|4|8, 4,
		1|8, 8, 4|8	}; */
	int positions[9] = {
		1|2, 1, 1|8,
		2,   1|2|4|8, 8,
		4|2, 4, 4|8	};

	CursorPosition _cursorPosition = (CursorPosition) positions[i];

	if (_cursorPosition == CursorPositionMiddle) {
		crop.moveTopLeft(crop.topLeft() + pos - crop.center());
		enforceBounds(crop, CursorPositionMiddle);
	} else {
		if(_cursorPosition & CursorPositionLeft) {
			crop.setLeft(pos.x());
			enforceBounds(crop, CursorPositionLeft);
		}
		if(_cursorPosition & CursorPositionRight) {
			crop.setRight(pos.x());
			enforceBounds(crop, CursorPositionRight);
		}
		if(	_cursorPosition & CursorPositionTop) {
			crop.setTop(pos.y());
			enforceBounds(crop, CursorPositionTop);
		}
		if(_cursorPosition & CursorPositionBottom) {
			crop.setBottom(pos.y());
			enforceBounds(crop, CursorPositionBottom);
		}
	}
}
	/*
void ImageCropper::paintEvent(QPaintEvent* _event) {
	QWidget::paintEvent( _event );

	QRectF croppingRect = imageCroppedRect();
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	{
		QPixmap scaledImage = image.scaled(this->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

		painter.fillRect( this->rect(), backgroundColor );
		painter.save();
		QPointF center = rect().center();
		painter.translate(center);
		painter.rotate(angle);  // angle is a float/double member
		painter.translate(-center);

			// Draw pixmap centered
		QPoint topLeft = center.toPoint() - QPoint(scaledImage.width()/2, scaledImage.height()/2);

		if ( this->size().height() == scaledImage.height() ) {
			painter.drawPixmap( ( this->width() - scaledImage.width() ) / 2, 0, scaledImage );
		} else {
			painter.drawPixmap( 0, ( this->height() - scaledImage.height() ) / 2, scaledImage );
		}
		painter.restore();
	}

	if(show_handle) {
		if (croppingRect.isNull()) {
			const int width = WIDGET_MINIMUM_SIZE.width()/2;
			const int height = WIDGET_MINIMUM_SIZE.height()/2;
			croppingRect.setSize(QSize(width, height));
			float x = (this->width() - croppingRect.width())/2;
			float y = (this->height() - croppingRect.height())/2;
			croppingRect.moveTo(x, y);
			emit areaChanged(croppedRect());
		}

		QPainterPath p;
		p.addRect(croppingRect);
		p.addRect(this->rect());
		painter.setBrush(QBrush(QColor(0,0,0,120)));
		painter.setPen(Qt::transparent);
		painter.drawPath(p);

		painter.setPen(croppingRectBorderColor);
		painter.setBrush(QBrush(Qt::transparent));
		painter.drawRect(croppingRect);


		{
			painter.setBrush(QBrush(croppingRectBorderColor));

			int leftXCoord   = croppingRect.left() - 2;
			int centerXCoord = croppingRect.center().x() - 3;
			int rightXCoord  = croppingRect.right() - 2;

			int topYCoord    = croppingRect.top() - 2;
			int middleYCoord = croppingRect.center().y() - 3;
			int bottomYCoord = croppingRect.bottom() - 2;

			const QSize pointSize(6, 6);

			QVector<QRect> points;
			points

					<< QRect( QPoint(leftXCoord, topYCoord), pointSize )
					<< QRect( QPoint(leftXCoord, middleYCoord), pointSize )
					<< QRect( QPoint(leftXCoord, bottomYCoord), pointSize )

					<< QRect( QPoint(centerXCoord, topYCoord), pointSize )
					<< QRect( QPoint(centerXCoord, middleYCoord), pointSize )
					<< QRect( QPoint(centerXCoord, bottomYCoord), pointSize )

					<< QRect( QPoint(rightXCoord, topYCoord), pointSize )
					<< QRect( QPoint(rightXCoord, middleYCoord), pointSize )
					<< QRect( QPoint(rightXCoord, bottomYCoord), pointSize );

			painter.drawRects( points );
		}

		{
			QPen dashPen(croppingRectBorderColor);
			dashPen.setStyle(Qt::DashLine);
			painter.setPen(dashPen);

			painter.drawLine(
						QPoint(croppingRect.center().x(), croppingRect.top()),
						QPoint(croppingRect.center().x(), croppingRect.bottom()) );

			painter.drawLine(
						QPoint(croppingRect.left(), croppingRect.center().y()),
						QPoint(croppingRect.right(), croppingRect.center().y()) );
		}
	}

	painter.end();
}

void ImageCropper::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		isMousePressed = true;
		startMousePos = event->pos();
		lastStaticCroppingRect = realSizeRect;
	}
	//
	updateCursorIcon(event->pos());
}

void ImageCropper::mouseMoveEvent(QMouseEvent* event)
{
	QPointF mousePos = event->pos();

	if (!isMousePressed) {
		QRectF croppingRect = imageCroppedRect();
		_cursorPosition = cursorPosition(croppingRect, mousePos);
		updateCursorIcon(mousePos);

	} else if (_cursorPosition != CursorPositionUndefined) {
		QPoint mouseDelta;
		mouseDelta.setX( round((mousePos.x() - startMousePos.x())*xScale) );
		mouseDelta.setY( round((mousePos.y() - startMousePos.y())*yScale) );

		QRect target = lastStaticCroppingRect;

		if (_cursorPosition != CursorPositionMiddle) {
			if(_cursorPosition & CursorPositionLeft) {
				target.setLeft(target.left() + mouseDelta.x());
			}
			if(_cursorPosition & CursorPositionRight) {
				target.setRight(target.right() + mouseDelta.x());
			}
			if(	_cursorPosition & CursorPositionTop) {
				target.setTop(target.top() + mouseDelta.y());
			}
			if(_cursorPosition & CursorPositionBottom) {
				target.setBottom(target.bottom() + mouseDelta.y());
			}
		} else {
			target.moveTo( lastStaticCroppingRect.topLeft() + mouseDelta );
		}
		enforceBounds(target, _cursorPosition);
		emit areaChanged(croppedRect());
		update();
	}
}*/

	void ImageCropper::enforceBounds(QRect target, CursorPosition position) {
		if(!target.isValid())
			return;

		if(position == CursorPositionUndefined) {
			//make sure it is of the right shape
			if(isProportionFixed) {
				//keep area more or less the same
				float area = target.width()*target.width();
				QPoint center = target.center();
				target.setWidth(round(sqrt(area*proportion.width()/proportion.height())));
				target.setHeight(round(sqrt(area*proportion.height()/proportion.width())));
				target.moveCenter(center);
			}
			target = ensureCropFits(target);
		}

		QSize size = getRotatedSize().size();
		int w = size.width();
		int h = size.height();
		if(position == CursorPositionMiddle) {
			if(target.left() < 0) target.moveLeft(0);
			if(target.top() < 0) target.moveTop(0);
			if(target.right() >= w) target.moveRight(w-1);
			if(target.bottom() >= h) target.moveBottom(h-1);

		} else {
			if(position & CursorPositionLeft) {
				if(target.left() < 0) target.setLeft(0);
			}
			if(position & CursorPositionRight) {
				if(target.right() >= w) target.setRight(w-1);
			}

			if(position & CursorPositionTop) {
				if(target.top() < 0) target.setTop(0);
			}
			if(position & CursorPositionBottom) {
				if(target.bottom() >= h) target.setBottom(h-1);
			}
		}

		target = ensureCropFits(target);
		if(isProportionFixed) {
			float h_over_w = proportion.height() / proportion.width();
			if(position & (CursorPositionLeft | CursorPositionRight)) {
				int height = round(target.width() * h_over_w);
				int delta = target.height() - height;
				target.setHeight(height);
				target.moveTop(target.top() + delta/2);
				if(target.top() < 0) target.moveTop(0);
				if(target.bottom() >= h)
					target.moveBottom(h-1);
			}
			if(position & (CursorPositionTop | CursorPositionBottom)) {
				int width = round(target.height() / h_over_w);
				int delta = target.width() - width;
				target.setWidth(width);
				target.moveLeft(target.left() + delta/2);
				if(target.left() < 0) target.moveLeft(0);
				if(target.right() >= w)
					target.moveRight(w-1);
			}
		}
		crop.setRect(target);
		emit areaChanged(crop);
		update();
	}

	void ImageCropper::update() {
		QSize center = imagePixmap->pixmap().size()/2;

		QRect rotatedSize = getRotatedSize();
		QSize new_center = rotatedSize.size()/2;

		QTransform t;
		t.translate(new_center.width(), new_center.height());
		t.rotate(crop.angle);

		t.translate(-center.width(), -center.height());
		imagePixmap->setTransform(t);
		updateCrop();

		ImageView::update();
	}


	QRect ImageCropper::ensureCropFits(QRect target) {
		QSizeF proportion = this->proportion;
		if(!isProportionFixed) {
			proportion.setWidth(target.width());
			proportion.setHeight(target.height());
		}
		QSize size = getRotatedSize().size();
		int w = size.width();
		int h = size.height();

		float h_over_w = proportion.height() / proportion.width();
		int max_width = std::min(w, (int)round(h/h_over_w));
		int max_height = std::min(h, (int)round(w*h_over_w));
		if(target.width() > max_width) {
			target.setWidth(max_width);
			target.setHeight(h);
		}
		if(target.height() > max_height) {
			target.setHeight(max_height);
			target.setWidth(w);
		}
		if(target.left() < 0) target.moveLeft(0);
		if(target.top() < 0) target.moveTop(0);

		return target;
	}

	/*
void ImageCropper::mouseReleaseEvent(QMouseEvent* _event) {
	isMousePressed = false;
	updateCursorIcon(_event->pos());
}


CursorPosition ImageCropper::cursorPosition(const QRectF& cropRect, const QPointF& mousePosition) {

	float x = mousePosition.x();
	float y = mousePosition.y();
	QRectF outside = cropRect.adjusted(-handleMargin, -handleMargin, handleMargin, handleMargin);
	if(!outside.contains(mousePosition))
		return CursorPositionUndefined;

	int top = 2*(fabs(cropRect.top() - y) < handleMargin);
	int bottom = 8*(fabs(cropRect.bottom() - y) < handleMargin);
	int left = 1*(fabs(cropRect.left() - x) < handleMargin);
	int right = 4*(fabs(cropRect.right() - x) < handleMargin);

	int position = top + bottom + left + right;
	if(!position)
		return CursorPositionMiddle;

	return (CursorPosition(position));
} */
	/*
void ImageCropper::updateCursorIcon(const QPointF& mousePosition)
{
	QCursor cursorIcon;
	QRectF croppingRect = imageCroppedRect();

	switch (cursorPosition(croppingRect, mousePosition))
	{
		case CursorPositionTopRight:
		case CursorPositionBottomLeft:
			cursorIcon = QCursor(Qt::SizeBDiagCursor);
			break;
		case CursorPositionTopLeft:
		case CursorPositionBottomRight:
			cursorIcon = QCursor(Qt::SizeFDiagCursor);
			break;
		case CursorPositionTop:
		case CursorPositionBottom:
			cursorIcon = QCursor(Qt::SizeVerCursor);
			break;
		case CursorPositionLeft:
		case CursorPositionRight:
			cursorIcon = QCursor(Qt::SizeHorCursor);
			break;
		case CursorPositionMiddle:
			cursorIcon = isMousePressed ?
						QCursor(Qt::ClosedHandCursor) :
						QCursor(Qt::OpenHandCursor);
			break;
		case CursorPositionUndefined:
		default:
			cursorIcon = QCursor(Qt::ArrowCursor);
			break;
	}
	this->setCursor(cursorIcon);
}*/

#include "imagecropper.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <math.h>
#include <iostream>
#include <algorithm>
using namespace std;

namespace {
	static const QSize WIDGET_MINIMUM_SIZE(300, 300);
}

ImageCropper::ImageCropper(QWidget* parent):	QWidget(parent) {
	setMinimumSize(WIDGET_MINIMUM_SIZE);
	setMouseTracking(true);
}

ImageCropper::~ImageCropper() {}

void ImageCropper::setImage(const QPixmap& _image) {
	image = _image;
	realSizeRect = QRect(QPoint(0, 0), image.size());
	update();
}

void ImageCropper::setBackgroundColor(const QColor& _backgroundColor) {
	backgroundColor = _backgroundColor;
	update();
}

void ImageCropper::setCroppingRectBorderColor(const QColor& _borderColor) {
	croppingRectBorderColor = _borderColor;
	update();
}

void ImageCropper::setProportion(const QSizeF& _proportion) {
	if (proportion != _proportion) {
		proportion = _proportion;
		float heightDelta = (float)_proportion.height() / _proportion.width();
		float widthDelta = (float)_proportion.width() / _proportion.height();
		deltas.setHeight(heightDelta);
		deltas.setWidth(widthDelta);
	}

	if ( isProportionFixed ) {
		enforceBounds(realSizeRect, CursorPositionUndefined);
	}
	emit areaChanged(croppedRect());
}

void ImageCropper::setProportionFixed(const bool _isFixed) {
	isProportionFixed = _isFixed;
}

//returns cropped rect in image ccords
QRectF ImageCropper::imageCroppedRect() {
	QRectF r;
	r.setLeft(realSizeRect.left()/xScale + leftDelta);
	r.setTop(realSizeRect.top()/yScale + topDelta);
	r.setWidth(realSizeRect.width()/xScale);
	r.setHeight(realSizeRect.height()/yScale);
	return r;
}

QRect ImageCropper::croppedRect() {
	return realSizeRect;
}

void ImageCropper::setCrop(QRect rect) {
	if(!rect.isValid())
		return;
	enforceBounds(rect, CursorPositionMiddle);
}

void ImageCropper::resetCrop() {
	setCrop(image.rect());
}

void ImageCropper::setWidth(int w) {
	QRect target = realSizeRect;
	if(target.width() == w) return;
	target.setWidth(w);
	enforceBounds(target, CursorPositionRight);
}
void ImageCropper::setHeight(int h) {
	QRect target = realSizeRect;
	if(target.height() == h) return;
	target.setHeight(h);
	enforceBounds(target, CursorPositionBottom);
}

void ImageCropper::setTop(int t) {
	QRect target = realSizeRect;
	if(target.top() == t) return;
	target.moveTop(t);
	enforceBounds(target, CursorPositionMiddle);
}
void ImageCropper::setLeft(int l) {
	QRect target = realSizeRect;
	if(target.left() == l) return;
	target.moveLeft(l);
	enforceBounds(target, CursorPositionMiddle);
}

void ImageCropper::maximizeCrop() {
	setCrop(image.rect());
}

void ImageCropper::centerCrop() {
	QRect target = realSizeRect;
	target.moveCenter(image.rect().center());
	setCrop(target);
}	

void ImageCropper::updateDeltaAndScale() {
	QSize initial = image.size();
	QSize scaledImageSize = initial.scaled(this->size(), Qt::KeepAspectRatio);

	leftDelta = 0.0f;
	topDelta = 0.0f;
	if (this->size().height() == scaledImageSize.height()) {
		leftDelta = (this->width() - scaledImageSize.width()) / 2.0f;
	} else {
		topDelta = (this->height() - scaledImageSize.height()) /  2.0f;
	}
	xScale = (float)image.width()  / scaledImageSize.width();
	yScale = (float)image.height() / scaledImageSize.height();
}

void ImageCropper::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	QRect currentRealRect = croppedRect();
	updateDeltaAndScale();
	setCrop(currentRealRect);
}


void ImageCropper::showHandle(bool _show) {
	show_handle = _show;
	update();
}
void ImageCropper::hideHandle() {
	show_handle = false;
	update();
}

void ImageCropper::paintEvent(QPaintEvent* _event) {
	QWidget::paintEvent( _event );

	QRectF croppingRect = imageCroppedRect();
	QPainter widgetPainter(this);

	{
		QPixmap scaledImage = image.scaled(this->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

		widgetPainter.fillRect( this->rect(), backgroundColor );
		
		if ( this->size().height() == scaledImage.height() ) {
			widgetPainter.drawPixmap( ( this->width() - scaledImage.width() ) / 2, 0, scaledImage );
		} else {
			widgetPainter.drawPixmap( 0, ( this->height() - scaledImage.height() ) / 2, scaledImage );
		}
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
		widgetPainter.setBrush(QBrush(QColor(0,0,0,120)));
		widgetPainter.setPen(Qt::transparent);
		widgetPainter.drawPath(p);

		widgetPainter.setPen(croppingRectBorderColor);
		widgetPainter.setBrush(QBrush(Qt::transparent));
		widgetPainter.drawRect(croppingRect);


		{
			widgetPainter.setBrush(QBrush(croppingRectBorderColor));

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

			widgetPainter.drawRects( points );
		}

		{
			QPen dashPen(croppingRectBorderColor);
			dashPen.setStyle(Qt::DashLine);
			widgetPainter.setPen(dashPen);

			widgetPainter.drawLine(
						QPoint(croppingRect.center().x(), croppingRect.top()),
						QPoint(croppingRect.center().x(), croppingRect.bottom()) );

			widgetPainter.drawLine(
						QPoint(croppingRect.left(), croppingRect.center().y()),
						QPoint(croppingRect.right(), croppingRect.center().y()) );
		}
	}

	widgetPainter.end();
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
}

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
	

	if(position == CursorPositionMiddle) {
		if(target.left() < 0) target.moveLeft(0);
		if(target.top() < 0) target.moveTop(0);
		if(target.right() >= image.width()) target.moveRight(image.width()-1);
		if(target.bottom() >= image.height()) target.moveBottom(image.height()-1);

	} else {
		if(position & CursorPositionLeft) {
			if(target.left() < 0) target.setLeft(0);
		}
		if(position & CursorPositionRight) {
			if(target.right() >= image.width()) target.setRight(image.width()-1);
		}

		if(position & CursorPositionTop) {
			if(target.top() < 0) target.setTop(0);
		}
		if(position & CursorPositionBottom) {
			if(target.bottom() >= image.height()) target.setBottom(image.height()-1);
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
			if(target.bottom() >= image.height()) 
				target.moveBottom(image.height()-1);
		}
		if(position & (CursorPositionTop | CursorPositionBottom)) {
			int width = round(target.height() / h_over_w);
			int delta = target.width() - width;
			target.setWidth(width);
			target.moveLeft(target.left() + delta/2);
			if(target.left() < 0) target.moveLeft(0);
			if(target.right() >= image.width()) 
				target.moveRight(image.width()-1);
		}
	}
	realSizeRect = target;
	emit areaChanged(croppedRect());
	update();
}

QRect ImageCropper::ensureCropFits(QRect target) {
	QSizeF proportion = this->proportion;
	if(!isProportionFixed) {
		proportion.setWidth(target.width());
		proportion.setHeight(target.height());
	}
		
	float h_over_w = proportion.height() / proportion.width();
	int max_width = std::min(image.width(), (int)round(image.height()/h_over_w));
	int max_height = std::min(image.height(), (int)round(image.width()*h_over_w));
	if(target.width() > max_width) {
		target.setWidth(max_width);
		target.setHeight(image.height());
	}
	if(target.height() > max_height) {
		target.setHeight(max_height);
		target.setWidth(image.width());
	}
	if(target.left() < 0) target.moveLeft(0);
	if(target.top() < 0) target.moveTop(0);

	return target;
}

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
}

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
}

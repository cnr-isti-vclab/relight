#include "imagecropper.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <math.h>
#include <iostream>
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
	imageForCropping = _image;
	update();
}

void ImageCropper::setBackgroundColor(const QColor& _backgroundColor) {
	backgroundColor = _backgroundColor;
	update();
}

void ImageCropper::setCroppingRectBorderColor(const QColor& _borderColor)
{
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
		float croppintRectSideRelation =
				(float)croppingRect.width() / croppingRect.height();
		float proportionSideRelation =
				(float)proportion.width() / proportion.height();
		if (croppintRectSideRelation != proportionSideRelation) {

			float area = croppingRect.width() * croppingRect.height();
			croppingRect.setWidth(sqrt(area /  deltas.height()));
			croppingRect.setHeight(sqrt(area /  deltas.width()));
			bool widthShotrerThenHeight =
					croppingRect.width() < croppingRect.height();
			if (widthShotrerThenHeight) {
				croppingRect.setHeight(
							croppingRect.width() * deltas.height());
			} else {
				croppingRect.setWidth(
							croppingRect.height() * deltas.width());
			}
			update();
		}
	}
	emit areaChanged(croppedRect());
}

void ImageCropper::setProportionFixed(const bool _isFixed)
{
	if (isProportionFixed != _isFixed) {
		isProportionFixed = _isFixed;
		setProportion(proportion);
	}
}

QRect ImageCropper::croppedRect() {
	QRectF &r = croppingRect;
	QRectF realSizeRect;
	realSizeRect.setLeft((r.left() - leftDelta) * xScale);
	realSizeRect.setTop ((r.top() - topDelta) * yScale);

	realSizeRect.setWidth(r.width() * xScale);
	realSizeRect.setHeight(r.height() * yScale);

	realSizeRect = realSizeRect.intersected(QRectF(0, 0, imageForCropping.width(), imageForCropping.height()));
	return realSizeRect.toRect();
}

void ImageCropper::setCrop(QRect rect) {
	if(!rect.isValid())
		return;
	QRectF &r = croppingRect;
	r.setLeft(rect.left()/xScale + leftDelta);
	r.setTop(rect.top()/yScale + topDelta);
	r.setWidth(rect.width()/xScale);
	r.setHeight(rect.height()/yScale);
	update();
}

void ImageCropper::resetCrop() {
	setCrop(imageForCropping.rect());
}

void ImageCropper::updateDeltaAndScale() {
	//TODO don't resize an image just to compute a proportion.
	QSize scaledImageSize = imageForCropping.scaled(
				this->size(), Qt::KeepAspectRatio, Qt::FastTransformation
				).size();
	leftDelta = 0.0f;
	topDelta = 0.0f;
	if (this->size().height() == scaledImageSize.height()) {
		leftDelta = (this->width() - scaledImageSize.width()) / 2.0f;
	} else {
		if(this->size().width() != scaledImageSize.width())
			throw "Should never happen this";
		topDelta = (this->height() - scaledImageSize.height()) /  2.0f;
	}
	xScale = (float)imageForCropping.width()  / scaledImageSize.width();
	yScale = (float)imageForCropping.height() / scaledImageSize.height();
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

// ********
// Protected section

void ImageCropper::paintEvent(QPaintEvent* _event)
{
	QWidget::paintEvent( _event );
	//
	QPainter widgetPainter(this);

	{
		QPixmap scaledImage =
				imageForCropping.scaled(this->size(), Qt::KeepAspectRatio, Qt::FastTransformation);

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

		{
			widgetPainter.setBrush(QBrush(Qt::transparent));
			widgetPainter.drawRect(croppingRect);
		}

		{
			widgetPainter.setBrush(QBrush(croppingRectBorderColor));

			int leftXCoord   = croppingRect.left() - 2;
			int centerXCoord = croppingRect.center().x() - 3;
			int rightXCoord  = croppingRect.right() - 2;

			int topYCoord    = croppingRect.top() - 2;
			int middleYCoord = croppingRect.center().y() - 3;
			int bottomYCoord = croppingRect.bottom() - 2;
			//
			const QSize pointSize(6, 6);
			//
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
			//
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
	//
	widgetPainter.end();
}

void ImageCropper::mousePressEvent(QMouseEvent* _event)
{
	if (_event->button() == Qt::LeftButton) {
		isMousePressed = true;
		startMousePos = _event->pos();
		lastStaticCroppingRect = croppingRect;
	}
	//
	updateCursorIcon(_event->pos());
}

void ImageCropper::mouseMoveEvent(QMouseEvent* _event)
{
	QPointF mousePos = _event->pos();
	//
	if (!isMousePressed) {
		_cursorPosition = cursorPosition(croppingRect, mousePos);
		updateCursorIcon(mousePos);
	} else if (_cursorPosition != CursorPositionUndefined) {
		QPointF mouseDelta;
		mouseDelta.setX( mousePos.x() - startMousePos.x() );
		mouseDelta.setY( mousePos.y() - startMousePos.y() );
		//
		QRectF &r = croppingRect;

		if (_cursorPosition != CursorPositionMiddle) {
		
			QRectF newGeometry =
					calculateGeometry(
						lastStaticCroppingRect,
						_cursorPosition,
						mouseDelta);

			if (!newGeometry.isNull()) {
				r = newGeometry;

				if(r.left() < leftDelta) r.setLeft(leftDelta);
				if(r.top() < topDelta) r.setTop(topDelta);
				float rightEdge = imageForCropping.width()/xScale + leftDelta;
				if(r.right() > rightEdge) r.setRight(rightEdge);

				float bottomEdge = imageForCropping.height()/yScale + topDelta;
				if(r.bottom() > bottomEdge) r.setBottom(bottomEdge);
			}
		} else {

			r.moveTo( lastStaticCroppingRect.topLeft() + mouseDelta );

			if(r.left() < leftDelta) r.moveLeft(leftDelta);
			if(r.top() < topDelta) r.moveTop(topDelta);

			float rightEdge = imageForCropping.width()/xScale + leftDelta;
			if(r.right() > rightEdge) r.moveRight(rightEdge);

			float bottomEdge = imageForCropping.height()/yScale + topDelta;
			if(r.bottom() > bottomEdge) r.moveBottom(bottomEdge);
		}
		emit areaChanged(croppedRect());
		update();
	}
}

void ImageCropper::mouseReleaseEvent(QMouseEvent* _event)
{
	isMousePressed = false;
	updateCursorIcon(_event->pos());
}

// ********
// Private section

CursorPosition ImageCropper::cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition)
{
	//
	float x = _mousePosition.x();
	float y = _mousePosition.y();
	QRectF outside = _cropRect.adjusted(-handleMargin, -handleMargin, handleMargin, handleMargin);
	if(!outside.contains(_mousePosition))
		return CursorPositionUndefined;

	int top = fabs(_cropRect.top() - y) < handleMargin;
	int bottom = fabs(_cropRect.bottom() - y) < handleMargin;
	int left = fabs(_cropRect.left() - x) < handleMargin;
	int right = fabs(_cropRect.right() - x) < handleMargin;

	if(top && left)
		return CursorPositionTopLeft;
	if(bottom && left)
		return CursorPositionBottomLeft;
	if(top && right)
		return CursorPositionTopRight;
	if(bottom && right)
		return CursorPositionBottomRight;

	if(left)
		return CursorPositionLeft;
	if(right)
		return CursorPositionRight;
	if(top)
		return CursorPositionTop;
	if(bottom)
		return CursorPositionBottom;

	return CursorPositionMiddle;
}

void ImageCropper::updateCursorIcon(const QPointF& _mousePosition)
{
	QCursor cursorIcon;
	//
	switch (cursorPosition(croppingRect, _mousePosition))
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

const QRectF ImageCropper::calculateGeometry(
		const QRectF& _sourceGeometry,
		const CursorPosition _cursorPosition,
		const QPointF& _mouseDelta
		)
{
	QRectF resultGeometry;
	//
	if ( isProportionFixed ) {
		resultGeometry =
				calculateGeometryWithFixedProportions(
					_sourceGeometry, _cursorPosition, _mouseDelta, deltas);
	} else {
		resultGeometry =
				calculateGeometryWithCustomProportions(
					_sourceGeometry, _cursorPosition, _mouseDelta);
	}

	if ((resultGeometry.left() >= resultGeometry.right()) ||
		(resultGeometry.top() >= resultGeometry.bottom())) {
		resultGeometry = QRect();
	}

	//ensure geometry fits in the image.
	if(resultGeometry.left() < 0) resultGeometry.setLeft(0);
	if(resultGeometry.top() < 0) resultGeometry.setTop(0);

	//
	return resultGeometry;
}

const QRectF ImageCropper::calculateGeometryWithCustomProportions(
		const QRectF& _sourceGeometry,
		const CursorPosition _cursorPosition,
		const QPointF& _mouseDelta
		)
{
	QRectF resultGeometry = _sourceGeometry;
	//
	switch ( _cursorPosition )
	{
		case CursorPositionTopLeft:
			resultGeometry.setLeft( _sourceGeometry.left() + _mouseDelta.x() );
			resultGeometry.setTop ( _sourceGeometry.top()  + _mouseDelta.y() );
			break;
		case CursorPositionTopRight:
			resultGeometry.setTop  ( _sourceGeometry.top()   + _mouseDelta.y() );
			resultGeometry.setRight( _sourceGeometry.right() + _mouseDelta.x() );
			break;
		case CursorPositionBottomLeft:
			resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
			resultGeometry.setLeft  ( _sourceGeometry.left()   + _mouseDelta.x() );
			break;
		case CursorPositionBottomRight:
			resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
			resultGeometry.setRight ( _sourceGeometry.right()  + _mouseDelta.x() );
			break;
		case CursorPositionTop:
			resultGeometry.setTop( _sourceGeometry.top() + _mouseDelta.y() );
			break;
		case CursorPositionBottom:
			resultGeometry.setBottom( _sourceGeometry.bottom() + _mouseDelta.y() );
			break;
		case CursorPositionLeft:
			resultGeometry.setLeft( _sourceGeometry.left() + _mouseDelta.x() );
			break;
		case CursorPositionRight:
			resultGeometry.setRight( _sourceGeometry.right() + _mouseDelta.x() );
			break;
		default:
			break;
	}
	//
	return resultGeometry;
}

const QRectF ImageCropper::calculateGeometryWithFixedProportions(
		const QRectF& _sourceGeometry,
		const CursorPosition _cursorPosition,
		const QPointF& _mouseDelta,
		const QSizeF& _deltas
		)
{
	QRectF resultGeometry = _sourceGeometry;
	//
	switch (_cursorPosition)
	{
		case CursorPositionLeft:
			resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.x() * _deltas.height());
			resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
			break;
		case CursorPositionRight:
			resultGeometry.setTop(_sourceGeometry.top() - _mouseDelta.x() * _deltas.height());
			resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
			break;
		case CursorPositionTop:
			resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
			resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
			break;
		case CursorPositionBottom:
			resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
			resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.y() * _deltas.width());
			break;
		case CursorPositionTopLeft:
			if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y())) {
				resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.x() * _deltas.height());
				resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
			} else {
				resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
				resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.y() * _deltas.width());
			}
			break;
		case CursorPositionTopRight:
			if ((_mouseDelta.x() * _deltas.height() * -1) < (_mouseDelta.y())) {
				resultGeometry.setTop(_sourceGeometry.top() - _mouseDelta.x() * _deltas.height());
				resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x() );
			} else {
				resultGeometry.setTop(_sourceGeometry.top() + _mouseDelta.y());
				resultGeometry.setRight(_sourceGeometry.right() - _mouseDelta.y() * _deltas.width());
			}
			break;
		case CursorPositionBottomLeft:
			if ((_mouseDelta.x() * _deltas.height()) < (_mouseDelta.y() * -1)) {
				resultGeometry.setBottom(_sourceGeometry.bottom() - _mouseDelta.x() * _deltas.height());
				resultGeometry.setLeft(_sourceGeometry.left() + _mouseDelta.x());
			} else {
				resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
				resultGeometry.setLeft(_sourceGeometry.left() - _mouseDelta.y() * _deltas.width());
			}
			break;
		case CursorPositionBottomRight:
			if ((_mouseDelta.x() * _deltas.height()) > (_mouseDelta.y())) {
				resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.x() * _deltas.height());
				resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.x());
			} else {
				resultGeometry.setBottom(_sourceGeometry.bottom() + _mouseDelta.y());
				resultGeometry.setRight(_sourceGeometry.right() + _mouseDelta.y() * _deltas.width());
			}
			break;
		default:
			break;
	}
	//
	return resultGeometry;
}


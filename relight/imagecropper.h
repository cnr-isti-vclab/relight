#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

#include <QWidget>
#include <QSizeF>

enum CursorPosition {
	CursorPositionUndefined,
	CursorPositionMiddle,
	CursorPositionTop,
	CursorPositionBottom,
	CursorPositionLeft,
	CursorPositionRight,
	CursorPositionTopLeft,
	CursorPositionTopRight,
	CursorPositionBottomLeft,
	CursorPositionBottomRight
};



class ImageCropper : public QWidget {
	Q_OBJECT

public:
	ImageCropper(QWidget *parent = 0);
	~ImageCropper();

public slots:
	void setImage(const QPixmap& _image);
	void setCroppingRectBorderColor(const QColor& _borderColor);
	void setProportion(const QSizeF& _proportion);
	void setProportionFixed(const bool _isFixed);
	void showHandle(bool _show = true);
	void hideHandle();
	void setCrop(QRect rect, bool preserveArea = false);
	void setWidth(int w);
	void setHeight(int h);
	void setTop(int t);
	void setLeft(int l);
	void resetCrop();

signals:
	void areaChanged(QRect rect);

public:
	bool handleShown() { return show_handle; }
	QRect imageCroppedRect(); //return cropped rect in image ccords
	QRect croppedRect();
	void enforceBounds(bool preserveArea); //makes sure the rectangle is inside the image.

protected:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent* _event);
	virtual void mousePressEvent(QMouseEvent* _event);
	virtual void mouseMoveEvent(QMouseEvent* _event);
	virtual void mouseReleaseEvent(QMouseEvent* _event);

private:
	CursorPosition cursorPosition(const QRectF& _cropRect, const QPointF& _mousePosition);
	void updateCursorIcon(const QPointF& _mousePosition);

	const QRectF calculateGeometry(
			const QRectF& _sourceGeometry,
			const CursorPosition _cursorPosition,
			const QPointF& _mouseDelta
			);
	const QRectF calculateGeometryWithCustomProportions(
			const QRectF& _sourceGeometry,
			const CursorPosition _cursorPosition,
			const QPointF& _mouseDelta
			);
	const QRectF calculateGeometryWithFixedProportions(
			const QRectF &_sourceGeometry,
			const CursorPosition _cursorPosition,
			const QPointF &_mouseDelta,
			const QSizeF &_deltas
			);

	void updateDeltaAndScale();


private:
	bool show_handle = true;

	//TODO invert logic: realSize rect is stored, cropping rect is computed instead.
	float leftDelta = 0, topDelta = 0;
	float xScale = 1, yScale = 1;

	QPixmap imageForCropping;
	QRectF croppingRect; //in image coords
	QRectF realSizeRect;

	QRectF lastStaticCroppingRect;
	CursorPosition _cursorPosition = CursorPositionUndefined;
	bool isMousePressed = false;
	bool isProportionFixed = false;
	QPointF startMousePos;
	QSizeF proportion = QSizeF(1.0, 1.0);
	QSizeF deltas = QSizeF(1.0, 1.0);
	QColor backgroundColor = Qt::black;
	QColor croppingRectBorderColor = Qt::white;
	float handleMargin = 20;

};

#endif // IMAGECROPPER_H

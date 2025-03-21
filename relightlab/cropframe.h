#ifndef CROPFRAME_H
#define CROPFRAME_H

#include <QFrame>

class ImageCropper;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;

class CropFrame: public QFrame {
	Q_OBJECT
public:
	CropFrame(QWidget *parent = nullptr);
	void clear();
	void init();
	void setCrop(QRect rect);

public slots:
	void setAspectRatio();
	void updateCrop(QRect rect);
	void scaleChanged(); //if units in mm are now available, update them.

signals:
	void cropChanged(QRect rect);
private:
	ImageCropper *cropper = nullptr;

	QComboBox *units = nullptr;
	float pixelSize = 1.0f;

	QDoubleSpinBox *crop_top    = nullptr;
	QDoubleSpinBox *crop_left   = nullptr;
	QDoubleSpinBox *crop_width  = nullptr;
	QDoubleSpinBox *crop_height = nullptr;

	QComboBox *aspect_combo = nullptr;
	QSpinBox *aspect_width = nullptr;
	QSpinBox *aspect_height = nullptr;
};

#endif // CROPFRAME_H

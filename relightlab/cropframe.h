#ifndef CROPFRAME_H
#define CROPFRAME_H

#include <QFrame>

class ImageCropper;
class QSpinBox;
class QComboBox;

class CropFrame: public QFrame {
	Q_OBJECT
public:
	CropFrame(QWidget *parent = nullptr);
	void clear();
	void init();

public slots:
	void setAspectRatio();
	void setArea(QRect rect);
	void areaChanged(QRect rect);

private:
	ImageCropper *cropper = nullptr;

	QSpinBox *crop_width = nullptr;
	QSpinBox *crop_height = nullptr;
	QSpinBox *crop_top = nullptr;
	QSpinBox *crop_left = nullptr;

	QComboBox *aspect_combo = nullptr;
	QSpinBox *aspect_width = nullptr;
	QSpinBox *aspect_height = nullptr;
};

#endif // CROPFRAME_H

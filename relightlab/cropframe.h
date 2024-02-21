#ifndef CROPFRAME_H
#define CROPFRAME_H

#include <QFrame>

class ImageCropper;
class QSpinBox;

class CropFrame: public QFrame {
	Q_OBJECT
public:
	CropFrame(QWidget *parent = nullptr);

public slots:
	void setAspectRatio(int n);
private:
	ImageCropper *cropper = nullptr;
	QSpinBox *crop_width = nullptr;
	QSpinBox *crop_height = nullptr;
	QSpinBox *crop_top = nullptr;
	QSpinBox *crop_left = nullptr;
	QSpinBox *crop_bottom = nullptr;
	QSpinBox *crop_right = nullptr;

	QSpinBox *aspect_width = nullptr;
	QSpinBox *aspect_height = nullptr;
};

#endif // CROPFRAME_H

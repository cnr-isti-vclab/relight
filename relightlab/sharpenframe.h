#ifndef SHARPENFRAME_H
#define SHARPENFRAME_H

#include <QFrame>
#include <QString>
#include "../src/normals/unsharp_mask.h"

class QComboBox;
class QLabel;
class QPixmap;
class QSlider;
class QToolBar;
class ImageView;

class SharpenFrame: public QFrame {
	Q_OBJECT
public:
	SharpenFrame(QWidget *parent = nullptr);

private slots:
	void loadImage();
	void loadNormal();
	void loadHeight();
	void rotateLeft();
	void rotateRight();

private:
	void loadFile(SharpenInputType type, const QString &kind);
	void updatePreview(bool fitView = false);
	void rotateImage(int degrees);

	SharpenData sharpen_data;
	QToolBar *navigation_toolbar = nullptr;
	ImageView *image_view = nullptr;
	QLabel *image_info = nullptr;
	QSlider *radius_slider = nullptr;
	QSlider *gain_slider = nullptr;
	QSlider *minimum_value_slider = nullptr;
	QSlider *maximum_value_slider = nullptr;
	QComboBox *color_scheme = nullptr;
	int rotation = 0;
};

#endif // SHARPENFRAME_H

#ifndef CALHISTOGRAMFRAME_H
#define CALHISTOGRAMFRAME_H

#include <QFrame>

class QSlider;
class QLabel;

class CalHistogramFrame: public QFrame {
	Q_OBJECT
public:
	explicit CalHistogramFrame(QWidget *parent = nullptr);

public slots:
	void whitePointChanged(int value);

private:
	QFrame  *histogram_display  = nullptr;  // painted histogram placeholder
	QSlider *white_point_slider = nullptr;
	QLabel  *white_point_label  = nullptr;
	QLabel  *burned_label       = nullptr;
};

#endif // CALHISTOGRAMFRAME_H

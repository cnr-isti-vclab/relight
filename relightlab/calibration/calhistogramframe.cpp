#include "calhistogramframe.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSizePolicy>

static QLabel *makeInfo(const QString &text) {
	QLabel *lbl = new QLabel(text);
	lbl->setWordWrap(true);
	lbl->setProperty("class", "info");
	return lbl;
}

static QFrame *makeSeparator() {
	QFrame *sep = new QFrame;
	sep->setFrameShape(QFrame::HLine);
	sep->setFrameShadow(QFrame::Sunken);
	return sep;
}

CalHistogramFrame::CalHistogramFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setSpacing(8);

	layout->addWidget(makeInfo(
		"Analyze the luminance distribution of your calibration images. "
		"Adjust the white point to exclude overexposed (burned) pixels from fitting. "
		"The 99th-percentile is used by default."));

	layout->addWidget(makeSeparator());

	// placeholder histogram display
	histogram_display = new QFrame;
	histogram_display->setFrameShape(QFrame::Box);
	histogram_display->setMinimumHeight(180);
	histogram_display->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	{
		QVBoxLayout *hl = new QVBoxLayout(histogram_display);
		QLabel *placeholder = new QLabel("[Histogram will be rendered here]");
		placeholder->setAlignment(Qt::AlignCenter);
		hl->addWidget(placeholder);
	}
	layout->addWidget(histogram_display, 1);

	// white-point slider
	{
		QHBoxLayout *row = new QHBoxLayout;
		row->addWidget(new QLabel("White point:"));

		white_point_slider = new QSlider(Qt::Horizontal);
		white_point_slider->setRange(1, 100);
		white_point_slider->setValue(99);
		row->addWidget(white_point_slider, 1);

		white_point_label = new QLabel("99%");
		white_point_label->setMinimumWidth(40);
		row->addWidget(white_point_label);

		layout->addLayout(row);

		connect(white_point_slider, &QSlider::valueChanged,
		        this, &CalHistogramFrame::whitePointChanged);
	}

	burned_label = new QLabel("Estimated burned pixels: 0%");
	layout->addWidget(burned_label);
}

void CalHistogramFrame::whitePointChanged(int value) {
	white_point_label->setText(QString::number(value) + "%");
	// TODO: recompute actual burned-pixel percentage from image data
	burned_label->setText("Estimated burned pixels: (recompute needed)");
}

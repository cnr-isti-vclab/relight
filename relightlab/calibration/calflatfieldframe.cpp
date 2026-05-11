#include "calflatfieldframe.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QFileDialog>

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

CalFlatfieldFrame::CalFlatfieldFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setSpacing(8);

	layout->addWidget(makeInfo(
		"Correct for vignetting and uneven per-LED illumination. "
		"Provide one RAW image per LED of a uniform neutral grey card captured with "
		"the same lens and aperture as your calibration session. "
		"The correction map will be fitted to a 2D polynomial to save memory."));

	layout->addWidget(makeSeparator());

	// toolbar
	{
		QHBoxLayout *toolbar = new QHBoxLayout;
		QPushButton *load_btn = new QPushButton(QIcon::fromTheme("folder"), "Load flatfield images...");
		load_btn->setProperty("class", "large");
		load_btn->setMinimumWidth(200);
		connect(load_btn, &QPushButton::clicked, this, &CalFlatfieldFrame::loadImages);
		toolbar->addWidget(load_btn);

		QPushButton *clear_btn = new QPushButton(QIcon::fromTheme("trash"), "Clear");
		connect(clear_btn, &QPushButton::clicked, this, &CalFlatfieldFrame::clearImages);
		toolbar->addWidget(clear_btn);

		toolbar->addStretch();
		layout->addLayout(toolbar);
	}

	ff_list = new QListWidget;
	ff_list->setAlternatingRowColors(true);
	layout->addWidget(ff_list, 1);

	ff_status = new QLabel("No flatfield images loaded.");
	layout->addWidget(ff_status);

	layout->addWidget(makeSeparator());

	// fit order
	{
		QHBoxLayout *row = new QHBoxLayout;
		row->addWidget(new QLabel("Gain-map polynomial order:"));
		fit_order = new QComboBox;
		fit_order->addItem("2nd order");
		fit_order->addItem("4th order (default)");
		fit_order->addItem("6th order");
		fit_order->setCurrentIndex(1);
		connect(fit_order, QOverload<int>::of(&QComboBox::currentIndexChanged),
		        this, &CalFlatfieldFrame::fitOrderChanged);
		row->addWidget(fit_order);
		row->addStretch();
		layout->addLayout(row);
	}

	layout->addStretch();
}

void CalFlatfieldFrame::loadImages() {
	QStringList files = QFileDialog::getOpenFileNames(
		this, "Select flatfield images", QString(),
		"Images (*.tif *.tiff *.jpg *.jpeg *.png *.cr2 *.arw *.nef *.dng);;All files (*)");

	for (const QString &f : files)
		ff_list->addItem(f);

	int n = ff_list->count();
	ff_status->setText(n == 0 ? "No flatfield images loaded."
	                           : QString::number(n) + " image(s) loaded.");
}

void CalFlatfieldFrame::clearImages() {
	ff_list->clear();
	ff_status->setText("No flatfield images loaded.");
}

void CalFlatfieldFrame::fitOrderChanged(int /*index*/) {
	// TODO: re-fit gain map when order changes
}

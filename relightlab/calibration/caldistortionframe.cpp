#include "caldistortionframe.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QButtonGroup>
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

CalDistortionFrame::CalDistortionFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setSpacing(8);

	layout->addWidget(makeInfo(
		"Remove barrel/pincushion distortion introduced by the lens optics. "
		"Load coefficients from a lens.json file, look up a lens profile in the "
		"Lensfun database, or enter Brown-Conrady parameters manually."));

	layout->addWidget(makeSeparator());

	// source radio buttons
	QGroupBox *source_box = new QGroupBox("Distortion source");
	QVBoxLayout *source_layout = new QVBoxLayout(source_box);

	radio_none    = new QRadioButton("None (no distortion correction)");
	radio_file    = new QRadioButton("Load from lens.json file");
	radio_lensfun = new QRadioButton("Look up in Lensfun database");
	radio_manual  = new QRadioButton("Enter coefficients manually");
	radio_none->setChecked(true);

	source_layout->addWidget(radio_none);
	source_layout->addWidget(radio_file);
	source_layout->addWidget(radio_lensfun);
	source_layout->addWidget(radio_manual);
	layout->addWidget(source_box);

	// stacked parameter area
	param_stack = new QStackedWidget;

	// page 0: none
	param_stack->addWidget(new QWidget);

	// page 1: file
	{
		QFrame *page = new QFrame;
		QHBoxLayout *row = new QHBoxLayout(page);
		row->addWidget(new QLabel("Lens file:"));
		lens_file_path = new QLineEdit;
		lens_file_path->setPlaceholderText("path/to/lens.json");
		row->addWidget(lens_file_path, 1);
		QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
		connect(browse, &QPushButton::clicked, this, &CalDistortionFrame::browseFile);
		row->addWidget(browse);
		param_stack->addWidget(page);
	}

	// page 2: lensfun
	{
		QFrame *page = new QFrame;
		QFormLayout *form = new QFormLayout(page);
		lf_maker = new QComboBox;
		lf_maker->addItem("(select manufacturer)");
		lf_model = new QComboBox;
		lf_model->addItem("(select model)");
		form->addRow("Manufacturer:", lf_maker);
		form->addRow("Model:", lf_model);
		QLabel *note = new QLabel("<i>Lensfun integration requires the lensfun library (not yet linked).</i>");
		note->setWordWrap(true);
		form->addRow(note);
		param_stack->addWidget(page);
	}

	// page 3: manual
	{
		QFrame *page = new QFrame;
		QFormLayout *form = new QFormLayout(page);
		auto makeSB = [](double lo, double hi) {
			QDoubleSpinBox *sb = new QDoubleSpinBox;
			sb->setRange(lo, hi);
			sb->setDecimals(6);
			sb->setSingleStep(0.001);
			return sb;
		};
		sb_k1 = makeSB(-5.0, 5.0);
		sb_k2 = makeSB(-5.0, 5.0);
		sb_k3 = makeSB(-5.0, 5.0);
		sb_p1 = makeSB(-1.0, 1.0);
		sb_p2 = makeSB(-1.0, 1.0);
		form->addRow("k1 (radial):",     sb_k1);
		form->addRow("k2 (radial):",     sb_k2);
		form->addRow("k3 (radial):",     sb_k3);
		form->addRow("p1 (tangential):", sb_p1);
		form->addRow("p2 (tangential):", sb_p2);
		param_stack->addWidget(page);
	}

	layout->addWidget(param_stack);

	// wire radio → stack
	QButtonGroup *bg = new QButtonGroup(this);
	bg->addButton(radio_none,    0);
	bg->addButton(radio_file,    1);
	bg->addButton(radio_lensfun, 2);
	bg->addButton(radio_manual,  3);
	connect(bg, QOverload<int>::of(&QButtonGroup::idClicked),
	        param_stack, &QStackedWidget::setCurrentIndex);

	// load / save row
	{
		QHBoxLayout *row = new QHBoxLayout;
		QPushButton *load_btn = new QPushButton(QIcon::fromTheme("folder"), "Load...");
		connect(load_btn, &QPushButton::clicked, this, &CalDistortionFrame::loadFile);
		row->addWidget(load_btn);
		QPushButton *save_btn = new QPushButton(QIcon::fromTheme("save"), "Save...");
		connect(save_btn, &QPushButton::clicked, this, &CalDistortionFrame::saveFile);
		row->addWidget(save_btn);
		row->addStretch();
		layout->addLayout(row);
	}

	layout->addStretch();
}

void CalDistortionFrame::sourceChanged() {}

void CalDistortionFrame::browseFile() {
	QString path = QFileDialog::getOpenFileName(
		this, "Select lens file", QString(), "JSON files (*.json);;All files (*)");
	if (!path.isEmpty())
		lens_file_path->setText(path);
}

void CalDistortionFrame::loadFile() {
	QString path = QFileDialog::getOpenFileName(
		this, "Load distortion parameters", QString(), "JSON files (*.json);;All files (*)");
	// TODO: parse lens.json and populate controls
	(void)path;
}

void CalDistortionFrame::saveFile() {
	QString path = QFileDialog::getSaveFileName(
		this, "Save distortion parameters", "lens.json", "JSON files (*.json)");
	// TODO: write current parameters to file
	(void)path;
}

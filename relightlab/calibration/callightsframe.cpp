#include "callightsframe.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
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

CalLightsFrame::CalLightsFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setSpacing(8);

	layout->addWidget(makeInfo(
		"Specify the 3D direction of each LED in the dome. Either load an LP file "
		"(pre-computed directions) or derive directions from reflective spheres "
		"captured in the same session."));

	layout->addWidget(makeSeparator());

	// source radio buttons
	QGroupBox *source_box = new QGroupBox("Light direction source");
	QVBoxLayout *source_layout = new QVBoxLayout(source_box);
	radio_lp     = new QRadioButton("Load LP file (pre-computed directions)");
	radio_sphere = new QRadioButton("Compute from reflective spheres");
	radio_lp->setChecked(true);
	source_layout->addWidget(radio_lp);
	source_layout->addWidget(radio_sphere);
	layout->addWidget(source_box);

	// stacked parameter area
	lp_stack = new QStackedWidget;

	// page 0: LP file
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);

		// LP path row
		{
			QHBoxLayout *row = new QHBoxLayout;
			row->addWidget(new QLabel("LP file:"));
			lp_file_path = new QLineEdit;
			lp_file_path->setPlaceholderText("path/to/lights.lp");
			row->addWidget(lp_file_path, 1);
			QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
			connect(browse, &QPushButton::clicked, this, &CalLightsFrame::browseLp);
			row->addWidget(browse);
			pl->addLayout(row);
		}

		// dome geometry
		QGroupBox *geom_box = new QGroupBox("Dome geometry");
		QFormLayout *form = new QFormLayout(geom_box);

		sb_radius = new QDoubleSpinBox;
		sb_radius->setRange(1.0, 5000.0);
		sb_radius->setValue(300.0);
		sb_radius->setSuffix(" mm");
		sb_radius->setDecimals(1);
		form->addRow("Dome radius:", sb_radius);

		sb_offset = new QDoubleSpinBox;
		sb_offset->setRange(-500.0, 500.0);
		sb_offset->setValue(0.0);
		sb_offset->setSuffix(" mm");
		sb_offset->setDecimals(1);
		form->addRow("Height offset:", sb_offset);

		pl->addWidget(geom_box);
		pl->addStretch();

		lp_stack->addWidget(page);
	}

	// page 1: sphere
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		sphere_info = new QLabel(
			"Light directions will be computed from the reflective sphere(s) "
			"marked in the <b>Spheres</b> tab of the main window.\n\n"
			"Make sure at least one sphere is configured and highlights have been detected.");
		sphere_info->setWordWrap(true);
		pl->addWidget(sphere_info);
		pl->addStretch();
		lp_stack->addWidget(page);
	}

	layout->addWidget(lp_stack);

	// wire radio → stack
	QButtonGroup *bg = new QButtonGroup(this);
	bg->addButton(radio_lp,     0);
	bg->addButton(radio_sphere, 1);
	connect(bg, QOverload<int>::of(&QButtonGroup::idClicked),
	        lp_stack, &QStackedWidget::setCurrentIndex);

	// load / export buttons
	{
		QHBoxLayout *row = new QHBoxLayout;
		QPushButton *load_btn = new QPushButton(QIcon::fromTheme("folder"), "Load LP...");
		connect(load_btn, &QPushButton::clicked, this, &CalLightsFrame::loadLp);
		row->addWidget(load_btn);
		QPushButton *export_btn = new QPushButton(QIcon::fromTheme("save"), "Export LP...");
		connect(export_btn, &QPushButton::clicked, this, &CalLightsFrame::exportLp);
		row->addWidget(export_btn);
		row->addStretch();
		layout->addLayout(row);
	}

	layout->addStretch();
}

void CalLightsFrame::sourceChanged() {}

void CalLightsFrame::browseLp() {
	QString path = QFileDialog::getOpenFileName(
		this, "Select LP file", QString(), "LP files (*.lp);;All files (*)");
	if (!path.isEmpty())
		lp_file_path->setText(path);
}

void CalLightsFrame::loadLp() {
	QString path = QFileDialog::getOpenFileName(
		this, "Load LP file", QString(), "LP files (*.lp);;All files (*)");
	// TODO: parse LP file and populate light directions
	(void)path;
}

void CalLightsFrame::exportLp() {
	QString path = QFileDialog::getSaveFileName(
		this, "Export LP file", "lights.lp", "LP files (*.lp)");
	// TODO: write current light directions to LP file
	(void)path;
}

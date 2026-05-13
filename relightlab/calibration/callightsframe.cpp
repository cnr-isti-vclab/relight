#include "callightsframe.h"
#include "../../src/calibration/calibrationsession.h"
#include "../../src/dome.h"

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
#include <QScrollArea>

CalLightsFrame::CalLightsFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	QScrollArea *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll);

	QWidget *body = new QWidget;
	scroll->setWidget(body);

	QVBoxLayout *layout = new QVBoxLayout(body);
	layout->setContentsMargins(16, 16, 16, 16);
	layout->setSpacing(12);

	// ---- Light-position model -------------------------------------------
	QGroupBox *model_box = new QGroupBox("Light position model");
	QVBoxLayout *model_lay = new QVBoxLayout(model_box);

	radio_dir = new QRadioButton(
		"Directional  \u2013 infinite-distance lights; only directions are stored in the LP");
	radio_sph = new QRadioButton(
		"Spherical  \u2013 lights on a sphere; requires dome radius, vertical offset, and image width");
	radio_3d  = new QRadioButton(
		"3-D positional  \u2013 LP contains full 3-D coordinates (same units as offset / image width)");

	radio_sph->setChecked(true);

	QButtonGroup *model_bg = new QButtonGroup(this);
	model_bg->addButton(radio_dir, (int)Dome::DIRECTIONAL);
	model_bg->addButton(radio_sph, (int)Dome::SPHERICAL);
	model_bg->addButton(radio_3d,  (int)Dome::LIGHTS3D);

	model_lay->addWidget(radio_dir);
	model_lay->addWidget(radio_sph);
	model_lay->addWidget(radio_3d);
	layout->addWidget(model_box);

	// ---- Light direction source -----------------------------------------
	QGroupBox *source_box = new QGroupBox("Light direction source");
	QVBoxLayout *source_lay = new QVBoxLayout(source_box);

	radio_lp     = new QRadioButton("Load LP file");
	radio_sphere = new QRadioButton("Compute from reflective spheres");
	radio_lp->setChecked(true);

	QButtonGroup *src_bg = new QButtonGroup(this);
	src_bg->addButton(radio_lp,     0);
	src_bg->addButton(radio_sphere, 1);

	source_lay->addWidget(radio_lp);
	source_lay->addWidget(radio_sphere);

	// stacked area under the source selector
	source_stack = new QStackedWidget;

	// page 0: LP file path + load / export
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		pl->setContentsMargins(0, 4, 0, 0);

		QHBoxLayout *path_row = new QHBoxLayout;
		path_row->addWidget(new QLabel("LP file:"));
		lp_file_path = new QLineEdit;
		lp_file_path->setPlaceholderText("path/to/lights.lp");
		path_row->addWidget(lp_file_path, 1);
		QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
		connect(browse, &QPushButton::clicked, this, &CalLightsFrame::browseLP);
		path_row->addWidget(browse);
		pl->addLayout(path_row);

		QHBoxLayout *btn_row = new QHBoxLayout;
		QPushButton *load_btn = new QPushButton("Load LP\u2026");
		connect(load_btn, &QPushButton::clicked, this, &CalLightsFrame::loadLP);
		btn_row->addWidget(load_btn);
		QPushButton *export_btn = new QPushButton("Export LP\u2026");
		connect(export_btn, &QPushButton::clicked, this, &CalLightsFrame::exportLP);
		btn_row->addWidget(export_btn);
		btn_row->addStretch();
		pl->addLayout(btn_row);
		pl->addStretch();

		source_stack->addWidget(page);
	}

	// page 1: sphere hint
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		pl->setContentsMargins(0, 4, 0, 0);
		sphere_info = new QLabel(
			"Light directions will be computed from the reflective sphere(s) "
			"marked in the main window.\n\n"
			"Make sure at least one sphere is configured and highlights have been detected. "
			"Lens-distortion correction (see the Distortion tab) is applied automatically.");
		sphere_info->setWordWrap(true);
		pl->addWidget(sphere_info);
		pl->addStretch();
		source_stack->addWidget(page);
	}

	source_lay->addWidget(source_stack);
	layout->addWidget(source_box);

	connect(src_bg, QOverload<int>::of(&QButtonGroup::idClicked),
	        source_stack, &QStackedWidget::setCurrentIndex);

	// ---- Dome geometry (conditional) ------------------------------------
	geom_box = new QGroupBox("Dome geometry");
	QFormLayout *geom_form = new QFormLayout(geom_box);

	sb_radius = new QDoubleSpinBox;
	sb_radius->setRange(1.0, 10000.0);
	sb_radius->setDecimals(1);
	sb_radius->setSuffix(" mm");
	sb_radius->setValue(300.0);
	geom_form->addRow("Dome radius:", sb_radius);

	sb_offset = new QDoubleSpinBox;
	sb_offset->setRange(-5000.0, 5000.0);
	sb_offset->setDecimals(1);
	sb_offset->setSuffix(" mm");
	sb_offset->setValue(0.0);
	geom_form->addRow("Vertical offset:", sb_offset);

	sb_width = new QDoubleSpinBox;
	sb_width->setRange(0.0, 100000.0);
	sb_width->setDecimals(3);
	sb_width->setSuffix(" mm");
	sb_width->setSpecialValueText("Unknown");
	geom_form->addRow("Image width:", sb_width);

	layout->addWidget(geom_box);
	layout->addStretch(1);

	// model changes update geometry visibility and write to session
	connect(model_bg, QOverload<int>::of(&QButtonGroup::idClicked),
	        this, &CalLightsFrame::modelChanged);
	connect(sb_radius, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, &CalLightsFrame::uiToSession);
	connect(sb_offset, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, &CalLightsFrame::uiToSession);
	connect(sb_width,  QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, &CalLightsFrame::uiToSession);

	modelChanged(); // set initial geometry visibility
}

void CalLightsFrame::modelChanged() {
	bool spherical   = radio_sph->isChecked();
	bool positional  = radio_3d->isChecked();
	bool show_geom   = spherical || positional;

	geom_box->setVisible(show_geom);
	// radius row: only needed for Spherical
	if (geom_box->layout()) {
		QFormLayout *form = qobject_cast<QFormLayout*>(geom_box->layout());
		if (form) {
			// row 0 is radius
			form->itemAt(0, QFormLayout::LabelRole)->widget()->setVisible(spherical);
			form->itemAt(0, QFormLayout::FieldRole)->widget()->setVisible(spherical);
		}
	}
	uiToSession();
}

void CalLightsFrame::setSession(CalibrationSession *s) {
	session = s;
	sessionToUi();
}

void CalLightsFrame::sessionToUi() {
	if (!session) return;
	const Dome &d = session->dome;
	switch (d.lightConfiguration) {
		case Dome::DIRECTIONAL: radio_dir->setChecked(true); break;
		case Dome::SPHERICAL:   radio_sph->setChecked(true); break;
		case Dome::LIGHTS3D:    radio_3d->setChecked(true);  break;
	}
	sb_radius->setValue(d.domeDiameter / 2.0);
	sb_offset->setValue(d.verticalOffset);
	sb_width->setValue(d.imageWidth);
	modelChanged();
}

void CalLightsFrame::uiToSession() {
	if (!session) return;
	Dome &d = session->dome;
	if (radio_sph->isChecked())      d.lightConfiguration = Dome::SPHERICAL;
	else if (radio_3d->isChecked())  d.lightConfiguration = Dome::LIGHTS3D;
	else                             d.lightConfiguration = Dome::DIRECTIONAL;

	d.domeDiameter  = sb_radius->value() * 2.0;
	d.verticalOffset = sb_offset->value();
	d.imageWidth    = sb_width->value();
}

void CalLightsFrame::browseLP() {
	QString path = QFileDialog::getOpenFileName(
		this, "Select LP file", QString(), "LP files (*.lp);;All files (*)");
	if (!path.isEmpty())
		lp_file_path->setText(path);
}

void CalLightsFrame::loadLP() {
	QString path = QFileDialog::getOpenFileName(
		this, "Load LP file", QString(), "LP files (*.lp);;All files (*)");
	if (path.isEmpty() || !session) return;
	session->loadLP(path);
	sessionToUi();
}

void CalLightsFrame::exportLP() {
	if (!session) return;
	QString path = QFileDialog::getSaveFileName(
		this, "Export LP file", "lights.lp", "LP files (*.lp);;All files (*)");
	if (!path.isEmpty())
		session->saveLP(path);
}



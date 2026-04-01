#include "lightgeometry.h"
#include "relightapp.h"
#include "helpbutton.h"
#include "directionsview.h"
#include "../src/sphere.h"
#include "../src/exif.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

LightsGeometry::~LightsGeometry() { if(group) delete group; }


LightsGeometry::LightsGeometry(QWidget *parent): QFrame(parent) {

	QVBoxLayout *page = new QVBoxLayout(this);

	page->addWidget(new QLabel("<h3>Current lights configuration<h3>"));
	//page->addSpacing(10);

	QGridLayout * content = new QGridLayout();
	page->addLayout(content);

	content->addWidget( new QLabel("Filename:"), 0, 0);
	content->addWidget(filename = new QLineEdit, 0, 1);
	filename->setEnabled(false);

	content->addWidget( new QLabel("Number of images:"), 1, 0);
	content->addWidget(images_number = new QSpinBox, 1, 1);
	images_number->setRange(1, 1024);
	images_number->setEnabled(false);

	content->addWidget(new QLabel("Notes:"), 2, 0);
	content->addWidget(notes = new QTextEdit, 2, 1);
	notes->setMaximumHeight(100);
	connect(notes, &QTextEdit::textChanged, [&]() { qRelightApp->project().dome.notes = notes->toPlainText(); });



	group = new QButtonGroup;

	content->addWidget(directional = new HelpRadio("Directional Lights", "lights/directional"), 3, 0);
	content->addWidget(sphere_approx = new HelpRadio("3D light positions on a sphere", "lights/3dsphere"), 4, 0);
	content->addWidget(lights3d = new HelpRadio("3D light positions", "lights/3dposition"), 5, 0);
	group->addButton(directional->radioButton(), Dome::DIRECTIONAL);
	group->addButton(sphere_approx->radioButton(), Dome::SPHERICAL);
	group->addButton(lights3d->radioButton(), Dome::LIGHTS3D);

	connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(setSpherical(QAbstractButton *)));

	QFrame *geometry = new QFrame;
	geometry->setFrameShape(QFrame::StyledPanel);

	content->addWidget(geometry, 3, 1, 3, 1);

	QGridLayout *grid = new QGridLayout(geometry);
	grid->setColumnMinimumWidth(0, 200);
	grid->addWidget(new QLabel("Image width:"), 2, 0);
	grid->addWidget(image_width = new QDoubleSpinBox, 2, 1);
	image_width->setKeyboardTracking(false);
	image_width->setRange(0, 10000);
	grid->addWidget(new QLabel("mm"), 2, 2);
	connect(image_width, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) {
		auto &project = qRelightApp->project();
		project.dome.imageWidth = v;
		project.pixelSize = project.dome.imageWidth/project.imgsize.width();
		qRelightApp->project().needs_saving = true;
		recomputeGeometry();
	});

	grid->addWidget(new QLabel("Dome radius:"), 3, 0);
	grid->addWidget(radius = new QDoubleSpinBox, 3, 1);
	radius->setKeyboardTracking(false);
	radius->setRange(0, 10000);
	grid->addWidget(new QLabel("mm"), 3, 2);
	connect(radius, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) {
		qRelightApp->project().dome.domeDiameter = v*2.0;
		qRelightApp->project().needs_saving = true;
		recomputeGeometry();
	});

	grid->addWidget(new QLabel("Vertical offset:"), 4, 0);
	grid->addWidget(vertical_offset = new QDoubleSpinBox, 4, 1);
	vertical_offset->setKeyboardTracking(false);
	vertical_offset->setRange(-1000, 1000);
	grid->addWidget(new QLabel("mm"), 4, 2);
	connect(vertical_offset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) {
		qRelightApp->project().dome.verticalOffset = v;
		qRelightApp->project().needs_saving = true;
		recomputeGeometry();
	});


	/* it seems basically impossible to have a widget scale while preserving aspect ratio, bummer */

	content->setSpacing(20);
	directions_view = new DirectionsView;
	content->addWidget(directions_view, 0, 2, 3, 1, Qt::AlignBottom);
	directions_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	directions_view->setMaximumSize(200, 200);
	directions_view->setMinimumSize(200, 200);

	page->addSpacing(30);

	// ---- Lens panel ----
	auto *lensFrame = new QFrame;
	lensFrame->setFrameShape(QFrame::StyledPanel);
	page->addWidget(lensFrame);

	auto *lensLayout = new QGridLayout(lensFrame);
	lensLayout->setColumnStretch(1, 1);

	auto *lensTitle = new QLabel("<b>Lens</b>");
	lensLayout->addWidget(lensTitle, 0, 0, 1, 3);

	QLabel *focal_label = new QLabel("Focal length (35mm equivalent):");
	focal_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	focal_label->setMaximumSize(300, 300);
	focal_label->setMinimumSize(300, 300);

	lensLayout->addWidget(focal_label, 1, 0);
	focal_length = new QDoubleSpinBox;
	focal_length->setKeyboardTracking(false);
	focal_length->setRange(0, 2000);
	focal_length->setDecimals(2);
	focal_length->setSuffix(" mm");
	focal_length->setSpecialValueText("Unknown");
	focal_length->setMinimumWidth(120);
	lensLayout->addWidget(focal_length, 1, 1);

	auto *readBtn = new QPushButton("Read from images");
	lensLayout->addWidget(readBtn, 1, 2);

	connect(focal_length, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double v) {
		qRelightApp->project().lens.focalLength = v;
		qRelightApp->project().lens.focal35equivalent = true;
		qRelightApp->project().needs_saving = true;
	});
	connect(readBtn, &QPushButton::clicked, this, &LightsGeometry::readFocalLength);

	page->addStretch();
}

void LightsGeometry::setSpherical(QAbstractButton *button) {
	Dome &dome = qRelightApp->project().dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;
	qRelightApp->project().needs_saving = true;

	bool spherical = (button == sphere_approx->radioButton());
	radius->setEnabled(spherical == Dome::SPHERICAL);
	vertical_offset->setEnabled(spherical == Dome::SPHERICAL);

	if(button == sphere_approx->radioButton()) {
		dome.lightConfiguration = Dome::SPHERICAL;
	} else if(button == lights3d->radioButton()) {
		dome.lightConfiguration = Dome::LIGHTS3D;
	}
	if(!qRelightApp->project().lens.focalLength && dome.lightConfiguration != Dome::DIRECTIONAL) {
		QMessageBox::warning(this, "Missing focal lenght.", "The images do not contain informations about the focal length,"
							 "it will not be possible to use geometric information to correct threedimensional light posisions.");
	}
	init();
}

void LightsGeometry::init() {
	Dome &dome = qRelightApp->project().dome;

	filename->setText(dome.label);
	notes->setText(dome.notes);
	images_number->setValue(dome.lightsCount());
	group->button(dome.lightConfiguration)->setChecked(true);

	bool spherical = dome.lightConfiguration == Dome::SPHERICAL;
	radius->setEnabled(spherical);
	vertical_offset->setEnabled(spherical);

	//stop signals!
	image_width->blockSignals(true);
	radius->blockSignals(true);
	vertical_offset->blockSignals(true);
	image_width->setValue(dome.imageWidth);
	radius->setValue(dome.domeDiameter/2.0);
	vertical_offset->setValue(dome.verticalOffset);

	image_width->blockSignals(false);
	radius->blockSignals(false);
	vertical_offset->blockSignals(false);

	directions_view->initFromDome(dome);

	if (focal_length) {
		focal_length->blockSignals(true);
		focal_length->setValue(qRelightApp->project().lens.focal35());
		focal_length->blockSignals(false);
	}
}


void LightsGeometry::setFromSpheres() {
	//get spheres & lens from project
	Project &project = qRelightApp->project();
	//call appropriate compute directions/positions
	Dome &dome = project.dome;
	dome.label = "";
	dome.lightSource = Dome::FROM_SPHERES;
	dome.fromSpheres(project.images, project.spheres, project.lens);

	init();
}

void LightsGeometry::exportDome() {
	QString filename = QFileDialog::getSaveFileName(this, "Select a dome file", qRelightApp->lastProjectDir(), "*.dome");
	if(filename.isNull())
		return;
	if(!filename.endsWith(".dome"))
		filename += ".dome";
	//TODO Basic checks, label is a problem (use filename!
	Dome &dome = qRelightApp->project().dome;
	dome.save(filename);
	qRelightApp->addDome(filename);
}

void LightsGeometry::readFocalLength() {
	Project &project = qRelightApp->project();

	// Find the first valid image
	const Image *first = nullptr;
	for (const Image &image : project.images)
		if (image.valid) { first = &image; break; }

	if (!first) {
		QMessageBox::warning(this, "No images", "No valid images in the project.");
		return;
	}

	QString path = project.dir.filePath(first->filename);
	try {
		Exif exif;
		exif.parse(path);
		project.lens.readExif(exif);
	} catch (QString &) {
		QMessageBox::warning(this, "EXIF error",
			QString("Could not read EXIF data from:\n%1").arg(path));
		return;
	}

	if (project.lens.focalLength == 0) {
		QMessageBox::warning(this, "No focal length",
			QString("The image does not contain focal length information:\n%1").arg(path));
		return;
	}


	project.needs_saving = true;

	focal_length->blockSignals(true);
	focal_length->setValue(project.lens.focal35());
	focal_length->blockSignals(false);
	recomputeGeometry();
}

void LightsGeometry::recomputeGeometry() {
	Project &project = qRelightApp->project();
	Dome &dome = project.dome;
	
	// When geometry parameters change, recompute positions for both sources
	if(dome.lightSource == Dome::FROM_SPHERES) {
		if(project.spheres.size() == 0) {
			QMessageBox::warning(this, "No spheres available",
				"Cannot recompute geometry: light directions were computed from spheres, "
				"but no spheres are defined.\n"
				"Please add spheres or load light directions from a dome/LP file.");
			return;
		}
		dome.fromSpheres(project.images, project.spheres, project.lens);
	} else if(dome.lightSource == Dome::FROM_LP) {
			dome.recomputePositions();
	}	
	init();
}

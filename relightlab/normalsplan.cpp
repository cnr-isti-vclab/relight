#include "normalsplan.h"
#include "qlabelbutton.h"
#include "helpbutton.h"
#include "relightapp.h"

#include <QButtonGroup>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QFileDialog>

NormalsPlanRow::NormalsPlanRow(NormalsParameters &_parameters, QFrame *parent):
	PlanRow(parent), parameters(_parameters) {

}


NormalsSourceRow::NormalsSourceRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Source:");
	label->help->setId("normals/normalmap");

	compute = new QLabelButton("Compute", "Compute normals from images");
	buttons->addWidget(compute, 0, Qt::AlignCenter);


	{
		QVBoxLayout *button_layout =new QVBoxLayout;
		file = new QLabelButton("Normalmap", "Load a normalmap image.");

		button_layout->addWidget(file);
		{
			QHBoxLayout *loader_layout = new QHBoxLayout;
			loader_layout->addWidget(input_path = new QLineEdit);
			loader_layout->addWidget(open = new QPushButton("..."));
			button_layout->addLayout(loader_layout);
			connect(open, &QPushButton::clicked, this, &NormalsSourceRow::selectOutput);

		}
		buttons->addLayout(button_layout, 0); //, Qt::AlignCenter);
	}

	connect(compute, &QAbstractButton::clicked, this, [this](){ setComputeSource(true); });
	connect(file, &QAbstractButton::clicked, this, [this](){ setComputeSource(false); });
	//connect(rbf, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::RBF, true); });
	//connect(bln, &QAbstractButton::clicked, this, [this](){ setBasis(Rti::BILINEAR, true); });

	QButtonGroup *group = new QButtonGroup(this);

	group->addButton(compute);
	group->addButton(file);

	setComputeSource(parameters.compute);
}


void NormalsSourceRow::setComputeSource(bool build) {
	parameters.compute = build;
	compute->setChecked(build);
}


void NormalsSourceRow::selectOutput() {
	//get folder if not legacy.
	QString output_parent = qRelightApp->lastOutputDir();

	QString output = QFileDialog::getOpenFileName(this, "Select an output folder", output_parent);
	if(output.isNull()) return;

	QDir output_parent_dir(output);
	output_parent_dir.cdUp();
	setSourcePath(output);
}

void NormalsSourceRow::setSourcePath(QString path) {
	parameters.input_path = path;
	input_path->setText(path);
}




NormalsFlattenRow::NormalsFlattenRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Flatten:");
	label->help->setId("normals/flattening");

	none = new QLabelButton("None", "Do not flatten the surface");
	radial = new QLabelButton("Radial", "Polynomial radial fitting.");
	fourier = new QLabelButton("Fourier", "Remove low frequencies");

	buttons->addWidget(none, 1, Qt::AlignCenter);
	buttons->addWidget(radial, 1, Qt::AlignCenter);

	QVBoxLayout *button_layout =new QVBoxLayout;
	button_layout->addWidget(fourier);

	QHBoxLayout *loader_layout = new QHBoxLayout;
	loader_layout->addWidget(new QLabel("Fourier low pass frequency."));
	loader_layout->addWidget(max_frequency = new QDoubleSpinBox);
	max_frequency->setRange(0, 100);
	max_frequency->setDecimals(4);
	max_frequency->setValue(parameters.flatPercentage);

	button_layout->addLayout(loader_layout);


	buttons->addLayout(button_layout, 1); //, Qt::AlignCenter);

	connect(none, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_NONE); });
	connect(radial, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_RADIAL); });
	connect(fourier, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_FOURIER); });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(max_frequency, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.flatPercentage = v; });
#else
	connect(max_frequency, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.flatPercentage = v; });
#endif


	QButtonGroup *group = new QButtonGroup(this);

	group->addButton(none);
	group->addButton(radial);
	group->addButton(fourier);

	setFlattenMethod(parameters.flatMethod);
}

void NormalsFlattenRow::setFlattenMethod(FlatMethod method) {
	parameters.flatMethod = method;
	none->setChecked(method == FLAT_NONE);
	radial->setChecked(method == FLAT_RADIAL);
	fourier->setChecked(method == FLAT_FOURIER);
}

void NormalsFlattenRow::setFourierFrequency(double f) {
	parameters.flatPercentage = f;
	max_frequency->setValue(f);
}



NormalsSurfaceRow::NormalsSurfaceRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Surface:");
	label->help->setId("normals/flattening");

	none = new QLabelButton("None", "Do generate a mesh.");
	fft = new QLabelButton("Fourier Normal Integration", "Dense, very fast");
	bni = new QLabelButton("Bilateral Normal Integration", "Dense, allows discontinuity");
	assm = new QLabelButton("Adaptive Surface Meshing.", "Adaptive, no discontinuities.");

	buttons->addWidget(none, 0, Qt::AlignCenter);

	buttons->addWidget(fft);
	{
		QVBoxLayout *bni_layout =new QVBoxLayout;
		bni_layout->addWidget(bni);
		{
			QHBoxLayout *bni_parameter = new QHBoxLayout;
			bni_parameter->addWidget(new QLabel("Discontinuity propensity."));
			bni_parameter->addWidget(bni_k = new QDoubleSpinBox);
			bni_k->setRange(0.01, 50);
			bni_k->setValue(parameters.bni_k);

			bni_layout->addLayout(bni_parameter);
		}
		buttons->addLayout(bni_layout, 0); //, Qt::AlignCenter);
	}

	{
		QVBoxLayout *assm_layout =new QVBoxLayout;
		assm_layout->addWidget(assm);

		{
			QHBoxLayout *assm_parameter = new QHBoxLayout;
			assm_parameter->addWidget(new QLabel("Mesh error in pixels.."));
			assm_parameter->addWidget(assm_error = new QDoubleSpinBox);
			assm_error->setRange(0.001, 100);
			assm_error->setValue(parameters.assm_error);

			assm_layout->addLayout(assm_parameter);
		}
		buttons->addLayout(assm_layout, 0); //, Qt::AlignCenter);
	}

	connect(none, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_NONE); });
	connect(fft, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_FFT); });
	connect(bni, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_BNI); });
	connect(assm, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_ASSM); });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(bni_k, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.bni_k = v; });
	connect(assm_error, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.assm_error = v; });
#else
	connect(bni_k, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.bni_k = v; });
	connect(assm_error, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.assm_error = v; });
#endif

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(none);
	group->addButton(fft);
	group->addButton(bni);
	group->addButton(assm);

	setSurfaceMethod(parameters.surface_integration);
}

void NormalsSurfaceRow::setSurfaceMethod(SurfaceIntegration surface) {
	parameters.surface_integration = surface;
	none->setChecked(surface == SURFACE_NONE);
	bni->setChecked(surface == SURFACE_BNI);
	assm->setChecked(surface == SURFACE_ASSM);
}



NormalsExportRow::NormalsExportRow(NormalsParameters &parameters, QFrame *parent): NormalsPlanRow(parameters, parent) {
	label->label->setText("Directory/File:");
	label->help->setId("normals/export");

	path_edit = new QLineEdit;
	connect(path_edit, &QLineEdit::editingFinished,this, &NormalsExportRow::verifyPath);
	buttons->addWidget(path_edit);
	QPushButton *path_button = new QPushButton("...");
	buttons->addWidget(path_button);
	connect(path_button, &QPushButton::clicked, this, &NormalsExportRow::selectOutput);
}

void NormalsExportRow::setPath(QString path, bool emitting) {
	path_edit->setText(path);
	parameters.path = path;
}

void NormalsExportRow::verifyPath() {
	parameters.path = QString();
	QString path = path_edit->text();
	QDir path_dir(path);
	path_dir.cdUp();
	if(!path_dir.exists()) {
		QMessageBox::warning(this, "Invalid output path", "The specified path is not valid");
		return;
	}
	if(!path.endsWith(".jpg") && !path.endsWith(".png")) {
		path += ".jpg";
		path_edit->setText(path);
	}
	parameters.path = path;
}

void NormalsExportRow::selectOutput() {
	//get folder if not legacy.
	QString output_parent = qRelightApp->lastOutputDir();

	QString output = QFileDialog::getSaveFileName(this, "Select a file name", output_parent);
	if(output.isNull()) return;

	if(!output.endsWith(".jpg") && !output.endsWith(".png"))
			output += ".jpg";

	QDir output_parent_dir(output);
	output_parent_dir.cdUp();
	qRelightApp->setLastOutputDir(output_parent_dir.absolutePath());
	setPath(output);
}


void NormalsExportRow::suggestPath() {
	QDir input = qRelightApp->project().dir;
	input.cdUp();
	QString filename = input.filePath("normals.jpg");
	setPath(filename);
}


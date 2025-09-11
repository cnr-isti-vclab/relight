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

	file = new QLabelButton("Normalmap", "Load a normalmap image.");
	buttons->addWidget(file, 0, Qt::AlignCenter);

	{
		input_frame = new QFrame;
		QHBoxLayout *input_layout = new QHBoxLayout(input_frame);
		input_layout->addWidget(new QLabel("File path:"));
		input_layout->addWidget(input_path = new QLineEdit);
		input_layout->addWidget(open = new QPushButton("..."));
		planLayout->addWidget(input_frame);

		connect(open, &QPushButton::clicked, this, &NormalsSourceRow::selectOutput);
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

	input_frame->setVisible(!build);
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
	gaussian = new QLabelButton("Blur", "Subtract gaussian blur");

	buttons->addWidget(none, 1, Qt::AlignCenter);
	buttons->addWidget(radial, 1, Qt::AlignCenter);
	buttons->addWidget(fourier, 1, Qt::AlignCenter);
	buttons->addWidget(gaussian, 1, Qt::AlignCenter);


	{
		frequency_frame = new QFrame;
		QHBoxLayout *frequency_layout = new QHBoxLayout(frequency_frame);

		frequency_layout->addWidget(new HelpLabel("Fourier high pass frequency %", "normals/flattening#fourier"));
		frequency_layout->addWidget(max_frequency = new QDoubleSpinBox);
		max_frequency->setRange(0, 100);
		max_frequency->setDecimals(4);
		max_frequency->setValue(parameters.flatPercentage);

		planLayout->addWidget(frequency_frame);
	}


	{
		blur_frame = new QFrame;
		QHBoxLayout *blur_layout = new QHBoxLayout(blur_frame);

		blur_layout->addWidget(new HelpLabel("Blur %", "normals/flattening#blur"));
		blur_layout->addWidget(blur_percentage = new QDoubleSpinBox);
		blur_percentage->setRange(0, 100);
		blur_percentage->setDecimals(4);
		blur_percentage->setValue(parameters.blurPercentage);
		planLayout->addWidget(blur_frame);
	}


	connect(none, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_NONE); });
	connect(radial, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_RADIAL); });
	connect(fourier, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_FOURIER); });
	connect(gaussian, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_BLUR); });


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(max_frequency, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.flatPercentage = v; });
#else
	connect(max_frequency, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.flatPercentage = v; });
	connect(blur_percentage, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.blurPercentage = v; });

#endif


	QButtonGroup *group = new QButtonGroup(this);

	group->addButton(none);
	group->addButton(radial);
	group->addButton(fourier);
	group->addButton(gaussian);

	setFlattenMethod(parameters.flatMethod);
}

void NormalsFlattenRow::setFlattenMethod(FlatMethod method) {
	parameters.flatMethod = method;
	none->setChecked(method == FLAT_NONE);
	radial->setChecked(method == FLAT_RADIAL);
	fourier->setChecked(method == FLAT_FOURIER);
	gaussian->setChecked(method == FLAT_BLUR);

	frequency_frame->setVisible(method == FLAT_FOURIER);
	blur_frame->setVisible(method == FLAT_BLUR);
}

void NormalsFlattenRow::setFourierFrequency(double f) {
	parameters.flatPercentage = f;
	max_frequency->setValue(f);
}

void NormalsFlattenRow::setBlurFrequency(double f) {
	parameters.blurPercentage = f;
	blur_percentage->setValue(f);
}



NormalsSurfaceRow::NormalsSurfaceRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Surface:");
	label->help->setId("normals/flattening");


	none = new QLabelButton("None", "Do not generate a mesh.");
	fft = new QLabelButton("Fourier Normal Integration", "Dense, very fast");
	bni = new QLabelButton("Bilateral Normal Integration", "Dense, allows discontinuity");
	assm = new QLabelButton("Adaptive Surface Meshing.", "Adaptive, no discontinuities.");

	buttons->addWidget(none, 0, Qt::AlignCenter);
	buttons->addWidget(fft);
	buttons->addWidget(bni); //, Qt::AlignCenter);
	buttons->addWidget(assm);


	//add surface downsampling header.
	{
		downsample_frame = new QFrame;
		QGridLayout *downsample_layout = new QGridLayout(downsample_frame);
		downsample_layout->setSpacing(6); //default spacing, it was modified by parent and inheritance
		downsample_layout->addWidget(new QLabel("Surface downsampling: "), 0, 0);
		downsample = new QDoubleSpinBox();
		downsample->setMaximum(100);
		downsample->setMinimum(1);
		downsample_layout->addWidget(downsample, 0, 1);
		downsample_layout->addWidget(new QLabel("Width:"), 1, 0);
		downsample_layout->addWidget(width = new QSpinBox(), 1, 1);
		width->setRange(1, 64000);
		downsample_layout->addWidget(new QLabel("Height:"), 2, 0);
		downsample_layout->addWidget(height = new QSpinBox(), 2, 1);
		height->setRange(1, 64000);


		planLayout->addWidget(downsample_frame);
	}
	{
		bni_frame = new QFrame;
		QGridLayout *bni_layout = new QGridLayout(bni_frame);
		bni_layout->setSpacing(6); //default spacing, it was modified by parent and inheritance

		bni_layout->addWidget(new HelpLabel("Discontinuity propensity.", "normals/surface#bni"), 0, 0);
		bni_layout->addWidget(bni_k = new QDoubleSpinBox, 0, 1);
		bni_k->setRange(0.00, 50);
		bni_k->setValue(parameters.bni_k);

		planLayout->addWidget(bni_frame);
	}

	{
		assm_frame = new QFrame;
		QGridLayout *assm_layout = new QGridLayout(assm_frame);
		assm_layout->setSpacing(6); //default spacing, it was modified by parent and inheritance

		assm_layout->addWidget(new HelpLabel("Mesh error in pixels..", "normals/surface#assm"), 0, 0);
		assm_layout->addWidget(assm_error = new QDoubleSpinBox, 0, 1);
		assm_error->setRange(0.001, 100);
		assm_error->setValue(parameters.assm_error);

		planLayout->addWidget(assm_frame);
	}


	connect(none, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_NONE); });
	connect(fft, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_FFT); });
	connect(bni, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_BNI); });
	connect(assm, &QAbstractButton::clicked, this, [this](){ setSurfaceMethod(SURFACE_ASSM); });

	connect(downsample, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) {
		float d = downsample->value();

		Project &p = qRelightApp->project();
		setDownsample(d, p.imgsize.width()/d, p.imgsize.height()/d);
	});

	connect(width, qOverload<int>(&QSpinBox::valueChanged), this, [this](int w) {
		//TODO basic validation?
		Project &p = qRelightApp->project();
		float d = p.imgsize.width()/(float)w;

		setDownsample(d, w, p.imgsize.height()/d);
	});

	connect(height, qOverload<int>(&QSpinBox::valueChanged), this, [this](int h) {
		//TODO basic validation?
		Project &p = qRelightApp->project();
		float d = p.imgsize.height()/(float)h;

		setDownsample(d, p.imgsize.width()/d, h);
	});

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

void NormalsSurfaceRow::init() {
	Project &p = qRelightApp->project();
	setDownsample(1.0f, p.imgsize.width(), p.imgsize.height());
}


void NormalsSurfaceRow::setSurfaceMethod(SurfaceIntegration surface) {
	parameters.surface_integration = surface;
	none->setChecked(surface == SURFACE_NONE);
	bni->setChecked(surface == SURFACE_BNI);
	assm->setChecked(surface == SURFACE_ASSM);

	downsample_frame->setVisible(surface == SURFACE_BNI || surface == SURFACE_FFT);
	bni_frame->setVisible(surface == SURFACE_BNI);
	assm_frame->setVisible(surface == SURFACE_ASSM);
}

void NormalsSurfaceRow::setDownsample(float down, int w, int h) {
	downsample->blockSignals(true);
	width->blockSignals(true);
	height->blockSignals(true);

	downsample->setValue(down);
	width->setValue(w);
	height->setValue(h);

	downsample->blockSignals(false);
	width->blockSignals(false);
	height->blockSignals(false);

	parameters.surface_width = w;
	parameters.surface_height = h;
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


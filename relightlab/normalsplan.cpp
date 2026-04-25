#include "normalsplan.h"
#include "planepicking.h"
#include "reflectionview.h"
#include "qlabelbutton.h"
#include "helpbutton.h"
#include "relightapp.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <cmath>
/*
We are going to add a few functionalities to the normals tab.

for the first row, we show additional options and parameterzs when compute or load button is selected similarly how parameters can be set in the Surface row (downsampline etc).

we have a single compute vs load button.
when compute is selected we can specify the algorithm (l2, robust) in a combo box
when load is selected we can specify if it is opengl or directx format.*/

NormalsPlanRow::NormalsPlanRow(NormalsParameters &_parameters, QFrame *parent):
	PlanRow(parent), parameters(_parameters) {

}


NormalsSourceRow::NormalsSourceRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Source:");
	label->help->setId("normals/normalmap");

	compute = new QLabelButton("Compute", "Compute normals from the image set.");
	load    = new QLabelButton("Load",    "Load an existing normalmap image.");

	buttons->addWidget(compute, 0, Qt::AlignCenter);
	buttons->addWidget(load,    0, Qt::AlignCenter);

	{
		compute_frame = new QFrame;
		QHBoxLayout *compute_layout = new QHBoxLayout(compute_frame);
		HelpLabel *algo_label = new HelpLabel("Algorithm:", "normals/normalmap#algorithm");
		algo_label->setFixedWidth(200);
		compute_layout->addWidget(algo_label);
		compute_layout->addWidget(solver_combo = new QComboBox);
		solver_combo->setFixedWidth(200);
		solver_combo->setProperty("class", "large");

		solver_combo->addItem("L2 (Least Squares)", QVariant(NORMALS_L2));
		solver_combo->addItem("Robust",              QVariant(NORMALS_ROBUST));
		compute_layout->addStretch(1);
		planLayout->addWidget(compute_frame);

		connect(solver_combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
			setSolver((NormalSolver)solver_combo->currentData().toInt());
		});
	}

	{
		load_frame = new QFrame;
		QVBoxLayout *load_layout = new QVBoxLayout(load_frame);
		load_layout->setContentsMargins(0, 0, 0, 0);

		QHBoxLayout *path_layout = new QHBoxLayout;
		HelpLabel *path_label = new HelpLabel("File path:", "normals/normalmap");
		path_label->setFixedWidth(200);
		path_layout->addWidget(path_label);
		path_layout->addWidget(input_path = new QLineEdit);
		path_layout->addWidget(open = new QPushButton("..."));
		load_layout->addLayout(path_layout);

		QHBoxLayout *format_layout = new QHBoxLayout;
		HelpLabel *fmt_label = new HelpLabel("Format:", "normals/normalmap#format");
		fmt_label->setFixedWidth(200);
		format_layout->addWidget(fmt_label);
		format_layout->addWidget(format_combo = new QComboBox);
		format_combo->setFixedWidth(200);
		format_combo->setProperty("class", "large");

		format_combo->addItem("OpenGL",  QVariant(NORMAL_OPENGL));
		format_combo->addItem("DirectX", QVariant(NORMAL_DIRECTX));
		format_layout->addStretch(1);
		load_layout->addLayout(format_layout);

		planLayout->addWidget(load_frame);

		connect(open, &QPushButton::clicked, this, &NormalsSourceRow::selectOutput);
		connect(format_combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
			setFormat((NormalFormat)format_combo->currentData().toInt());
		});
	}

	connect(compute, &QAbstractButton::clicked, this, [this](){ setComputeSource(true); });
	connect(load,    &QAbstractButton::clicked, this, [this](){ setComputeSource(false); });

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(compute);
	group->addButton(load);

	setComputeSource(parameters.compute);
	setSolver(parameters.solver);
	setFormat(parameters.normalFormat);
}


void NormalsSourceRow::setComputeSource(bool build) {
	parameters.compute = build;
	compute->setChecked(build);
	load->setChecked(!build);

	compute_frame->setVisible(build);
	load_frame->setVisible(!build);
	updateSize();
}

void NormalsSourceRow::setSolver(NormalSolver solver) {
	parameters.solver = solver;
	int idx = solver_combo->findData(QVariant(solver));
	if(idx >= 0) {
		solver_combo->blockSignals(true);
		solver_combo->setCurrentIndex(idx);
		solver_combo->blockSignals(false);
	}
}

void NormalsSourceRow::setFormat(NormalFormat format) {
	parameters.normalFormat = format;
	int idx = format_combo->findData(QVariant(format));
	if(idx >= 0) {
		format_combo->blockSignals(true);
		format_combo->setCurrentIndex(idx);
		format_combo->blockSignals(false);
	}
}

void NormalsSourceRow::updateSize() {
	int w = 0, h = 0;
	if (parameters.compute) {
		Project &p = qRelightApp->project();
		w = p.crop.width();
		h = p.crop.height();
	} else {
		QImageReader reader(parameters.input_path);
		QSize size = reader.size();
		if (size.isValid()) {
			w = size.width();
			h = size.height();
		}
	}
	if(w > 0 && h > 0) {
		emit sourceSizeChanged(w, h);
	}
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
	updateSize();
}




NormalsFlattenRow::NormalsFlattenRow(NormalsParameters &_parameters, QFrame *parent):
	NormalsPlanRow(_parameters, parent) {
	label->label->setText("Flatten:");
	label->help->setId("normals/flattening");

	none    = new QLabelButton("None",   "Do not flatten the surface.");
	radial  = new QLabelButton("Radial", "Polynomial radial fitting.");
	plane   = new QLabelButton("Plane",  "4-point plane flattening.");
	gaussian = new QLabelButton("Blur",  "Subtract gaussian blur.");

	buttons->addWidget(none,     1, Qt::AlignCenter);
	buttons->addWidget(radial,   1, Qt::AlignCenter);
	buttons->addWidget(plane,    1, Qt::AlignCenter);
	buttons->addWidget(gaussian, 1, Qt::AlignCenter);

	// ── Plane flattening options ─────────────────────────────────────────────
	{
		plane_frame = new QFrame;
		QHBoxLayout *plane_layout = new QHBoxLayout(plane_frame);
		plane_layout->setContentsMargins(0, 0, 0, 0);

		QPushButton *pick_btn = new QPushButton("Pick 4 points...");
		pick_btn->setToolTip("Open the point-picking dialog to select 4 reference points on a flat surface.");
		pick_btn->setProperty("class", "large");
		pick_btn->setFixedWidth(200);
		plane_layout->addWidget(pick_btn);

		plane_overview = new PlaneOverview(80);
		plane_overview->setPoints(parameters.plane_points);
		plane_layout->addWidget(plane_overview);

		plane_layout->addStretch(1);

		planLayout->addWidget(plane_frame);
		connect(pick_btn, &QPushButton::clicked, this, &NormalsFlattenRow::pickPlanePoints);
	}

	// ── Blur options ─────────────────────────────────────────────────────────
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

	connect(none,    &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_NONE); });
	connect(radial,  &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_RADIAL); });
	connect(plane,   &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_PLANE); });
	connect(gaussian, &QAbstractButton::clicked, this, [this](){ setFlattenMethod(FLAT_BLUR); });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	connect(blur_percentage, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.blurPercentage = v; });
#else
	connect(blur_percentage, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double v) { parameters.blurPercentage = v; });
#endif

	QButtonGroup *group = new QButtonGroup(this);
	group->addButton(none);
	group->addButton(radial);
	group->addButton(plane);
	group->addButton(gaussian);

	setFlattenMethod(parameters.flatMethod);
}

// Returns the RMS distance from the fitted circle divided by the circle radius.
// A value close to 0 means the points are nearly co-circular (ill-conditioned for
// sphere fitting).
static double circleResidualFraction(const std::vector<QPointF> &pts)
{
	if(pts.size() < 3) return 1.0;

	double n = pts.size();
	double sx = 0, sy = 0, sxy = 0, sx2 = 0, sy2 = 0;
	double sx3 = 0, sy3 = 0, sx2y = 0, sxy2 = 0;
	for(const QPointF &p : pts) {
		double x = p.x(), y = p.y();
		sx += x;   sy += y;
		sxy += x*y; sx2 += x*x; sy2 += y*y;
		sx3 += x*x*x; sy3 += y*y*y;
		sx2y += x*x*y; sxy2 += x*y*y;
	}
	double d11 = n*sxy  - sx*sy;
	double d20 = n*sx2  - sx*sx;
	double d02 = n*sy2  - sy*sy;
	double denom = 2*(d20*d02 - d11*d11);
	if(std::abs(denom) < 1e-10) return 1.0;
	double d30 = n*sx3  - sx2*sx;
	double d03 = n*sy3  - sy2*sy;
	double d21 = n*sx2y - sx2*sy;
	double d12 = n*sxy2 - sx*sy2;
	double a = ((d30 + d12)*d02 - (d03 + d21)*d11) / denom;
	double b = ((d03 + d21)*d20 - (d30 + d12)*d11) / denom;
	double c = (sx2 + sy2 - 2*a*sx - 2*b*sy) / n;
	double r  = std::sqrt(std::max(0.0, c + a*a + b*b));
	if(r < 1e-6) return 1.0;

	double rms = 0;
	for(const QPointF &p : pts) {
		double d = std::hypot(p.x() - a, p.y() - b) - r;
		rms += d*d;
	}
	return std::sqrt(rms / n) / r;
}

void NormalsFlattenRow::pickPlanePoints()
{
	PlanePickingDialog dlg(this);
	dlg.setCrop(qRelightApp->project().crop);
	if(dlg.exec() != QDialog::Accepted)
		return;

	std::vector<QPointF> pts = dlg.getPoints();

	// Warn if the points lie nearly on a circle: the sphere fit becomes ill-conditioned
	// because the sphere centre can slide freely along the axis through the circle.
	const double CIRCLE_THRESHOLD = 0.05; // 5% of fitted circle radius
	if(circleResidualFraction(pts) < CIRCLE_THRESHOLD) {
		QMessageBox::warning(this, "Poor point distribution",
			"The selected points lie nearly on a circle.\n"
			"This makes the sphere fitting ill-conditioned: the estimated curvature\n"
			"will be unreliable.\n\n"
			"Please re-pick the points so they are spread across the surface\n"
			"rather than arranged in a ring.");
	}

	parameters.plane_points = pts;
	plane_overview->setPoints(parameters.plane_points);
}

void NormalsFlattenRow::clear()
{
	parameters.plane_points.clear();
	plane_overview->setPoints(parameters.plane_points);
}

void NormalsFlattenRow::init()
{
	plane_overview->init();
}

void NormalsFlattenRow::setFlattenMethod(FlatMethod method) {
	parameters.flatMethod = method;
	none->setChecked(method == FLAT_NONE);
	radial->setChecked(method == FLAT_RADIAL);
	plane->setChecked(method == FLAT_PLANE);
	gaussian->setChecked(method == FLAT_BLUR);

	plane_frame->setVisible(method == FLAT_PLANE);
	blur_frame->setVisible(method == FLAT_BLUR);
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
		bni_k->setKeyboardTracking(false);
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
		assm_error->setKeyboardTracking(false);
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
		setDownsample(d, base_width/d, base_height/d);
	});

	connect(width, qOverload<int>(&QSpinBox::valueChanged), this, [this](int w) {
		float d = base_width/(float)w;
		setDownsample(d, w, base_height/d);
	});

	connect(height, qOverload<int>(&QSpinBox::valueChanged), this, [this](int h) {
		float d = base_height/(float)h;
		setDownsample(d, base_width/d, h);
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

void NormalsSurfaceRow::setSurfaceMethod(SurfaceIntegration surface) {
	parameters.surface_integration = surface;
	none->setChecked(surface == SURFACE_NONE);
	bni->setChecked(surface == SURFACE_BNI);
	assm->setChecked(surface == SURFACE_ASSM);

	downsample_frame->setVisible(surface == SURFACE_BNI || surface == SURFACE_FFT);
	bni_frame->setVisible(surface == SURFACE_BNI);
	assm_frame->setVisible(surface == SURFACE_ASSM);
}

void NormalsSurfaceRow::updateDimensions(int w, int h) {
	base_width = w;
	base_height = h;
	float down = downsample->value();
	setDownsample(down, w / down, h / down);
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
	parameters.path = path;
}

void NormalsExportRow::selectOutput() {
	//get folder if not legacy.
	QString output_parent = qRelightApp->lastOutputDir();

	QString output = QFileDialog::getSaveFileName(this, "Select a folder name", output_parent);
	if(output.isNull()) return;

	QDir output_parent_dir(output);
	output_parent_dir.cdUp();
	qRelightApp->setLastOutputDir(output_parent_dir.absolutePath());
	setPath(output);
}



void NormalsExportRow::suggestPath() {
	QDir input = qRelightApp->project().dir;
	input.cdUp();
	QString filename = input.filePath("normals");
	setPath(filename);
}


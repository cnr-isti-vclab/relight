#include "caldistortionframe.h"
#include "../../src/exif.h"

#include <lensfun/lensfun.h>
#include <libraw/libraw.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QFileDialog>
#include <QScrollArea>
#include <QSpinBox>
#include <QDir>
#include <QFileInfo>

// Forward declaration — defined at end of file
static cv::Mat loadAsGrayCvMat(const QString &path);

CalDistortionFrame::CalDistortionFrame(QWidget *parent): QFrame(parent) {
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

	// ---- Source selector ------------------------------------------------
	QGroupBox *source_box = new QGroupBox("Distortion correction source");
	QVBoxLayout *source_lay = new QVBoxLayout(source_box);

	radio_none   = new QRadioButton("None \u2013 no distortion correction");
	radio_file   = new QRadioButton("Load from lens.json file (previously saved coefficients)");
	radio_sample = new QRadioButton("Sample photo \u2013 read EXIF data and look up Lensfun profile");
	radio_grid   = new QRadioButton("Calibration grid photo \u2013 compute coefficients from a checkerboard");

	radio_none->setChecked(true);

	QButtonGroup *bg = new QButtonGroup(this);
	bg->addButton(radio_none,   0);
	bg->addButton(radio_file,   1);
	bg->addButton(radio_sample, 2);
	bg->addButton(radio_grid,   3);

	source_lay->addWidget(radio_none);
	source_lay->addWidget(radio_file);
	source_lay->addWidget(radio_sample);
	source_lay->addWidget(radio_grid);
	layout->addWidget(source_box);

	// ---- Stacked parameter area -----------------------------------------
	param_stack = new QStackedWidget;

	// page 0: none — plain label
	{
		QLabel *lbl = new QLabel("No distortion correction will be applied.");
		lbl->setWordWrap(true);
		param_stack->addWidget(lbl);
	}

	// page 1: lens.json file
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		pl->setContentsMargins(0, 4, 0, 0);

		QHBoxLayout *path_row = new QHBoxLayout;
		path_row->addWidget(new QLabel("Lens file:"));
		lens_file_path = new QLineEdit;
		lens_file_path->setPlaceholderText("path/to/lens.json");
		path_row->addWidget(lens_file_path, 1);
		QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
		connect(browse, &QPushButton::clicked, this, &CalDistortionFrame::browseLensFile);
		path_row->addWidget(browse);
		pl->addLayout(path_row);

		QHBoxLayout *btn_row = new QHBoxLayout;
		QPushButton *load_btn = new QPushButton("Load\u2026");
		connect(load_btn, &QPushButton::clicked, this, &CalDistortionFrame::loadLensFile);
		btn_row->addWidget(load_btn);
		QPushButton *save_btn = new QPushButton("Save\u2026");
		connect(save_btn, &QPushButton::clicked, this, &CalDistortionFrame::saveLensFile);
		btn_row->addWidget(save_btn);
		btn_row->addStretch();
		pl->addLayout(btn_row);
		pl->addStretch();

		param_stack->addWidget(page);
	}

	// page 2: sample photo → EXIF → Lensfun
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		pl->setContentsMargins(0, 4, 0, 0);
		pl->setSpacing(8);

		QLabel *hint = new QLabel(
			"Choose any photo taken with the same camera and lens. "
			"EXIF metadata will be read to identify the camera model and focal length, "
			"then the matching lens profile will be looked up in the Lensfun database.");
		hint->setWordWrap(true);
		pl->addWidget(hint);

		QHBoxLayout *path_row = new QHBoxLayout;
		path_row->addWidget(new QLabel("Photo:"));
		sample_path = new QLineEdit;
		sample_path->setPlaceholderText("path/to/sample.jpg");
		path_row->addWidget(sample_path, 1);
		QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
		connect(browse, &QPushButton::clicked, this, &CalDistortionFrame::browseSamplePhoto);
		path_row->addWidget(browse);
		pl->addLayout(path_row);

		sample_preview = new QLabel;
		sample_preview->setFixedHeight(200);
		sample_preview->setAlignment(Qt::AlignCenter);
		sample_preview->setStyleSheet(
			"QLabel { background: palette(base); border: 1px solid palette(mid); "
			"border-radius: 4px; }");
		sample_preview->setText("<i>No photo selected.</i>");
		pl->addWidget(sample_preview);

		exif_result = new QLabel;
		exif_result->setWordWrap(true);
		exif_result->setTextFormat(Qt::RichText);
		exif_result->setStyleSheet(
			"QLabel { background: palette(base); border: 1px solid palette(mid); "
			"border-radius: 4px; padding: 6px; }");
		exif_result->setText("<i>No photo selected yet.</i>");
		pl->addWidget(exif_result);
		pl->addStretch();

		param_stack->addWidget(page);
	}

	// page 3: calibration grid photo
	{
		QFrame *page = new QFrame;
		QVBoxLayout *pl = new QVBoxLayout(page);
		pl->setContentsMargins(0, 4, 0, 0);
		pl->setSpacing(8);

		QLabel *hint = new QLabel(
			"Provide a photo (or folder of photos) of a checkerboard / dot-grid target. "
			"Corner detection and Zhang\u2019s method will be used to compute the "
			"Brown-Conrady distortion coefficients.");
		hint->setWordWrap(true);
		pl->addWidget(hint);

		QHBoxLayout *path_row = new QHBoxLayout;
		path_row->addWidget(new QLabel("Image / folder:"));
		grid_path = new QLineEdit;
		grid_path->setPlaceholderText("path/to/grid.jpg  or  path/to/grid-images/  (JPEG, TIFF, RAW…)");
		path_row->addWidget(grid_path, 1);
		QPushButton *browse = new QPushButton(QIcon::fromTheme("folder"), "");
		connect(browse, &QPushButton::clicked, this, &CalDistortionFrame::browseGridPhoto);
		path_row->addWidget(browse);
		pl->addLayout(path_row);

		// Inner corners count
		QHBoxLayout *board_row = new QHBoxLayout;
		board_row->addWidget(new QLabel("Inner corners:"));
		board_row->addSpacing(4);
		board_row->addWidget(new QLabel("Cols:"));
		grid_cols = new QSpinBox;
		grid_cols->setRange(2, 30);
		grid_cols->setValue(8);
		grid_cols->setToolTip("Number of inner corners horizontally");
		board_row->addWidget(grid_cols);
		board_row->addSpacing(8);
		board_row->addWidget(new QLabel("Rows:"));
		grid_rows = new QSpinBox;
		grid_rows->setRange(2, 30);
		grid_rows->setValue(5);
		grid_rows->setToolTip("Number of inner corners vertically");
		board_row->addWidget(grid_rows);
		board_row->addStretch();
		pl->addLayout(board_row);

		// Image preview
		grid_preview = new QLabel;
		grid_preview->setFixedHeight(240);
		grid_preview->setAlignment(Qt::AlignCenter);
		grid_preview->setStyleSheet(
			"QLabel { background: palette(base); border: 1px solid palette(mid); "
			"border-radius: 4px; }");
		grid_preview->setText("<i>No image selected.</i>");
		pl->addWidget(grid_preview);

		QPushButton *run_btn = new QPushButton("Run calibration\u2026");
		connect(run_btn, &QPushButton::clicked, this, &CalDistortionFrame::runGridCalibration);
		pl->addWidget(run_btn);

		grid_result = new QLabel;
		grid_result->setWordWrap(true);
		grid_result->setTextFormat(Qt::RichText);
		grid_result->setStyleSheet(
			"QLabel { background: palette(base); border: 1px solid palette(mid); "
			"border-radius: 4px; padding: 6px; }");
		grid_result->setText("<i>No calibration run yet.</i>");
		pl->addWidget(grid_result);
		pl->addStretch();

		param_stack->addWidget(page);
	}

	connect(bg, QOverload<int>::of(&QButtonGroup::idClicked),
	        param_stack, &QStackedWidget::setCurrentIndex);

	layout->addWidget(param_stack);
	layout->addStretch(1);
}

void CalDistortionFrame::browseLensFile() {
	QString path = QFileDialog::getOpenFileName(
		this, "Select lens file", QString(), "JSON files (*.json);;All files (*)");
	if (!path.isEmpty()) lens_file_path->setText(path);
}

void CalDistortionFrame::loadLensFile() {
	QString path = QFileDialog::getOpenFileName(
		this, "Load distortion parameters", QString(), "JSON files (*.json);;All files (*)");
	// TODO: parse lens.json and apply to session
	(void)path;
}

void CalDistortionFrame::saveLensFile() {
	QString path = QFileDialog::getSaveFileName(
		this, "Save distortion parameters", "lens.json", "JSON files (*.json);;All files (*)");
	// TODO: write current coefficients to file
	(void)path;
}

void CalDistortionFrame::browseSamplePhoto() {
	QString path = QFileDialog::getOpenFileName(
		this, "Select sample photo", QString(),
		"Images (*.jpg *.jpeg *.tiff *.tif *.png "
		"*.cr2 *.cr3 *.nef *.nrw *.arw *.srf *.sr2 *.orf *.rw2 *.raf "
		"*.dng *.pef *.ptx *.kdc *.dcr *.raw *.rwl *.mrw *.3fr *.fff "
		"*.iiq *.x3f *.erf);;All files (*)");
	if (!path.isEmpty()) {
		sample_path->setText(path);
		updatePreview(path, sample_preview);
		detectFromExif();
	}
}

void CalDistortionFrame::detectFromExif() {
	QString path = sample_path->text().trimmed();
	if (path.isEmpty()) {
		exif_result->setText("<i>No photo selected.</i>");
		return;
	}

	Exif exif;
	exif.parse(path);

	if (exif.isEmpty()) {
		exif_result->setText("<i>Could not read EXIF data from this file.</i>");
		return;
	}

	// Camera make / model
	QString make  = exif.value(Exif::Make,  QString()).toString().trimmed();
	QString model = exif.value(Exif::Model, QString()).toString().trimmed();

	// Remove the make prefix that many cameras duplicate inside Model
	if (!make.isEmpty() && model.startsWith(make))
		model = model.mid(make.size()).trimmed();

	// Focal length: prefer the 35 mm equivalent when available
	double focal35  = exif.value(Exif::FocalLengthIn35mmFilm, 0.0).toDouble();
	double focalRaw = exif.value(Exif::FocalLength, 0.0).toDouble();
	float  focal    = (focalRaw > 0) ? (float)focalRaw : (float)focal35;

	// ---- Lensfun lookup -------------------------------------------------
	lfDatabase *ldb = new lfDatabase;
	if (ldb->Load() != LF_NO_ERROR) {
		delete ldb;
		exif_result->setText("<i>Could not load the Lensfun database.</i>");
		return;
	}

	// Search for camera (make + model)
	const lfCamera *cam = nullptr;
	{
		const lfCamera **cams = ldb->FindCamerasExt(
			make.toUtf8().constData(), model.toUtf8().constData());
		if (cams) {
			cam = cams[0];
			lf_free(cams);
		}
	}

	// Search for lens — try the camera model string as a lens hint first,
	// then fall back to a wildcard search constrained by the camera
	const lfLens *lens = nullptr;
	{
		const lfLens **lenses = ldb->FindLenses(cam, nullptr, nullptr);
		if (lenses) {
			lens = lenses[0];
			lf_free(lenses);
		}
	}

	// Build result HTML
	QString html = "<table cellspacing=\"2\">"
		"<tr><td><b>Make:</b></td><td>"
		    + (make.isEmpty() ? "<i>not found</i>" : make.toHtmlEscaped()) + "</td></tr>"
		"<tr><td><b>Model:</b></td><td>"
		    + (model.isEmpty() ? "<i>not found</i>" : model.toHtmlEscaped()) + "</td></tr>"
		"<tr><td><b>Focal&nbsp;length:</b></td><td>";

	if (focal35 > 0)
		html += QString("%1 mm (35 mm eq.)").arg(focal35, 0, 'f', 1).toHtmlEscaped();
	else if (focalRaw > 0)
		html += QString("%1 mm").arg(focalRaw, 0, 'f', 1).toHtmlEscaped();
	else
		html += "<i>unknown</i>";
	html += "</td></tr></table><br>";

	if (!cam) {
		html += "<span style='color:red'>Camera not found in Lensfun database.</span>";
	} else {
		html += "<b>Lensfun camera:</b> "
		      + QString(cam->Maker).toHtmlEscaped() + " "
		      + QString(cam->Model).toHtmlEscaped() + "<br>";
	}

	if (!lens) {
		html += "<br><span style='color:red'>No matching lens found in Lensfun database.</span>";
	} else {
		html += "<b>Lensfun lens:</b> " + QString(lens->Model).toHtmlEscaped() + "<br>";

		// Interpolate distortion at the photo's focal length
		lfLensCalibDistortion dist{};
		if (focal > 0 && lens->InterpolateDistortion(focal, dist)) {
			html += "<b>Distortion model:</b> ";
			switch (dist.Model) {
			case LF_DIST_MODEL_POLY3:
				html += QString("Poly3 &mdash; k1 = %1")
					.arg((double)dist.Terms[0], 0, 'g', 5);
				break;
			case LF_DIST_MODEL_POLY5:
				html += QString("Poly5 &mdash; k1 = %1, k2 = %2")
					.arg((double)dist.Terms[0], 0, 'g', 5)
					.arg((double)dist.Terms[1], 0, 'g', 5);
				break;
			case LF_DIST_MODEL_PTLENS:
				html += QString("PTLens &mdash; a = %1, b = %2, c = %3")
					.arg((double)dist.Terms[0], 0, 'g', 5)
					.arg((double)dist.Terms[1], 0, 'g', 5)
					.arg((double)dist.Terms[2], 0, 'g', 5);
				break;
			default:
				html += "<i>unknown model</i>";
				break;
			}
		} else {
			html += "<i>No distortion data for this focal length.</i>";
		}
	}

	delete ldb;
	exif_result->setText(html);
}

void CalDistortionFrame::browseGridPhoto() {
	// Allow either a single file or a directory
	QString path = QFileDialog::getExistingDirectory(
		this, "Select folder of grid images", QString());
	if (path.isEmpty())
		path = QFileDialog::getOpenFileName(
			this, "Select grid image", QString(),
			"Images (*.jpg *.jpeg *.tiff *.tif *.png "
			"*.cr2 *.cr3 *.nef *.nrw *.arw *.srf *.sr2 *.orf *.rw2 *.raf "
			"*.dng *.pef *.ptx *.kdc *.dcr *.raw *.rwl *.mrw *.3fr *.fff "
			"*.iiq *.x3f *.erf);;All files (*)");
	if (!path.isEmpty()) {
		grid_path->setText(path);
		// Show preview for single file; for a directory pick the first image
		QString previewFile = path;
		if (QFileInfo(path).isDir()) {
			static const QStringList exts = {
				"*.jpg","*.jpeg","*.tiff","*.tif","*.png",
				"*.cr2","*.cr3","*.nef","*.nrw","*.arw","*.srf","*.sr2",
				"*.orf","*.rw2","*.raf","*.dng","*.pef","*.ptx","*.kdc",
				"*.dcr","*.raw","*.rwl","*.mrw","*.3fr","*.fff","*.iiq",
				"*.x3f","*.erf"
			};
			auto entries = QDir(path).entryInfoList(exts, QDir::Files);
			if (!entries.isEmpty())
				previewFile = entries.first().absoluteFilePath();
		}
		updatePreview(previewFile, grid_preview);
	}
}

void CalDistortionFrame::runGridCalibration() {
	QString path = grid_path->text().trimmed();
	if (path.isEmpty()) {
		grid_result->setText("<i>No image or folder selected.</i>");
		return;
	}

	int cols = grid_cols->value();
	int rows = grid_rows->value();
	cv::Size board(cols, rows);

	// Collect image files
	static const QStringList imgFilters = {
		"*.jpg","*.jpeg","*.tiff","*.tif","*.png",
		"*.cr2","*.cr3","*.nef","*.nrw","*.arw","*.srf","*.sr2",
		"*.orf","*.rw2","*.raf","*.dng","*.pef","*.ptx","*.kdc",
		"*.dcr","*.raw","*.rwl","*.mrw","*.3fr","*.fff","*.iiq",
		"*.x3f","*.erf"
	};

	QStringList files;
	if (QFileInfo(path).isDir()) {
		for (const auto &fi : QDir(path).entryInfoList(imgFilters, QDir::Files))
			files << fi.absoluteFilePath();
	} else {
		files << path;
	}

	if (files.isEmpty()) {
		grid_result->setText("<i>No image files found at the selected path.</i>");
		return;
	}

	// 3D object points for one view (Z=0 plane, square side = 1 unit)
	std::vector<cv::Point3f> objPt;
	objPt.reserve(cols * rows);
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols; ++c)
			objPt.push_back({(float)c, (float)r, 0.f});

	std::vector<std::vector<cv::Point3f>> objPoints;
	std::vector<std::vector<cv::Point2f>> imgPoints;
	cv::Size imgSize;

	cv::Mat lastGray;
	std::vector<cv::Point2f> lastCorners;
	int skipped = 0;

	for (const QString &fp : files) {
		cv::Mat gray = loadAsGrayCvMat(fp);
		if (gray.empty()) { ++skipped; continue; }
		if (imgSize.empty()) imgSize = gray.size();

		std::vector<cv::Point2f> corners;
		bool found = cv::findChessboardCorners(
			gray, board, corners,
			cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
		if (!found) { ++skipped; continue; }

		cv::cornerSubPix(gray, corners, {11, 11}, {-1, -1},
			cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.01));

		objPoints.push_back(objPt);
		imgPoints.push_back(corners);
		lastGray = gray;
		lastCorners = corners;
	}

	int found_count = (int)objPoints.size();
	if (found_count == 0) {
		grid_result->setText(
			QString("<span style='color:red'>Checkerboard not detected in any of %1 image(s). "
			        "Check that the inner corners count (%2 \u00d7 %3) matches the pattern.</span>")
			.arg(files.size()).arg(cols).arg(rows));
		return;
	}

	// Run calibration
	cv::Mat K = cv::Mat::eye(3, 3, CV_64F);
	cv::Mat dist;
	std::vector<cv::Mat> rvecs, tvecs;
	double rms = cv::calibrateCamera(objPoints, imgPoints, imgSize, K, dist, rvecs, tvecs);

	// dist layout: [k1, k2, p1, p2, k3, k4, k5, k6, s1, s2, s3, s4, tx, ty]
	auto D = [&](int i) -> double {
		return (dist.cols > i) ? dist.at<double>(i) : 0.0;
	};
	double k1 = D(0), k2 = D(1), p1 = D(2), p2 = D(3), k3 = D(4);

	// Update preview: draw detected corners on the last good image
	if (!lastGray.empty()) {
		cv::Mat vis;
		cv::cvtColor(lastGray, vis, cv::COLOR_GRAY2BGR);
		cv::drawChessboardCorners(vis, board, lastCorners, true);
		// Convert cv::Mat BGR to QImage RGB
		QImage qi(vis.data, vis.cols, vis.rows, (int)vis.step, QImage::Format_BGR888);
		QPixmap pm = QPixmap::fromImage(qi)
			.scaled(grid_preview->width(), grid_preview->height(),
			        Qt::KeepAspectRatio, Qt::SmoothTransformation);
		grid_preview->setPixmap(pm);
	}

	QString html = QString(
		"<b>Calibration result</b> &mdash; "
		"RMS reprojection error: <b>%1 px</b> &nbsp;|&nbsp; "
		"%2 image(s) used"
		"%3"
		"<table cellspacing=\"4\" style='margin-top:6px'>"
		"<tr><td><b>k1</b></td><td>%4</td></tr>"
		"<tr><td><b>k2</b></td><td>%5</td></tr>"
		"<tr><td><b>k3</b></td><td>%6</td></tr>"
		"<tr><td><b>p1</b></td><td>%7</td></tr>"
		"<tr><td><b>p2</b></td><td>%8</td></tr>"
		"</table>")
		.arg(rms, 0, 'f', 3)
		.arg(found_count)
		.arg(skipped > 0
			? QString(" <span style='color:orange'>(%1 skipped)</span>").arg(skipped)
			: QString())
		.arg(k1, 0, 'g', 6).arg(k2, 0, 'g', 6).arg(k3, 0, 'g', 6)
		.arg(p1, 0, 'g', 6).arg(p2, 0, 'g', 6);

	grid_result->setText(html);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool CalDistortionFrame::isRawPath(const QString &path) {
	static const QSet<QString> rawExts = {
		"cr2","cr3","nef","nrw","arw","srf","sr2","orf","rw2","raf",
		"dng","pef","ptx","kdc","dcr","raw","rwl","mrw","3fr","fff",
		"iiq","x3f","erf"
	};
	return rawExts.contains(QFileInfo(path).suffix().toLower());
}

QPixmap CalDistortionFrame::loadPreviewPixmap(const QString &path, int maxDim) {
	QImage img;
	if (isRawPath(path)) {
		LibRaw raw;
		// Fast path: try embedded JPEG thumbnail
		if (raw.open_file(path.toLocal8Bit().constData()) == LIBRAW_SUCCESS
		    && raw.unpack_thumb() == LIBRAW_SUCCESS)
		{
			libraw_processed_image_t *thumb = raw.dcraw_make_mem_thumb();
			if (thumb) {
				if (thumb->type == LIBRAW_IMAGE_JPEG) {
					img.loadFromData(thumb->data, thumb->data_size, "JPEG");
				} else if (thumb->type == LIBRAW_IMAGE_BITMAP && thumb->colors == 3) {
					img = QImage(thumb->data, thumb->width, thumb->height,
					             thumb->width * 3, QImage::Format_RGB888).copy();
				}
				LibRaw::dcraw_clear_mem(thumb);
			}
		}
		// Fall back: decode a half-size version of the raw data
		if (img.isNull()) {
			raw.imgdata.params.use_camera_wb = 1;
			raw.imgdata.params.output_bps    = 8;
			raw.imgdata.params.half_size     = 1;
			if (raw.unpack() == LIBRAW_SUCCESS && raw.dcraw_process() == LIBRAW_SUCCESS) {
				libraw_processed_image_t *full = raw.dcraw_make_mem_image();
				if (full && full->colors == 3 && full->bits == 8) {
					img = QImage(full->data, full->width, full->height,
					             full->width * 3, QImage::Format_RGB888).copy();
				}
				if (full) LibRaw::dcraw_clear_mem(full);
			}
		}
	} else {
		img = QImage(path);
	}

	if (img.isNull()) return {};
	return QPixmap::fromImage(
		img.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CalDistortionFrame::updatePreview(const QString &path, QLabel *target) {
	if (path.isEmpty() || !target) return;
	QPixmap pm = loadPreviewPixmap(path, 800);
	if (pm.isNull()) {
		target->setText("<i>Could not load image.</i>");
	} else {
		target->setPixmap(pm.scaled(
			target->width(), target->height(),
			Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}

// Load an image as an 8-bit greyscale cv::Mat (handles RAW via LibRaw)
static cv::Mat loadAsGrayCvMat(const QString &path) {
	static const QSet<QString> rawExts = {
		"cr2","cr3","nef","nrw","arw","srf","sr2","orf","rw2","raf",
		"dng","pef","ptx","kdc","dcr","raw","rwl","mrw","3fr","fff",
		"iiq","x3f","erf"
	};

	if (rawExts.contains(QFileInfo(path).suffix().toLower())) {
		LibRaw raw;
		raw.imgdata.params.use_camera_wb = 1;
		raw.imgdata.params.output_bps = 8;
		if (raw.open_file(path.toLocal8Bit().constData()) != LIBRAW_SUCCESS) return {};
		if (raw.unpack()         != LIBRAW_SUCCESS) return {};
		if (raw.dcraw_process()  != LIBRAW_SUCCESS) return {};
		libraw_processed_image_t *img = raw.dcraw_make_mem_image();
		if (!img) return {};
		// img->data is packed RGB, 8-bit
		cv::Mat rgb(img->height, img->width, CV_8UC3,
		            const_cast<unsigned char*>(img->data));
		cv::Mat gray;
		cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
		cv::Mat result = gray.clone();
		LibRaw::dcraw_clear_mem(img);
		return result;
	} else {
		return cv::imread(path.toStdString(), cv::IMREAD_GRAYSCALE);
	}
}



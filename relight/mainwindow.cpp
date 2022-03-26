#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "graphics_view_zoom.h"
#include "rtiexport.h"
#include "zoomdialog.h"
#include "../src/imageset.h"
#include "helpdialog.h"
#include "focaldialog.h"
#include "scripts.h"
#include "queuewindow.h"
#include "httpserver.h"
#include "settingsdialog.h"
#include "domecalibration.h"
#include "convertdialog.h"
#include "aligndialog.h"

#include "qmeasuremarker.h"
#include "qspheremarker.h"
#include "qalignmarker.h"
#include "qwhitemarker.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QSettings>
#include <QListView>
#include <QStandardItemModel>
#include <QItemSelectionModel>

#include <QtConcurrent/QtConcurrent>

#include <set>
#include <iostream>
#include <assert.h>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	settings = new QSettings;

	ui->setupUi(this);

    // File menu
	this->setWindowTitle("Relight");
	connect(ui->actionNew,        SIGNAL(triggered(bool)),  this, SLOT(newProject()));
	connect(ui->actionOpen,       SIGNAL(triggered(bool)),  this, SLOT(openProject()));

	connect(ui->actionSave,       SIGNAL(triggered(bool)),  this, SLOT(saveProject()));
	connect(ui->actionSave_as,    SIGNAL(triggered(bool)),  this, SLOT(saveProjectAs()));
	connect(ui->actionPreferences,    SIGNAL(triggered(bool)),  this, SLOT(preferences()));
	connect(ui->actionExit,       SIGNAL(triggered(bool)),  this, SLOT(quit()));

    connect(ui->actionSave_LP,    SIGNAL(triggered(bool)), this, SLOT(saveLPs()));
    connect(ui->actionLoad_LP,    SIGNAL(triggered(bool)), this, SLOT(loadLP()));

    // View menu
	connect(ui->actionPrevious,   SIGNAL(triggered(bool)),  this, SLOT(previous()));
	connect(ui->actionNext,       SIGNAL(triggered(bool)),  this, SLOT(next()));
	connect(ui->actionToggle_max_luma, SIGNAL(triggered(bool)), this, SLOT(toggleMaxLuma()));
    connect(ui->actionView_RTI,     SIGNAL(triggered(bool)),  this, SLOT(viewRTI()));

    // Export menu
	connect(ui->actionExport_RTI, SIGNAL(triggered(bool)),  this, SLOT(exportRTI()));
	connect(ui->actionExport_Normals, SIGNAL(triggered(bool)),  this, SLOT(exportNormals()));
	connect(ui->actionConvert_rti, SIGNAL(triggered(bool)),  this, SLOT(convertRTI()));
	connect(ui->actionShow_queue,   SIGNAL(triggered(bool)),  this, SLOT(showQueue()));
    connect(ui->actionDeepzoom, SIGNAL(triggered(bool)), this, SLOT(deepZoom()));
    connect(ui->actionTarzoom, SIGNAL(triggered(bool)), this, SLOT(tarZoom()));
    connect(ui->actionItarzoom, SIGNAL(triggered(bool)), this, SLOT(itarZoom()));

    // Edit menu
	connect(ui->actionDetectHighlights, SIGNAL(triggered(bool)),   this, SLOT(detectHighlights()));
    connect(ui->newSphere, SIGNAL(clicked()), this, SLOT(newSphere()));
    connect(ui->newWhite, SIGNAL(clicked()), this, SLOT(newWhite()));
    connect(ui->newAlign, SIGNAL(clicked()), this, SLOT(newAlign()));
    connect(ui->newMeasure, SIGNAL(clicked()), this, SLOT(newMeasure()));

    connect(ui->actionNewSphere, SIGNAL(triggered()), this, SLOT(newSphere()));
    connect(ui->actionNewWhite, SIGNAL(triggered()), this, SLOT(newWhite()));
    connect(ui->actionNewAlign, SIGNAL(triggered()), this, SLOT(newAlign()));
    connect(ui->actionNewMeasure, SIGNAL(triggered()), this, SLOT(newMeasure()));

    connect(ui->actionDelete_selected, SIGNAL(triggered()), this, SLOT(deleteSelected()));
    ui->actionDelete_selected->setShortcuts(QList<QKeySequence>() << Qt::Key_Delete << Qt::Key_Backspace);

    // Calibration menu
	connect(ui->actionLens_parameters, SIGNAL(triggered(bool)), this, SLOT(editLensParameters()));
	connect(ui->actionDome_geometry, SIGNAL(triggered(bool)), this, SLOT(domeCalibration()));
	connect(ui->actionWhite_balance, SIGNAL(triggered(bool)), this, SLOT(whiteBalance()));

    // Help menu
	connect(ui->actionHelp,       SIGNAL(triggered(bool)), this, SLOT(showHelp()));

	scene = new RTIScene(this);
	connect(scene, SIGNAL(borderPointMoved(QGraphicsEllipseItem *)), this, SLOT(updateBorderPoints(QGraphicsEllipseItem *)));
	connect(scene, SIGNAL(highlightMoved(QGraphicsEllipseItem *)), this, SLOT(updateHighlight(QGraphicsEllipseItem *)));


	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);
	QApplication::setOverrideCursor( Qt::ArrowCursor );

	auto *gvz = new Graphics_view_zoom(ui->graphicsView);
	connect(gvz, SIGNAL(dblClicked(QPoint)), this, SLOT(doubleClick(QPoint)));
	connect(gvz, SIGNAL(clicked(QPoint)), this, SLOT(pointClick(QPoint)));
	connect(ui->actionZoom_in,  SIGNAL(triggered(bool)), gvz, SLOT(zoomIn()));
	connect(ui->actionZoom_out, SIGNAL(triggered(bool)), gvz, SLOT(zoomOut()));


	rtiexport = new RtiExport(this);
    zoom = new ZoomDialog(this);
	help = new HelpDialog(this);
	imageModel = new QStandardItemModel(ui->imageList1);
	ui->imageList1->setModel(imageModel);


	// Register model item  changed signal
	QItemSelectionModel *selectionModel = ui->imageList1->selectionModel();

	connect(selectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
			this, SLOT(openImage(const QModelIndex &)));
	connect( imageModel , SIGNAL(itemChanged(QStandardItem *)), this, SLOT( imageChecked(QStandardItem * )));

}

MainWindow::~MainWindow() {	delete ui; }

void MainWindow::preferences() {
	if(!settings_dialog) {
		settings_dialog = new SettingsDialog();
		settings_dialog->setModal(true);
	}
	settings_dialog->show();
}

void MainWindow::clear() {
	if(imagePixmap) {
		delete imagePixmap;
		imagePixmap = nullptr;
	}
	project_filename = QString();
	if(imagePixmap)
		delete imagePixmap;
	ui->graphicsView->resetMatrix();

	ui->markerList->clear();
	project.clear();
}



void MainWindow::newProject() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder", lastDir);
	if(dir.isNull()) return;

	clear();

	project.setDir(QDir(dir));
	bool ok = project.scanDir();
	if(!project.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + project.dir.path());
		return;
	}
	if(!ok) {
		//check if we can rotate a few images.
		bool canrotate = false;
		for(Image &image: project.images) {
			if(image.size == project.imgsize)
				continue;

			if(image.isRotated(project.imgsize))
				canrotate = true;
		}
		if(canrotate) {
			int answer = QMessageBox::question(this, "Some images are rotated.", "Do you wish to uniform image rotation?", QMessageBox::Yes, QMessageBox::No);
			if(answer != QMessageBox::No)
				project.rotateImages();
		} else
			QMessageBox::critical(this, "Resolution problem", "Not all of the images in the folder have the same resolution (marked in red)");
	}

	QStringList img_ext;
	img_ext << "*.lp";
	QStringList lps = QDir(dir).entryList(img_ext);
	if(lps.size() > 1) {
	} else if(lps.size() == 1) {
		int answer = QMessageBox::question(this, "Found an .lp file", "Do you wish to load the .lp file?", QMessageBox::Yes, QMessageBox::No);
		if(answer != QMessageBox::No)
			loadLP(lps[0]);
	}

	enableActions();
	init();
}

void MainWindow::openProject() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();

	QString filename = QFileDialog::getOpenFileName(this, "Select a project", lastDir, "*.relight");
	if(filename.isNull())
		return;

	clear();

	project.clear();
	try {
		project.load(filename);
	} catch(QString e) {
		QMessageBox::critical(this, "Could not load the project: " + filename, "Error: " + e);
		return;
	}
	project_filename = project.dir.relativeFilePath(filename);
	init();
	enableActions();
}

void MainWindow::saveProject() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();

	if(project_filename.isNull()) {
		QString filename = QFileDialog::getSaveFileName(this, "Save file: ", lastDir, "*.relight");
		if(!filename.endsWith((".relight")))
			filename += ".relight";
		project_filename = filename;
	}
	if(!project_filename.isNull())
		project.save(project_filename);
	//TODO set window title as project filename filename
}

void MainWindow::saveProjectAs() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();

	QString new_filename = QFileDialog::getSaveFileName(this, "Save file: ", lastDir, "*.relight");
	if(new_filename.isNull())
		return;
	project_filename = new_filename;
	saveProject();
}

void MainWindow::enableActions() {
	ui->actionSave->setEnabled(true);
	ui->actionSave_as->setEnabled(true);
	ui->actionPrevious->setEnabled(true);
	ui->actionNext->setEnabled(true);
	ui->actionExport_RTI->setEnabled(true);
	ui->actionExport_Normals->setEnabled(true);
	ui->actionLoad_LP->setEnabled(true);
	ui->actionSave_LP->setEnabled(true);
	ui->actionZoom_in->setEnabled(true);
	ui->actionZoom_out->setEnabled(true);

	if(project.hasDirections())
		ui->actionSave_LP->setEnabled(true);

	ui->actionDetectHighlights->setEnabled(true);
	ui->actionDelete_selected->setEnabled(true);

	ui->newSphere->setEnabled(true);
	ui->newMeasure->setEnabled(true);
	ui->newAlign->setEnabled(true);
	ui->newWhite->setEnabled(true);

	ui->actionNewSphere->setEnabled(true);
	ui->actionNewMeasure->setEnabled(true);
	ui->actionNewAlign->setEnabled(true);
	ui->actionNewWhite->setEnabled(true);
}

bool MainWindow::init() {

	if(imagePixmap)
		delete imagePixmap;
	settings->setValue("LastDir", project.dir.path());

	setupSpheres();
	setupMeasures();
	setupAligns();

	//create the items (name and TODO thumbnail
	int count = 0;
	imageModel->clear();
	for(Image &a: project.images) {

		QStandardItem *item = new QStandardItem;
		item->setText(QString("%1 - %2").arg(count+1).arg(a.filename));
		item->setCheckable(true);
		// Uncheck the item
		item->setCheckState(a.valid ? Qt::Checked : Qt::Unchecked);
		item->setData(a.valid ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
		item->setData(count, Qt::UserRole+1);
		item->setBackground(a.hasLightDirection() ? Qt::darkGreen : QBrush());
		imageModel->setItem(count, item);

		count++;
	}

	openImage(0);
	return true;
}

void MainWindow::imageChecked(QStandardItem *item) {
	QModelIndex index = imageModel->indexFromItem(item);
	Image &image = project.images[index.row()];
	bool skip = !index.data(Qt::CheckStateRole).toBool();
	if(!skip && !image.valid) {
		QMessageBox::critical(this, "Can't include this image.", "This image has a different resolution or focal, cannot include in the processing");
		item->setCheckState(Qt::Unchecked);
		return;
	}
	image.skip = skip;
//item->setBackground(image.skip? Qt::red : Qt::white);
}

void MainWindow::openImage(const QModelIndex &index) {
	// Set selection
	openImage(index.row(), false);
}

void MainWindow::openImage(int id, bool fit) {
	QString filename = project.images[id].filename;//item->text();

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	currentImage = id;

	maxLuming = false;

	size_t n = size_t(currentImage);
	if(imagePixmap)
		delete imagePixmap;
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene->addItem(imagePixmap);

	if(fit) {
		//find smallest problems
		double sx =  double(ui->graphicsView->width()) / project.imgsize.width();
		double sy = double(ui->graphicsView->height()) / project.imgsize.height();
		double s = std::min(sx, sy);
		ui->graphicsView->scale(s, s);
	}
	showHighlights(n);
}

void MainWindow::showHighlights(size_t n) {
	ignore_scene_changes = true;
	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<SphereMarker *>(marker);
		if(!m)
			continue;
		m->showHighlight(n);
	}
	ignore_scene_changes = false;
}

	
bool MainWindow::lumaCallback(std::string s, int n) {
	QString str(s.c_str());
	emit lumaProgressText(str);
	emit lumaProgress(n);
	if(lumaCancelling)
		return false;
	return true;
}

void MainWindow::lumaCancel() {
	lumaCancelling = true;
}

void MainWindow::lumaFinish() {
	if(progress == nullptr)
		return;
	progress->close();
	delete progress;
	progress = nullptr;
	toggleMaxLuma();
}

void MainWindow::computeMaxLuma() {
	
	lumaCancelling = false;
	QFuture<void> future = QtConcurrent::run([this]() {
		ImageSet imageset;
		for(auto image: project.images)
			imageset.images.push_back(image.filename);
		imageset.initImages(this->project.dir.path().toStdString().c_str());
		std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->lumaCallback(s, n); };
		this->maxLuma = imageset.maxImage(&callback); 
	} );
	watcher.setFuture(future);
	connect(&watcher, SIGNAL(finished()), this, SLOT(lumaFinish()));

	progress = new QProgressDialog("Building max luma image", "Cancel", 0, 100, this);
	progress->setAutoClose(false);
	progress->setAutoReset(false);
	progress->setWindowModality(Qt::WindowModal);
	connect(progress, SIGNAL(canceled()), this, SLOT(lumaCancel()));
	connect(this, SIGNAL(lumaProgress(int)), progress, SLOT(setValue(int)));
	connect(this, SIGNAL(lumaProgressText(const QString &)), progress, SLOT(setLabelText(const QString &)));
	progress->show();
}

void MainWindow::toggleMaxLuma() {
	
	if(maxLuma.isNull()) {
		computeMaxLuma();
		return;
	}
	if(maxLuming)
		openImage(currentImage); //ui->imageList->item(currentImage));
	else {
		if(imagePixmap)
			delete imagePixmap;
		imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(maxLuma));
		imagePixmap->setZValue(-1);
		scene->addItem(imagePixmap);
	}
	maxLuming = !maxLuming;
}

void MainWindow::previous() {
	if(currentImage == 0)
		return;
	openImage(currentImage -1); //ui->imageList->item(currentImage-1));

}


void MainWindow::next() {
	if(currentImage == int(project.size()-1))
		return;
	openImage(currentImage + 1); //ui->imageList->item(currentImage+1));
}


void MainWindow::pointClick(QPoint p) {
	QPointF pos = ui->graphicsView->mapToScene(p);

	for(auto marker: ui->markerList->getItems()) {
		if(marker->editing)
			return marker->click(pos);
	}
}

void MainWindow::doubleClick(QPoint p) {

	QPointF pos = ui->graphicsView->mapToScene(p);

	for(auto marker: ui->markerList->getItems()) {
		if(marker->editing)
			return marker->doubleClick(pos);
	}
}

void MainWindow::updateBorderPoints(QGraphicsEllipseItem *point) {
	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<SphereMarker *>(marker);
		if(!m)
			continue;
		m->updateBorderPoint(point);
		m->fit();
	}
}

void MainWindow::updateHighlight(QGraphicsEllipseItem *highlight) {
	if(ignore_scene_changes)
		return;

	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<SphereMarker *>(marker);
		if(!m)
			continue;

		if(!m->sphere->fitted)
			continue;

		if(m->highlight != highlight)
			continue;

		m->updateHighlightPosition(currentImage);

		QStandardItem *item = imageModel->item(currentImage);
		item->setBackground(Qt::darkGreen);
	}
}


void MainWindow::newSphere() {
	auto sphere = project.newSphere();
	auto marker = new SphereMarker(sphere, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
	connect(marker, SIGNAL(removed()), this, SLOT(removeSphere()));
	marker->setEditing(true);
}



void MainWindow::newMeasure() {
	auto measure = project.newMeasure();
	auto marker = new MeasureMarker(measure, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
	connect(marker, SIGNAL(removed()), this, SLOT(removeMeasure()));
	marker->startMeasure();
}

void MainWindow::newAlign() {
	auto align = project.newAlign();
	auto marker = new AlignMarker(align, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
	connect(marker, SIGNAL(removed()), this, SLOT(removeAlign()));
	connect(marker, SIGNAL(showTable(AlignMarker *)), this, SLOT(showAlignDialog(AlignMarker *)));
	marker->setEditing(true);
}

void MainWindow::showAlignDialog(AlignMarker *marker) {
	AlignDialog *dialog = new AlignDialog(marker, &project, this);
	dialog->show();
}

void MainWindow::newWhite() {
	auto white = project.newWhite();
	auto marker = new WhiteMarker(white, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
	connect(marker, SIGNAL(removed()), this, SLOT(removeWhite()));
	marker->setEditing(true);
}



void MainWindow::removeSphere() {
	auto marker = dynamic_cast<SphereMarker *>(QObject::sender());
	project.spheres.erase(std::remove(project.spheres.begin(), project.spheres.end(), marker->sphere), project.spheres.end());
	delete marker;
}

void MainWindow::removeMeasure() {
	auto marker = dynamic_cast<MeasureMarker *>(QObject::sender());
	project.measures.erase(std::remove(project.measures.begin(), project.measures.end(), marker->measure), project.measures.end());
	delete marker;
}

void MainWindow::removeAlign() {
	auto marker = dynamic_cast<AlignMarker *>(QObject::sender());
	project.aligns.erase(std::remove(project.aligns.begin(), project.aligns.end(), marker->align), project.aligns.end());
	delete marker;
}

void MainWindow::removeWhite() {
	auto marker = dynamic_cast<WhiteMarker *>(QObject::sender());
	project.whites.erase(std::remove(project.whites.begin(), project.whites.end(), marker->white), project.whites.end());
	delete marker;
}


void MainWindow::setupSpheres() {
	for(auto sphere: project.spheres) {
		auto marker = new SphereMarker(sphere, ui->graphicsView, ui->markerList);
		connect(marker, SIGNAL(removed()), this, SLOT(removeSphere()));
		ui->markerList->addItem(marker);
	}
}

void MainWindow::setupMeasures() {
	for(auto m: project.measures) {
		auto marker = new MeasureMarker(m, ui->graphicsView, ui->markerList);
		connect(marker, SIGNAL(removed()), this, SLOT(removeMeasure()));
		ui->markerList->addItem(marker);
	}
}

void MainWindow::setupAligns() {
	for(auto align: project.aligns) {
		auto marker = new AlignMarker(align, ui->graphicsView, ui->markerList);
		connect(marker, SIGNAL(removed()), this, SLOT(removeAlign()));
		ui->markerList->addItem(marker);
	}
}

void MainWindow::setupWhites() {
	for(auto white: project.whites) {
		auto marker = new WhiteMarker(white, ui->graphicsView, ui->markerList);
		connect(marker, SIGNAL(removed()), this, SLOT(removeWhite()));
		ui->markerList->addItem(marker);
	}
}


void MainWindow::deleteSelected() {
	for(auto marker: ui->markerList->getItems()) {
		auto sphere = dynamic_cast<SphereMarker *>(marker);
		if(sphere)
			sphere->deleteSelected(currentImage);
	}
}

void MainWindow::detectCurrentSphereHighlight() {
	QMessageBox::critical(this, "detectCurrentSphereHighlight", "To be implemented");

	/*for(auto a: ui->sphereList->selectedItems()) {
		int id = a->data(Qt::UserRole).toInt();
		assert(project.spheres.count(id));
		Sphere *sphere = project.spheres[id];
		if(!sphere->fitted) {
			QMessageBox::critical(this, "Sorry can't do.", "This sphere has no center or radius!");
			return;
		}
		sphere_to_process = id;
	}
	detectHighlights(); */
}

void MainWindow::detectHighlights() {
	if(highlightDetecting)
		return;
	highlightDetecting = true;
	if(!progress) {
		progress = new QProgressDialog("Looking for highlights...", "Cancel", 0, project.size(), this);
		connect(&watcher, SIGNAL(finished()), this, SLOT(finishedDetectHighlights()));
		connect(&watcher, SIGNAL(progressValueChanged(int)), progress, SLOT(setValue(int)));
		connect(progress, SIGNAL(canceled()), this, SLOT(cancelDetectHighlights()));
		progress->setWindowModality(Qt::WindowModal);
	}
	progress->show();
	progress->setMaximum(project.size());

	QThreadPool::globalInstance()->setMaxThreadCount(1);
	progress_jobs.clear();
	for(size_t i = 0; i < project.size(); i++)
		progress_jobs.push_back(i);
	//0 -> ok, 1 -> could not open 2 -> flipped, 3-> wrong resolution
	QFuture<void> future = QtConcurrent::map(progress_jobs, [&](int i) -> int { return detectHighlight(i); });
	watcher.setFuture(future);
}

void MainWindow::cancelDetectHighlights() {
	watcher.cancel();
	highlightDetecting = false;
}

void MainWindow::finishedDetectHighlights() {
	highlightDetecting = false;
	if(notloaded.size() || flipped.size() || resolution.size()) {
		if(notloaded.size())
			QMessageBox::critical(this, "Houston we have a problem!", "Could not load images: " + notloaded.join(", "));
		if(resolution.size())
			QMessageBox::critical(this, "Houston we have a problem!", "These images have different sizes: "+ resolution.join(", "));
		if(flipped.size())
			QMessageBox::critical(this, "Houston we have a problem!", "These images are probably just rotated: " + flipped.join(", "));
		return;
	}

/*
	//histogram for highlight threshold
	for(auto it: project.spheres) {
		Sphere *sphere = it.second;
		cout << "Sphere " << it.first << "\n";
		for(size_t i = 0; i < sphere->histogram.size(); i++) {
			cout << "Light: " << i << " ";
			for(int n: sphere->histogram[i])
				cout << n << " ";
			cout << "\n";
		}
		cout << endl;
	}
*/
	project.computeDirections();
	showHighlights(currentImage);
	/*QModelIndexList selected = ui->imageList1->selectionModel()->selectedIndexes();
	if(!selected.size())
		return;
	openImage(selected[0]); */
}

int MainWindow::detectHighlight(int n) {
	if(project.images[size_t(n)].skip) return 0;

	QString filename = project.images[n].filename;
	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		notloaded.push_back(project.images[n].filename);
		return 0;
	}
	if(img.size() != project.imgsize) {
		if(img.size() == project.imgsize.transposed())
			flipped.push_back(project.images[n].filename);
		else
			resolution.push_back(project.images[n].filename);
		return 0;
	}


	for(auto sphere: project.spheres)
		if(sphere->fitted)
			sphere->findHighlight(img, n);

	return 1;
}

void MainWindow::quit() {
	int res = QMessageBox::question(this, "Closing relight.", "Sure?");
	if(res == QMessageBox::Yes)
		exit(0);
}

/*load LP:
   if no images:
		   look for images in the directory of the LP,
				 if no images found ask for directory with images.
					 if wrong number: complain and ask for again/cancel.
					 if image names not matching jwarn and -> use order or cancel.
					 load images
	if images:
			wrong number: complain
			wrong names ->use order or cancel
*/


void MainWindow::loadLP() {
	QString lp = QFileDialog::getOpenFileName(this, "Select light direction file", QString(), "Light directions (*.lp)");
	if(lp.isNull())
		return;
	loadLP(lp);
}

void MainWindow::loadLP(QString lp) {
	vector<QString> filenames;
	std::vector<Vector3f> directions;

	try {
		ImageSet::parseLP(lp, directions, filenames);

	} catch(QString error) {
		QMessageBox::critical(this, "LP file invalid: ", error);
		return;
	}

	if(project.size() != filenames.size()) {
		auto response = QMessageBox::question(this, "Light directions and images",
			QString("The folder contains %1 images, the .lp file specify %1 images.\n")
					.arg(project.size()).arg(filenames.size()));
		if(response == QMessageBox::Cancel || response == QMessageBox::No)
			return;
	}

	vector<Vector3f> ordered_dir(directions.size());
	bool success = true;
	for(size_t i = 0; i < filenames.size(); i++) {
		QString &s = filenames[i];
		int pos = project.indexOf(s);
		if(pos == -1) {
			success = false;
			break;
		}
		ordered_dir[pos] = directions[i];
	}

	if(success) {
		for(size_t i = 0; i < project.size(); i++)
			project.images[i].direction = ordered_dir[i];
	} else {
		auto response = QMessageBox::question(this, "Light directions and images",
			"Filenames in .lp do not match with images in the .lp directory. Do you want to just use the filename order?");
		if(response == QMessageBox::Cancel || response == QMessageBox::No)
			return;
		for(size_t i = 0; i < project.size(); i++)
			project.images[i].direction = directions[i];
	}
}
void MainWindow::saveLPs() {
	int count = 1;
	QString basename = "sphere";
	for(auto sphere: project.spheres) {
		QString filename = basename;
		filename += QString::number(count++);
		filename += ".lp";

		filename = project.dir.filePath(filename);
		sphere->computeDirections(project.lens);
		project.saveLP(filename, sphere->directions);
	}
	QMessageBox::information(this, "Saved LPs", QString("Saved %1 .lp's in images folder.").arg(project.spheres.size()));
	project.computeDirections();

	vector<Vector3f> directions;
	for(auto img: project.images)
		directions.push_back(img.direction);

	project.saveLP(basename + ".lp", directions);
}

void MainWindow::exportNormals() {
	exportRTI(true);
}

void MainWindow::convertRTI() {
	if(!convert)
		convert = new ConvertDialog(this);
	convert->show();
}

void MainWindow::exportRTI(bool normals) {
	if(project.spheres.size())
		project.computeDirections();

	QStringList nodir;
	for(size_t i = 0; i < project.size(); i++) {
		if(project.images[i].skip)
			continue;

		if(project.images[i].direction.isZero())
			nodir.push_back(project.images[i].filename);
	}
	if(nodir.size()) {
		QMessageBox::critical(this, "Could not export RTI.", "Some images lack a light direction: " + nodir.join(", "));
		return;
	}


	//should init with saved preferences.
	rtiexport->setTabIndex(normals? 1 : 0);
	rtiexport->setImages(project.getImages());

	rtiexport->showImage(imagePixmap->pixmap());
	rtiexport->lights = project.directions();
	rtiexport->path = project.dir.path();
	rtiexport->setModal(true);

	rtiexport->show();
	//this needs to be called AFTER show, to ensure proportions are computed properly
	rtiexport->setCrop(project.crop);
	rtiexport->exec();
	project.crop = rtiexport->crop;
	if(ProcessQueue::instance().queue.size())
		showQueue();
}

void MainWindow::viewRTI() {
	QString output = QFileDialog::getExistingDirectory(this, "Select an output directory");
	if(output.isNull()) return;

	HttpServer &server = HttpServer::instance();
	server.port = QSettings().value("port", 8080).toInt();
	server.start(output);
	server.show();
}

void MainWindow::showQueue() {
	if(!queue)
		queue = new QueueWindow(this);

	queue->show();
	queue->raise();
}

void MainWindow::editLensParameters() {
	FocalDialog *focal = new FocalDialog(&project, this);
	focal->setWindowModality(Qt::WindowModal);
	bool result = focal->exec();
	delete focal;
}

void MainWindow::whiteBalance() {

}

void MainWindow::domeCalibration() {
	DomeCalibration *calibration = new DomeCalibration(this, project.dome);
	calibration->setModal(true);
	calibration->exec();
	project.dome = calibration->dome;
}

void MainWindow::deepZoom()
{
    zoom->setTabIndex(0);

    zoom->setModal(true);
    zoom->show();
    zoom->exec();

    if(ProcessQueue::instance().queue.size())
        showQueue();
}

void MainWindow::tarZoom()
{
    zoom->setTabIndex(1);

    zoom->setModal(true);
    zoom->show();
    zoom->exec();

    if(ProcessQueue::instance().queue.size())
        showQueue();
}

void MainWindow::itarZoom()
{
    zoom->setTabIndex(2);

    zoom->setModal(true);
    zoom->show();
    zoom->exec();

    if(ProcessQueue::instance().queue.size())
        showQueue();
}


void MainWindow::showHelp() {
	help->show();
//	help->setUrl(":/docs/help.html");
}

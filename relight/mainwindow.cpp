#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "graphics_view_zoom.h"
#include "rtiexport.h"
#include "../src/imageset.h"
#include "helpdialog.h"
#include "focaldialog.h"
#include "scripts.h"
#include "queuewindow.h"
#include "httpserver.h"
#include "settingsdialog.h"
#include "domecalibration.h"

#include "qmeasuremarker.h"
#include "qspheremarker.h"

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
	connect(ui->actionNew,        SIGNAL(triggered(bool)),  this, SLOT(newProject()));
	connect(ui->actionOpen,       SIGNAL(triggered(bool)),  this, SLOT(openProject()));

	connect(ui->actionSave,       SIGNAL(triggered(bool)),  this, SLOT(saveProject()));
	connect(ui->actionSave_as,    SIGNAL(triggered(bool)),  this, SLOT(saveProjectAs()));
	connect(ui->actionPreferences,    SIGNAL(triggered(bool)),  this, SLOT(preferences()));
	connect(ui->actionExit,       SIGNAL(triggered(bool)),  this, SLOT(quit()));

	connect(ui->actionRuler,      SIGNAL(triggered(bool)),  this, SLOT(startMeasure()));
	connect(ui->actionPrevious,   SIGNAL(triggered(bool)),  this, SLOT(previous()));
	connect(ui->actionNext,       SIGNAL(triggered(bool)),  this, SLOT(next()));
	connect(ui->actionToggle_max_luma, SIGNAL(triggered(bool)), this, SLOT(toggleMaxLuma()));
	connect(ui->actionExport_RTI, SIGNAL(triggered(bool)),  this, SLOT(exportRTI()));
	connect(ui->actionExport_Normals, SIGNAL(triggered(bool)),  this, SLOT(exportNormals()));

	connect(ui->actionView_RTI,     SIGNAL(triggered(bool)),  this, SLOT(viewRTI()));

	connect(ui->actionShow_queue,   SIGNAL(triggered(bool)),  this, SLOT(showQueue()));


	connect(ui->actionAdd_a_sphere, SIGNAL(triggered(bool)),   this, SLOT(addSphere()));
	connect(ui->addSphere,          SIGNAL(clicked(bool)),   this, SLOT(addSphere()));
	connect(ui->removeSphere,       SIGNAL(clicked(bool)),   this, SLOT(removeSphere()));
	connect(ui->saveLP,             SIGNAL(clicked(bool)),   this, SLOT(saveLPs()));

	connect(ui->detectHighlights, SIGNAL(clicked(bool)),   this, SLOT(detectHighlights()));
	connect(ui->actionDetectHighlights, SIGNAL(triggered(bool)),   this, SLOT(detectHighlights()));

	connect(ui->actionSave_LP,    SIGNAL(triggered(bool)), this, SLOT(saveLPs()));
	connect(ui->actionLoad_LP,    SIGNAL(triggered(bool)), this, SLOT(loadLP()));

	connect(ui->actionLens_parameters, SIGNAL(triggered(bool)), this, SLOT(editLensParameters()));
	connect(ui->actionDome_geometry, SIGNAL(triggered(bool)), this, SLOT(domeCalibration()));
	connect(ui->actionWhite_balance, SIGNAL(triggered(bool)), this, SLOT(whiteBalance()));

	connect(ui->actionHelp,       SIGNAL(triggered(bool)), this, SLOT(showHelp()));


	connect(ui->newSphere, SIGNAL(clicked()), this, SLOT(newSphere()));
	connect(ui->newWhite, SIGNAL(clicked()), this, SLOT(newWhite()));
	connect(ui->newAlign, SIGNAL(clicked()), this, SLOT(newAlign()));
	connect(ui->newMeasure, SIGNAL(clicked()), this, SLOT(newMeasure()));


	//ui->imageList->setContextMenuPolicy(Qt::CustomContextMenu);
	//connect(ui->imageList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(imagesContextMenu(QPoint)));

	ui->sphereList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->sphereList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(spheresContextMenu(QPoint)));


	connect(ui->actionDelete_selected,     SIGNAL(triggered(bool)),   this, SLOT(deleteSelected()));

	//connect(ui->imageList,     SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(openImage(QListWidgetItem *)));
	//connect(ui->sphereList,     SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changeSphere(QListWidgetItem *, QListWidgetItem *)));


	scene = new RTIScene(this);
	connect(scene, SIGNAL(borderPointMoved()), this, SLOT(updateBorderPoints()));
	connect(scene, SIGNAL(highlightMoved()), this, SLOT(updateHighlight()));


	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);
	QApplication::setOverrideCursor( Qt::ArrowCursor );
	//QGraphicsView override zoom when dragging.
	//ui->graphicsView->viewport()->setCursor(Qt::CrossCursor);

	auto *gvz = new Graphics_view_zoom(ui->graphicsView);
	connect(gvz, SIGNAL(dblClicked(QPoint)), this, SLOT(pointPicked(QPoint)));
	connect(gvz, SIGNAL(clicked(QPoint)), this, SLOT(pointClick(QPoint)));
	connect(ui->actionZoom_in,  SIGNAL(triggered(bool)), gvz, SLOT(zoomIn()));
	connect(ui->actionZoom_out, SIGNAL(triggered(bool)), gvz, SLOT(zoomOut()));


	rtiexport = new RtiExport(this);
	help = new HelpDialog(this);
	imageModel = new QStandardItemModel(ui->imageList1);
	ui->imageList1->setModel(imageModel);


	// Register model item  changed signal
	QItemSelectionModel *selectionModel = ui->imageList1->selectionModel();

	//TODO remove QListWidget!
//	ui->imageList->setVisible(false);
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
//	ui->imageList->clear();
	ui->graphicsView->resetMatrix();
	ui->sphereList->clear();
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
		for(Image &image: project.images1) {
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
	ui->actionLoad_LP->setEnabled(true);
	ui->actionSave_LP->setEnabled(true);
	ui->actionZoom_in->setEnabled(true);
	ui->actionZoom_out->setEnabled(true);

	ui->addSphere->setEnabled(true);
	ui->removeSphere->setEnabled(true);
	if(project.hasDirections())
		ui->actionSave_LP->setEnabled(true);


	ui->newSphere->setEnabled(true);
	ui->newMeasure->setEnabled(true);
	ui->newAlign->setEnabled(true);
	ui->newWhite->setEnabled(true);
}

bool MainWindow::init() {

	if(imagePixmap)
		delete imagePixmap;
	settings->setValue("LastDir", project.dir.path());

//	ui->imageList->clear();

	ui->markerList->clear();
	setupSpheres();
	setupMeasures();



	//create the items (name and TODO thumbnail
	int count = 0;
	imageModel->clear();
	for(Image &a: project.images1) {
		//auto *item = new QListWidgetItem(QString("%1 - %2").arg(count+1).arg(a.filename), ui->imageList);
		//item ->setData(Qt::UserRole, count);

		QStandardItem *listItem = new QStandardItem;
		//if(!a.valid)
		//	poListItem->setBackground(Qt::red);
		listItem->setText(QString("%1 - %2").arg(count+1).arg(a.filename));
		listItem->setCheckable(true);
		// Uncheck the item
		listItem->setCheckState(a.valid ? Qt::Checked : Qt::Unchecked);
		listItem->setData(a.valid ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
		listItem->setData(count, Qt::UserRole+1);
		listItem->setBackground(a.hasLightDirection() ? Qt::darkGreen : QBrush());
		imageModel->setItem(count, listItem);

		count++;
	}

	openImage(0);//ui->imageList->item(0), true);

	ui->addSphere->setEnabled(true);
	ui->removeSphere->setEnabled(true);
	ui->detectHighlights->setEnabled(true);
	ui->saveLP->setEnabled(true);
	return true;
}

void MainWindow::imageChecked(QStandardItem *item) {
	QModelIndex index = imageModel->indexFromItem(item);
	Image &image = project.images1[index.row()];
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

/*void MainWindow::openImage(QListWidgetItem *item, bool fit) {
	if(!item)
		return;
	ui->imageList->setCurrentItem(item);
	int id = item->data(Qt::UserRole).toInt();
	openImage(id, fit);
}*/

void MainWindow::openImage(int id, bool fit) {
	QString filename = project.images1[id].filename;//item->text();

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
	//if(!project.imgsize.isValid())
	//	project.imgsize = img.size();

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
		auto m = dynamic_cast<QSphereMarker *>(marker);
		if(!m)
			continue;
		m->showHighlight(n);
		//Sphere *sphere = marker->sphere;
		//if(!sphere->fitted)
		//	continue;

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
		for(auto image: project.images1)
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

/*void MainWindow::startMeasure() {
	//auto measure = project.newMeasure();
	//auto item = new QMeasureMarker(measure, scene, this);
	//ui->
	//measure = new Measure();
	//measure->setScene(scene);
	QApplication::setOverrideCursor( Qt::CrossCursor );
} */
/*
void MainWindow::endMeasure() {
	QApplication::setOverrideCursor( Qt::ArrowCursor );

	bool ok = true;
	double length = QInputDialog::getDouble(this, "Enter a measurement", "The distance between the two points in mm.", 0.0, 0.0, 1000000.0, 1, &ok);
	if(!ok) {
		return;
	}

	measure->setLength(length);
	measure = nullptr;
}*/


void MainWindow::pointClick(QPoint p) {
	QPointF pos = ui->graphicsView->mapToScene(p);

	for(auto marker: ui->markerList->getItems()) {
		if(marker->editing)
			return marker->click(pos);
	}
	/*
	if(!measure)
		return;

	
	if(measure->measuring == Measure::FIRST_POINT) {
		measure->setFirstPoint(pos);

	} else if(measure->measuring == Measure::SECOND_POINT) {
		measure->setSecondPoint(pos);
		endMeasure();
	}*/
	
}
void MainWindow::pointPicked(QPoint p) {
	//works only on images with correct resolution and lens.
	Image image = project.images1[currentImage];
	if(!image.valid)
		return;

	QPointF pos = ui->graphicsView->mapToScene(p);
	
	QBrush blueBrush(Qt::blue);

	QPen outlinePen(Qt::white);
	outlinePen.setCosmetic(true);
	outlinePen.setWidth(5);
	auto borderPoint = new BorderPoint(-3, -3, 6, 6);
	borderPoint->setPos(pos.x(), pos.y());
	borderPoint->setPen(outlinePen);
	borderPoint->setBrush(blueBrush);
	borderPoint->setFlag(QGraphicsItem::ItemIsMovable);
	borderPoint->setFlag(QGraphicsItem::ItemIsSelectable);
	borderPoint->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	borderPoint->setCursor(Qt::CrossCursor);
	scene->addItem(borderPoint);


	//auto item = ui->sphereList->selectedItems()[0];
	//int id = item->data(Qt::UserRole).toInt();
	//assert(project.spheres.count(id));
	//Sphere *sphere = project.spheres[id];
	//sphere->border.push_back(borderPoint);

	//updateBorderPoints();
}

void MainWindow::updateBorderPoints() {
	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<QSphereMarker *>(marker);
		if(!m)
			continue;
		m->fit(project.imgsize);
	}
}

void MainWindow::updateHighlight() {
	if(ignore_scene_changes)
		return;

	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<QSphereMarker *>(marker);
		if(!m)
			continue;

		if(!m->sphere->fitted)
			continue;
		m->updateHighlightPosition(currentImage);

		QStandardItem *item = imageModel->item(currentImage);
		item->setBackground(Qt::darkGreen);
	}
}

void MainWindow::deleteSelected() {
	for(auto marker: ui->markerList->getItems()) {
		auto m = dynamic_cast<QSphereMarker *>(marker);
		if(!m)
			continue;
		m->deleteSelected();
	}
}

/*
void MainWindow::changeSphere(QListWidgetItem *current, QListWidgetItem *previous) {

//	for(auto sphere: project.spheres)
//		sphere.second->setActive(false);

	if(!current)
		return;

	int current_id = current->data(Qt::UserRole).toInt();
	if(!project.spheres.count(current_id))
		throw QString("A sphere was not properly deleted!");
	project.spheres[current_id]->setActive(true);
} */

void MainWindow::newSphere() {
	auto sphere = project.newSphere();
	auto marker = new QSphereMarker(sphere, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
}

void MainWindow::newMeasure() {
	auto measure = project.newMeasure();
	auto marker = new QMeasureMarker(measure, ui->graphicsView, this);
	ui->markerList->addItem(marker);
	ui->markerList->setSelected(marker);
	marker->startMeasure();
}

void MainWindow::newAlign() {

}

void MainWindow::newWhite() {

}

/*

int MainWindow::addSphere() {
	for(auto &sphere: project.spheres)
		sphere.second->setActive(false);

	ignore_scene_changes = true;
	std::set<int> used;

	for(int i = 0; i < ui->sphereList->count(); ++i)
		used.insert(ui->sphereList->item(i)->data(Qt::UserRole).toInt());

	int id = 0;;
	while(used.count(id))
		id++;
	Sphere *sphere = new Sphere(project.size());
	project.spheres[id] = sphere;
	setupSphere(id, sphere);
	ignore_scene_changes = false;
	return id;
}
 */
void MainWindow::setupMeasures() {
	for(auto m: project.measures) {
		//m->setScene(scene);
		//m->setVisible(true);

		auto item = new QMeasureMarker(m, ui->graphicsView, ui->markerList);
		ui->markerList->addItem(item);
	}
}

void MainWindow::setupSpheres() {
	for(auto sphere: project.spheres) {
		//int id = b.first;
		//setupSphere(id, sphere);

		auto item = new QSphereMarker(sphere, ui->graphicsView, ui->markerList);
		ui->markerList->addItem(item);
	}
}

/*void MainWindow::setupSphere(int id, Sphere *sphere) {
	auto *item = new QListWidgetItem(QString("Shere %1").arg(id+1), ui->sphereList);
	item->setSelected(true);
	item ->setData(Qt::UserRole, id);

	QPen outlinePen(Qt::yellow);
	outlinePen.setCosmetic(true);
	sphere->circle = scene->addEllipse(0, 0, 1, 1, outlinePen);
	sphere->smallcircle = scene->addEllipse(0, 0, 1, 1, outlinePen);
	if(sphere->center.isNull()) {
		sphere->circle->setVisible(false);
		sphere->smallcircle->setVisible(false);
	} else {
		QPointF c = sphere->center;
		double R = double(sphere->radius);
		double r = double(sphere->smallradius);
		sphere->circle->setRect(c.x()-R, c.y()-R, 2*R, 2*R);
		sphere->circle->setVisible(true);
		sphere->smallcircle->setRect(c.x()-r, c.y()-r, 2*r, 2*r);
		sphere->smallcircle->setVisible(true);
	}


	auto high = new HighlightPoint(-2, -2, 2, 2);
	high->setVisible(false);
	QPen pen;
	pen.setColor(Qt::transparent);
	pen.setWidth(0);
	high->setPen(pen);
	high->setBrush(Qt::green);
	high->setFlag(QGraphicsItem::ItemIsMovable);
	high->setFlag(QGraphicsItem::ItemIsSelectable);
	high->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	sphere->highlight = high;
	scene->addItem(high);

	for(auto b: sphere->border)
		scene->addItem(b);
} */

void MainWindow::removeSphere() {
	QMessageBox::critical(this, "Removing sphere", "To be implemented");
	/*for(auto a: ui->sphereList->selectedItems()) {
		int id = a->data(Qt::UserRole).toInt();
		assert(project.spheres.count(id));
		Sphere *sphere = project.spheres[id];
		delete sphere;
		project.spheres.erase(id);
	}
	qDeleteAll(ui->sphereList->selectedItems()); */
}

void MainWindow::imagesContextMenu(QPoint) {

}


void MainWindow::markersContextMenu(QPoint pos) {
	auto *marker = ui->markerList->itemAt(pos);
	if(!marker) return;

	QMenu menu;
	menu.addAction("New sphere", this, SLOT(newSphere()));
	menu.addAction("New alignment", this, SLOT(newAlign()));
	menu.addAction("New white balance", this, SLOT(newWhite()));
	menu.addAction("New measure", this, SLOT(newMeasure()));
	menu.addSeparator();
	//menu.addAction("Find highlights",  this, SLOT(detectCurrentSphereHighlight()));
	menu.exec(mapToGlobal(pos));
}

/*
void MainWindow::spheresContextMenu(QPoint pos) {
	auto *item = ui->sphereList->itemAt(pos);
	if(!item) return;
	ui->sphereList->setCurrentItem(item);

	QMenu myMenu;
	myMenu.addAction("Find highlights",  this, SLOT(detectCurrentSphereHighlight()));
	myMenu.addAction("Insert", this, SLOT(addSphere()));
	myMenu.addSeparator();
	myMenu.addAction("RemoveThisSphere",  this, SLOT(removeSphere()));


	myMenu.exec(mapToGlobal(pos));
} */


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

#ifdef NDEBUG
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
#endif
	project.computeDirections();
	QModelIndexList selected = ui->imageList1->selectionModel()->selectedIndexes();
	if(!selected.size())
		return;
	openImage(selected[0]);

/*	auto selected = ui->imageList->selectedItems();
	if(selected.size() == 0)
		return;
	openImage(selected[0]); */
}

int MainWindow::detectHighlight(int n) {
	if(project.images1[size_t(n)].skip) return 0;

	QString filename = project.images1[n].filename;
	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		notloaded.push_back(project.images1[n].filename);
		return 0;
	}
	if(img.size() != project.imgsize) {
		if(img.size() == project.imgsize.transposed())
			flipped.push_back(project.images1[n].filename);
		else
			resolution.push_back(project.images1[n].filename);
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
			project.images1[i].direction = ordered_dir[i];
	} else {
		auto response = QMessageBox::question(this, "Light directions and images",
			"Filenames in .lp do not match with images in the .lp directory. Do you want to just use the filename order?");
		if(response == QMessageBox::Cancel || response == QMessageBox::No)
			return;
		for(size_t i = 0; i < project.size(); i++)
			project.images1[i].direction = directions[i];
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
	for(auto img: project.images1)
		directions.push_back(img.direction);

	project.saveLP(basename + ".lp", directions);
}

void MainWindow::exportNormals() {
	exportRTI(true);
}

void MainWindow::exportRTI(bool normals) {
	if(project.spheres.size())
		project.computeDirections();

	QStringList nodir;
	for(size_t i = 0; i < project.size(); i++) {
		if(project.images1[i].skip)
			continue;

		if(project.images1[i].direction.isZero())
			nodir.push_back(project.images1[i].filename);
	}
	if(nodir.size()) {
		QMessageBox::critical(this, "Could not export RTI.", "Some images lack a light direction: " + nodir.join(", "));
		return;
	}


	//should init with saved preferences.
	rtiexport->setTabIndex(normals? 1 : 0);
	rtiexport->setImages(project.images());

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


void MainWindow::showHelp() {
	help->show();
//	help->setUrl(":/docs/help.html");
}

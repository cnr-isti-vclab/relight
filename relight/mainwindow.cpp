#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "graphics_view_zoom.h"
#include "rtiexport.h"
#include "helpdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QSettings>

#include <QtConcurrent/QtConcurrent>

#include <set>
#include <iostream>

#include <assert.h>
using namespace std;


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	settings = new QSettings("VCG", "Relight", this);

	ui->setupUi(this);
	connect(ui->actionNew,        SIGNAL(triggered(bool)),              this, SLOT(newProject()));
	connect(ui->actionOpen,       SIGNAL(triggered(bool)),              this, SLOT(openProject()));

	connect(ui->actionSave,       SIGNAL(triggered(bool)),              this, SLOT(saveProject()));
	connect(ui->actionSave_as,    SIGNAL(triggered(bool)),              this, SLOT(saveProjectAs()));
	connect(ui->actionPrevious,   SIGNAL(triggered(bool)),              this, SLOT(previous()));
	connect(ui->actionNext,       SIGNAL(triggered(bool)),              this, SLOT(next()));
	connect(ui->actionExport_RTI, SIGNAL(triggered(bool)), this, SLOT(exportRTI()));

	connect(ui->addSphere,      SIGNAL(clicked(bool)),                this, SLOT(addSphere()));
	connect(ui->removeSphere,   SIGNAL(clicked(bool)),                this, SLOT(removeSphere()));
	connect(ui->process,        SIGNAL(clicked(bool)),                this, SLOT(process()));
	connect(ui->actionSave_LP, SIGNAL(triggered(bool)), this, SLOT(saveLPs()));
	connect(ui->loadLP, SIGNAL(clicked(bool)), this, SLOT(loadLP()));



	//connect(ui->actionProcess,  SIGNAL(triggered(bool)),            this, SLOT(process()));
	connect(ui->actionDelete_selected,     SIGNAL(triggered(bool)),   this, SLOT(deleteSelected()));


	connect(ui->imageList,     SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(openImage(QListWidgetItem *)));
	connect(ui->sphereList,     SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changeSphere(QListWidgetItem *, QListWidgetItem *)));


	scene = new RTIScene(this);
	connect(scene, SIGNAL(borderPointMoved()), this, SLOT(updateBorderPoints()));
	connect(scene, SIGNAL(highlightMoved()), this, SLOT(updateHighlight()));

	//	connect(scene, SIGNAL(changed(QList<QRectF>)), this, SLOT(updateBalls()));


	ui->graphicsView->setScene(scene);
	ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
	ui->graphicsView->setInteractive(true);
	QApplication::setOverrideCursor( Qt::ArrowCursor );

	auto *gvz = new Graphics_view_zoom(ui->graphicsView);
	connect(gvz, SIGNAL(dblClicked(QPoint)), this, SLOT(pointPicked(QPoint)));
	connect(ui->actionZoom_in,  SIGNAL(triggered(bool)), gvz, SLOT(zoomIn()));
	connect(ui->actionZoom_out, SIGNAL(triggered(bool)), gvz, SLOT(zoomOut()));



	connect(ui->actionHelp, SIGNAL(triggered(bool)), this, SLOT(showHelp()));

	rtiexport = new RtiExport(this);
	help = new HelpDialog(this);
}

MainWindow::~MainWindow() {	delete ui; }

void MainWindow::clear() {
	if(imagePixmap) {
		delete imagePixmap;
		imagePixmap = nullptr;
	}

	ui->imageList->clear();
	project.imgsize = QSize();

	project = Project();
}

void MainWindow::newProject() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder", lastDir);
	if(dir.isNull()) return;
	clear();
	project.setDir(QDir(dir));
	enableActions();
	init();
}

void MainWindow::openProject() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();

	QString filename = QFileDialog::getOpenFileName(this, "Select a project", lastDir, "*.relight");
	if(filename.isNull())
		return;

	clear();

	Project p;
	try {
		p.load(filename);
	} catch(QString e) {
		QMessageBox::critical(this, "Could not load the project: " + filename, "Error: " + e);
		return;
	}
	project = p;
	enableActions();
	init();
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

	ui->addSphere->setEnabled(true);
	ui->removeSphere->setEnabled(true);
	ui->loadLP->setEnabled(true);
	if(project.hasDirections())
		ui->actionSave_LP->setEnabled(true);
}

bool MainWindow::init() {
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	for(QString &s: project.dir.entryList(img_ext))
		project.images1.push_back(Image(s));
	if(!project.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + project.dir.path());
		return false;
	}

	if(imagePixmap)
		delete imagePixmap;
	settings->setValue("LastDir", project.dir.path());

	ui->imageList->clear();
	project.imgsize = QSize();

	//create the items (name and TODO thumbnail
	int count = 0;
	for(Image &a: project.images1) {
		auto *item = new QListWidgetItem(a.filename, ui->imageList);
		item ->setData(Qt::UserRole, count++);
	}

	openImage(ui->imageList->item(0), true);
	//TODO: in background load and process the images

//	addSphere();

	ui->addSphere->setEnabled(true);
	ui->removeSphere->setEnabled(true);
	ui->process->setEnabled(true);
	ui->loadLP->setEnabled(true);
	ui->saveLP->setEnabled(true);
	return true;
}

void MainWindow::openImage(QListWidgetItem *item, bool fit) {
	if(!item)
		return;

	ui->imageList->setCurrentItem(item);
	QString filename = item->text();

	QImage img(project.dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	currentImage = item->data(Qt::UserRole).toInt();
	size_t n = size_t(currentImage);
	if(imagePixmap)
		delete imagePixmap;
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene->addItem(imagePixmap);
	if(!project.imgsize.isValid())
		project.imgsize = img.size();

	if(fit) {
		//find smallest problems
		double sx =  double(ui->graphicsView->width()) / project.imgsize.width();
		double sy = double(ui->graphicsView->height()) / project.imgsize.height();
		double s = std::min(sx, sy);
		ui->graphicsView->scale(s, s);
	}

	for(auto it: project.balls) {
		Ball *ball = it.second;
		if(ball->fitted && !ball->lights[n].isNull()) {
			QRectF mark(- QPointF(2, 2), QPointF(2, 2));
			ball->highlight->setRect(mark);
			ball->highlight->setPos(ball->lights[n]);
			ball->highlight->setVisible(true);
		} else
			ball->highlight->setVisible(false);

	}
}

void MainWindow::previous() {
	if(currentImage == 0)
		return;
	openImage(ui->imageList->item(currentImage-1));

}


void MainWindow::next() {
	if(currentImage == (int)project.size()-1)
		return;
	openImage(ui->imageList->item(currentImage+1));
}

void MainWindow::pointPicked(QPoint p) {

	QPointF pos = ui->graphicsView->mapToScene(p);
	QBrush blueBrush(Qt::blue);

	QPen outlinePen(Qt::white);
	outlinePen.setCosmetic(true);
	outlinePen.setWidth(5);
	auto borderPoint = new BorderPoint(pos.x()-3, pos.y()-3, 6, 6);
	borderPoint->setPen(outlinePen);
	borderPoint->setBrush(blueBrush);
	borderPoint->setFlag(QGraphicsItem::ItemIsMovable);
	borderPoint->setFlag(QGraphicsItem::ItemIsSelectable);
	borderPoint->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	borderPoint->setCursor(Qt::CrossCursor);
	scene->addItem(borderPoint);

	if(!project.balls.size())
		addSphere();

	auto item = ui->sphereList->selectedItems()[0];
	int id = item->data(Qt::UserRole).toInt();
	Ball *ball = project.balls[id];
	ball->border.push_back(borderPoint);

	updateBorderPoints();
}

void MainWindow::updateBorderPoints() {

	for(auto &it: project.balls) {
		Ball *ball = it.second;

		ball->circle->setVisible(false);

		if(ball->border.size() >= 3) {
			bool fitted = ball->fit(project.imgsize);
			if(fitted) {
				QPointF c = ball->center;
				double r = double(ball->radius);
				ball->circle->setRect(c.x()-r, c.y()-r, 2*r, 2*r);
				ball->circle->setVisible(true);
			}
		}
	}
}

void MainWindow::updateHighlight() {
	for(auto &it: project.balls) {
		Ball *ball = it.second;
		if(!ball->highlight) continue;

		ball->lights[size_t(currentImage)] = ball->highlight->pos();
	}
}

void MainWindow::deleteSelected() {
	for(auto &it: project.balls) {
		Ball *ball = it.second;
		auto border = ball->border;
		ball->border.clear();
		std::copy_if (border.begin(), border.end(), std::back_inserter(ball->border), [border](QGraphicsEllipseItem *e) {
			bool remove = e->isSelected();
			if(remove) delete e;
			return !remove;
		});
	}
	updateBorderPoints();
}

void MainWindow::changeSphere(QListWidgetItem *current, QListWidgetItem */*previous*/) {

	for(auto &ball: project.balls)
		ball.second->setActive(false);

	int current_id = current->data(Qt::UserRole).toInt();
	project.balls[current_id]->setActive(true);
}

int MainWindow::addSphere() {
	for(auto &ball: project.balls)
		ball.second->setActive(false);

	ignore_scene_changes = true;
	std::set<int> used;

	for(int i = 0; i < ui->sphereList->count(); ++i)
		used.insert(ui->sphereList->item(i)->data(Qt::UserRole).toInt());

	int id = 0;
	while(used.count(id))
		id++;

	auto *item = new QListWidgetItem(QString("Shere %1").arg(id+1), ui->sphereList);
	item->setSelected(true);
	item ->setData(Qt::UserRole, id);
	project.balls[id] = new Ball(project.size());
	QPen outlinePen(Qt::yellow);
	outlinePen.setCosmetic(true);
	project.balls[id]->circle = scene->addEllipse(0, 0, 1, 1, outlinePen);
	project.balls[id]->circle->setVisible(false);

	QPen highpen(Qt::red);
	highpen.setWidth(3);
	highpen.setCosmetic(true);

	auto high = new HighlightPoint(0, 0, 1, 1);
	high->setVisible(false);
	high->setPen(highpen);
	high->setBrush(Qt::red);
	high->setFlag(QGraphicsItem::ItemIsMovable);
	high->setFlag(QGraphicsItem::ItemIsSelectable);
	high->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	scene->addItem(high);
	project.balls[id]->highlight = high;
	return id;
}

void MainWindow::removeSphere() {
	for(auto a: ui->sphereList->selectedItems()) {
		int id = a->data(Qt::UserRole).toInt();
		Ball *ball = project.balls[id];
		for(auto e: ball->border)
			delete e;
		if(ball->circle)
			delete ball->circle;
		if(ball->highlight)
			delete ball->highlight;
		delete ball;
		project.balls.erase(id);
	}
	qDeleteAll(ui->sphereList->selectedItems());
}


void MainWindow::process() {
	progress = new QProgressDialog("Looking for highlights...", "Cancel", 0, project.size(), this);

	QThreadPool::globalInstance()->setMaxThreadCount(1);
	progress_jobs.clear();
	for(size_t i = 0; i < project.size(); i++)
		progress_jobs.push_back(i);
	//0 -> ok, 1 -> could not open 2 -> flipped, 3-> wrong resolution
	QFuture<void> future = QtConcurrent::map(progress_jobs, [&](int i) -> int { return processImage(i); });
	watcher.setFuture(future);
	connect(&watcher, SIGNAL(finished()), this, SLOT(finishedProcess()));
	connect(&watcher, SIGNAL(progressValueChanged(int)), progress, SLOT(setValue(int)));
	connect(progress, SIGNAL(canceled()), this, SLOT(cancelProcess()));
	progress->setWindowModality(Qt::WindowModal);
}

void MainWindow::cancelProcess() {
	watcher.cancel();
}

void MainWindow::finishedProcess() {
	if(notloaded.size() || flipped.size() || resolution.size()) {
		if(notloaded.size())
			QMessageBox::critical(this, "Houston we have a problem!", "Could not load images: " + notloaded.join(", "));
		if(resolution.size())
			QMessageBox::critical(this, "Houston we have a problem!", "These images have different sizes: "+ resolution.join(", "));
		if(flipped.size())
			QMessageBox::critical(this, "Houston we have a problem!", "These images are probably just rotated: " + flipped.join(", "));
		return;
	}

	project.computeDirections();
	auto selected = ui->imageList->selectedItems();
	if(selected.size() == 0)
		return;
	openImage(selected[0]);
}

int MainWindow::processImage(int n) {
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


	for(auto &it: project.balls) {
		if(it.second->fitted) {
			it.second->findHighlight(img, n);
		}
	}
	return 1;
}

void MainWindow::quit() {

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
	QFile file(lp);
	if(!file.open(QFile::ReadOnly)) {
		QMessageBox::critical(this, "Could not load file", file.errorString());
		return;
	}
	QTextStream stream(&file);

	size_t n;
	stream >> n;

	vector<QString> filenames;
	std::vector<Vector3f> directions;

	for(size_t i = 0; i < n; i++) {
		QString s;
		Vector3f light;
		stream >> s >> light[0] >> light[1] >> light[2];
		directions.push_back(light);

		//we keep only the filename (not the path)
		QFileInfo info(s);
		s = info.fileName();
		filenames.push_back(s);
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
/*
	if(project.balls[0].border.size() == 0) {
		Ball &ball = project.balls[0];
		ball.only_directions = true;
		ball.directions = directions;
		ball.valid = valid;
		
	} else {
		int id = addSphere();
		Ball &ball = project.balls[id];
		ball.only_directions = true;
		ball.directions = directions;
		ball.valid = valid;
	} */
}
void MainWindow::saveLPs() {
	int count = 0;
	QString basename = "sphere";
	for(auto it: project.balls) {
		QString filename = basename;
		if(count > 0)
			filename += QString::number(count++);
		filename += ".lp";

		Ball *ball = it.second;
		filename = project.dir.filePath(filename);
		ball->computeDirections();
		project.saveLP(filename, ball->directions);
		//ball->saveLP(filename, project.images1);
	}
}

void MainWindow::exportRTI() {
	if(!project.size())
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
	rtiexport->setImages(project.images());
	rtiexport->showImage(imagePixmap->pixmap());
	rtiexport->lights = project.directions();
	rtiexport->path = project.dir.path();
	rtiexport->show();
	rtiexport->exec();
}

void MainWindow::showHelp() {
	help->show();
//	help->setUrl(":/docs/help.html");
}

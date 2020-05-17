#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "imagedialog.h"
#include "graphics_view_zoom.h"
#include "ballwidget.h"
//#include "ui_progress.h"

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
	connect(ui->actionOpen,     SIGNAL(triggered(bool)),              this, SLOT(open()));
	connect(ui->actionPrevious, SIGNAL(triggered(bool)),              this, SLOT(previous()));
	connect(ui->actionNext,     SIGNAL(triggered(bool)),              this, SLOT(next()));
	connect(ui->addSphere,      SIGNAL(clicked(bool)),                this, SLOT(addSphere()));
	connect(ui->removeSphere,   SIGNAL(clicked(bool)),                this, SLOT(removeSphere()));
	connect(ui->process,        SIGNAL(clicked(bool)),                this, SLOT(process()));

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
	auto *gvz = new Graphics_view_zoom(ui->graphicsView);
	connect(gvz, SIGNAL(dblClicked(QPoint)), this, SLOT(pointPicked(QPoint)));
	connect(ui->actionZoom_in,  SIGNAL(triggered(bool)), gvz, SLOT(zoomIn()));
	connect(ui->actionZoom_out, SIGNAL(triggered(bool)), gvz, SLOT(zoomOut()));

}

MainWindow::~MainWindow() {	delete ui; }

void MainWindow::open() {
	QString lastDir = settings->value("LastDir", QDir::homePath()).toString();
	QString dir = QFileDialog::getExistingDirectory(this, "Choose picture folder", lastDir);
	if(dir.isNull()) return;
	init(dir);
}

bool MainWindow::init(QString dirname) {
	dir = QDir(dirname);
	if(!dir.exists()) {
		cerr << "Could not find " << qPrintable(dirname) << " folder.\n";
		return false;
	}
	QStringList img_ext;
	img_ext << "*.jpg" << "*.JPG" << "*.NEF" << "*.CR2";
	images = dir.entryList(img_ext);
	if(!images.size()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not find images in directory: " + dirname);
		return false;
	}

	if(imagePixmap)
		delete imagePixmap;
	settings->setValue("LastDir", dir.path());

	ui->imageList->clear();
	imgsize = QSize();
	valid.clear();
	valid.resize(images.size(), true);

	//create the items (name and TODO thumbnail
	int count = 0;
	for(QString a: images) {
		auto *item = new QListWidgetItem(a, ui->imageList);
		item ->setData(Qt::UserRole, count++);
	}

	openImage(ui->imageList->item(0), true);
	//TODO: in background load and process the images

	addSphere();
	return true;
}

void MainWindow::openImage(QListWidgetItem *item, bool fit) {
	ui->imageList->setCurrentItem(item);
	QString filename = item->text();

	QImage img(dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return;
	}
	int n = item->data(Qt::UserRole).toInt();
	currentImage = n;
	if(imagePixmap)
		delete imagePixmap;
	imagePixmap = new QGraphicsPixmapItem(QPixmap::fromImage(img));
	imagePixmap->setZValue(-1);
	scene->addItem(imagePixmap);
	if(!imgsize.isValid())
		imgsize = img.size();

	if(imgsize != img.size()) {
		valid[n] = false;
	}


	if(fit) {
		//find smallest problems
		float sx =  float(ui->graphicsView->width()) / imgsize.width();
		float sy = float(ui->graphicsView->height()) / imgsize.height();
		float s = std::min(sx, sy);
		ui->graphicsView->scale(s, s);
	}

	for(auto it: balls) {
		Ball &ball = it.second;
		if(ball.fitted && ball.valid[n]) {
			ball.highlight->setRect(QRectF(ball.lights[n] - QPointF(1, 1), ball.lights[n] + QPointF(1, 1)));
			ball.highlight->setVisible(true);
		} else
			ball.highlight->setVisible(false);

	}
}

void MainWindow::previous() {
	if(currentImage == 0)
		return;
	openImage(ui->imageList->item(currentImage-1));

}


void MainWindow::next() {
	if(currentImage == images.size()-1)
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


	auto item = ui->sphereList->selectedItems()[0];
	int id = item->data(Qt::UserRole).toInt();
	Ball &ball = balls[id];
	ball.border.push_back(borderPoint);

	updateBorderPoints();
}

void MainWindow::updateBorderPoints() {

	for(auto &it: balls) {
		Ball &ball = it.second;
		//bool changed = false;
		//for(auto b: ball.border)
		//	if(b->)

		ball.circle->setVisible(false);

		if(ball.border.size() >= 3) {
			bool fitted = ball.fit(imgsize);
			if(fitted) {
				QPointF c = ball.center;
				float r = ball.radius;
				ball.circle->setRect(c.x()-r, c.y()-r, 2*r, 2*r);
				ball.circle->setVisible(true);
			}
		}
	}
}


void MainWindow::updateHighlight() {
	cout << "Updating highlight" << endl;

	//find WHICH ball and highlight is modeve.
	for(auto &it: balls) {
		Ball &ball = it.second;
		if(!ball.highlight) continue;

		cout << "ball pos: " << ball.highlight->pos().x() << endl;

	}
}



void MainWindow::deleteSelected() {
	for(auto &it: balls) {
		Ball &ball = it.second;
		auto border = ball.border;
		ball.border.clear();
		std::copy_if (border.begin(), border.end(), std::back_inserter(ball.border), [border](QGraphicsEllipseItem *e) {
			bool remove = e->isSelected();
			if(remove) delete e;
			return !remove;
		});
	}
	updateBorderPoints();
}

void MainWindow::changeSphere(QListWidgetItem *current, QListWidgetItem *previous) {
	int current_id = current->data(Qt::UserRole).toInt();
	Ball &ball = balls[current_id];
	ball.setActive(true);

	if(previous) {
		int previous_id = previous->data(Qt::UserRole).toInt();
		Ball &old = balls[previous_id];
		old.setActive(false);
	}




}

void MainWindow::addSphere() {
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
	balls[id] = Ball(images.size());
	QPen outlinePen(Qt::yellow);
	balls[id].circle = scene->addEllipse(0, 0, 1, 1, outlinePen);
	balls[id].circle->setVisible(false);

	QPen highpen(Qt::red);
	highpen.setWidth(3);
	highpen.setCosmetic(true);

	auto high = new HighlightPoint(0, 0, 1, 1);
	high->setVisible(false);
	high->setFlag(QGraphicsItem::ItemIsMovable);
	high->setFlag(QGraphicsItem::ItemIsSelectable);
	high->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
	scene->addItem(high);
	balls[id].highlight = high;
}

void MainWindow::removeSphere() {
	for(auto a: ui->sphereList->selectedItems()) {
		int id = a->data(Qt::UserRole).toInt();
		Ball &ball = balls[id];
		for(auto e: ball.border)
			delete e;
		if(ball.circle)
			delete ball.circle;
		if(ball.highlight)
			delete ball.highlight;
		balls.erase(id);
	}
	qDeleteAll(ui->sphereList->selectedItems());
}


void MainWindow::process() {
	progress = new QProgressDialog("Looking for highlights...", "Cancel", 0, images.size(), this);

	QThreadPool::globalInstance()->setMaxThreadCount(1);
	progress_jobs.clear();
	for(int i = 0; i < images.size(); i++)
		progress_jobs.push_back(i);
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
	auto selected = ui->imageList->selectedItems();
	if(selected.size() == 0) {
		cerr << "Porca paletta!" << endl;
		return;
	}
	openImage(selected[0]);
}

int MainWindow::processImage(int n) {
	if(n < 0 || n >= valid.size()) {
			cerr << "Failed!" << endl;
			return 0;
	}
	if(!valid[n]) return 0;

	QString filename = images[n];
	QImage img(dir.filePath(filename));
	if(img.isNull()) {
		QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
		return 0;
	}
	if(img.size() != imgsize) {
		QMessageBox::critical(this, "Houston we have a problem!", "All images must be the same size! (" + filename + " doesn't...)");
		return 0;
	}
	for(auto &it: balls) {
		if(it.second.fitted)
			it.second.findHighlight(img, n);
	}
	return 1;
}

void MainWindow::quit() {

}

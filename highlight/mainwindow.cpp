#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "imagedialog.h"
#include "graphics_view_zoom.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include <QSettings>

#include <set>
#include <iostream>
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
	connect(ui->process,   SIGNAL(clicked(bool)),                this, SLOT(process()));

	//connect(ui->actionProcess,  SIGNAL(triggered(bool)),            this, SLOT(process()));
	connect(ui->actionDelete_selected,     SIGNAL(triggered(bool)),   this, SLOT(deleteSelected()));


	connect(ui->imageList,     SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openImage(QListWidgetItem *)));
	connect(ui->imageList,     SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(openImage(QListWidgetItem *)));

	scene = new RTIScene(this);
	connect(scene, SIGNAL(borderPointMoved()), this, SLOT(updateBalls()));
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

	updateBalls();
}

void MainWindow::updateBalls() {

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
	updateBalls();
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
	balls[id].highlight = scene->addEllipse(0, 0, 1, 1, highpen, Qt::red);
	balls[id].highlight->setVisible(false);
}

void MainWindow::removeSphere() {
	for(auto a: ui->sphereList->selectedItems()) {
		int id = a->data(Qt::UserRole).toInt();
		Ball &ball = balls[id];
		for(auto e: ball.border)
			delete e;
		if(ball.circle)
			delete ball.circle;
		balls.erase(id);
	}
	qDeleteAll(ui->sphereList->selectedItems());
}


void MainWindow::process() {
	auto b = ui->sphereList->selectedItems();
	int id = b[0]->data(Qt::UserRole).toInt();
	Ball &ball = balls[id];

	for(int i = 0; i < images.size(); i++) {
		if(!valid[i]) continue;
		QString filename = images[i];
		QImage img(dir.filePath(filename));
		if(img.isNull()) {
			QMessageBox::critical(this, "Houston we have a problem!", "Could not load image " + filename);
			return;
		}
		if(img.size() != imgsize) {
			QMessageBox::critical(this, "Houston we have a problem!", "All images must be the same size! (" + filename + " doesn't...)");
			return;
		}
		for(auto &it: balls) {
			if(it.second.fitted)
				it.second.findHighlight(img, i);
		}
	}
	/*
	for(auto &ball: balls) {
		//TODO check for smallradius circle not going out of image.
		//TODO process disabled until we have a center and radius
		//TODO spherelights shown with lights positions...
		QImage spherelights(ball.inner.width(), ball.inner.height(), QImage::Format_RGB32);
		spherelights.fill(0);

		//iterate over images, find center.
		//TODO warn the user when light findingn failed. (and have him provide manually!
	QStringList failed;
	for(int i = 0; i < balls.size(); i++) {
		QPointF light = findLightDir(spherelights, dir.filePath(balls[i]));
		if(light == QPointF(0, 0))
			failed.push_back(balls[i]);
		cout << "light: " << light.x() << " " << light.y() << endl;

		lights.push_back(light); //still image coordinates
	}

	if(failed.size()) {
		QString str = failed.join('\n');
		QMessageBox::critical(this, "Too bad we miss some light.", "Couldn't find the highlight in the following images:" + str);
	}

	for(size_t i = 0; i < lights.size(); i++) {
		QPointF l = lights[i];
		int x = l.x() -inner.left();
		int y = l.y() - inner.top();
		spherelights.setPixel(x, y, qRgb(255, 0, 0));
	}
	spherelights.save(dir.filePath("sphere.png"));

	auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(spherelights));
	item->setPos(inner.topLeft());
	scene->addItem(item);

	cout << "Done!" << endl;
	processed = failed.size() == 0; */
}

void MainWindow::quit() {

}
